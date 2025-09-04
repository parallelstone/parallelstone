/**
 * @file core.cpp
 * @brief Implementation of ParellelStone server core
 * @details This file implements the main server core functionality including
 *          initialization, connection handling, and lifecycle management.
 * @author @logpacket (https://github.com/logpacket)
 * @version 1.0.0
 * @date 2025-07-16
 */

#include "server/core.hpp"
#include "server/session_manager.hpp"
#include <spdlog/spdlog.h>
#include <iostream>
#include <thread>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cstddef>
#include <cstdint>
#include <cstring>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

namespace parallelstone {
namespace server {

ServerCore::ServerCore(const ServerConfig& config)
    : m_config(config)
    , m_state(ServerState::STOPPED)
    , m_statistics()
    , m_listen_socket(0)
    , m_shutdown_requested(false) {
    
    // Initialize statistics
    m_statistics.Reset();
    
    // Create network implementation
    m_network = std::shared_ptr<network::INetworkCore>(network::CreateNetworkCore());
    
    // Create session manager
    SessionManagerConfig session_config;
    session_config.SetMaxSessions(m_config.max_connections)
                  .SetSessionTimeout(m_config.session_timeout);
    
    m_session_manager = std::make_unique<SessionManager>(session_config, m_network);
}

ServerCore::~ServerCore() {
    Stop();
}

network::NetworkResult ServerCore::Start() {
    if (m_state.load() != ServerState::STOPPED) {
        return network::NetworkResult::SUCCESS;
    }

    SetState(ServerState::STARTING);
    EmitEvent(ServerState::STARTING, "Server starting up...");

    // Initialize network subsystem
    network::NetworkResult result = InitializeNetwork();
    if (result != network::NetworkResult::SUCCESS) {
        SetState(ServerState::FAILED);
        EmitEvent(ServerState::FAILED, "Failed to initialize network: " + 
                 std::string(network::NetworkResultToString(result)));
        return result;
    }

    // Create and bind listening socket
    result = m_network->CreateSocket(m_listen_socket, AF_INET, SOCK_STREAM);
    if (result != network::NetworkResult::SUCCESS) {
        SetState(ServerState::FAILED);
        EmitEvent(ServerState::FAILED, "Failed to create listening socket");
        return result;
    }

    // Bind to address
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(m_config.port);
    
    if (m_config.bind_address == "0.0.0.0") {
        addr.sin_addr.s_addr = INADDR_ANY;
    } else {
        inet_pton(AF_INET, m_config.bind_address.c_str(), &addr.sin_addr);
    }

    result = m_network->BindSocket(m_listen_socket, &addr, sizeof(addr));
    if (result != network::NetworkResult::SUCCESS) {
        m_network->CloseSocket(m_listen_socket);
        SetState(ServerState::FAILED);
        EmitEvent(ServerState::FAILED, "Failed to bind to " + m_config.bind_address + 
                 ":" + std::to_string(m_config.port));
        return result;
    }

    // Start listening
    result = m_network->ListenSocket(m_listen_socket, SOMAXCONN);
    if (result != network::NetworkResult::SUCCESS) {
        m_network->CloseSocket(m_listen_socket);
        SetState(ServerState::FAILED);
        EmitEvent(ServerState::FAILED, "Failed to start listening");
        return result;
    }

    // Start session manager
    m_session_manager->Start();

    // Initialize worker threads
    InitializeWorkerThreads();

    // Start accepting connections
    StartAcceptLoop();

    SetState(ServerState::RUNNING);
    m_statistics.start_time = std::chrono::steady_clock::now();
    
    std::ostringstream oss;
    oss << "Server started successfully on " << m_config.bind_address 
        << ":" << m_config.port << " (max connections: " << m_config.max_connections << ")";
    EmitEvent(ServerState::RUNNING, oss.str());

    return network::NetworkResult::SUCCESS;
}

void ServerCore::Stop() {
    if (m_state.load() == ServerState::STOPPED || m_state.load() == ServerState::STOPPING) {
        return;
    }

    SetState(ServerState::STOPPING);
    EmitEvent(ServerState::STOPPING, "Server shutting down...");

    // Signal shutdown
    m_shutdown_requested = true;

    // Disconnect all clients
    if (m_session_manager) {
        DisconnectAllClients("Server shutdown");
    }

    // Stop session manager
    if (m_session_manager) {
        m_session_manager->Stop();
    }

    // Shutdown worker threads
    ShutdownWorkerThreads();

    // Close listening socket
    if (m_listen_socket != 0) {
        m_network->CloseSocket(m_listen_socket);
        m_listen_socket = 0;
    }

    // Shutdown network
    ShutdownNetwork();

    SetState(ServerState::STOPPED);
    EmitEvent(ServerState::STOPPED, "Server stopped");
}

void ServerCore::Run() {
    if (!IsRunning()) {
        return;
    }

    EmitEvent(ServerState::RUNNING, "Entering main server loop");

    while (IsRunning() && !m_shutdown_requested) {
        ProcessEvents();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    EmitEvent(ServerState::RUNNING, "Exiting main server loop");
}

void ServerCore::RunWithTicks(size_t tick_rate) {
    if (!IsRunning()) {
        return;
    }

    auto tick_duration = std::chrono::milliseconds(1000 / tick_rate);
    auto last_tick = std::chrono::steady_clock::now();

    EmitEvent(ServerState::RUNNING, "Entering ticked server loop (TPS: " + 
             std::to_string(tick_rate) + ")");

    while (IsRunning() && !m_shutdown_requested) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = now - last_tick;

        if (elapsed >= tick_duration) {
            ProcessEvents();
            UpdateStatistics();
            last_tick = now;
        } else {
            auto sleep_time = tick_duration - elapsed;
            std::this_thread::sleep_for(sleep_time);
        }
    }

    EmitEvent(ServerState::RUNNING, "Exiting ticked server loop");
}

size_t ServerCore::ProcessEvents() {
    if (!IsRunning()) {
        return 0;
    }

    size_t events_processed = 0;

    // Process network events
    events_processed += m_network->ProcessCompletions(0);

    // Process session events
    if (m_session_manager) {
        m_session_manager->ProcessAllSessions();
    }

    return events_processed;
}

void ServerCore::DisconnectAllClients(const std::string& reason) {
    if (m_session_manager) {
        size_t disconnected = m_session_manager->DisconnectAllSessions(
            DisconnectReason::SERVER_SHUTDOWN, reason);
        
        if (disconnected > 0) {
            EmitEvent(ServerState::RUNNING, "Disconnected " + std::to_string(disconnected) + 
                     " clients: " + reason);
        }
    }
}

network::NetworkResult ServerCore::InitializeNetwork() {
    network::NetworkConfig net_config;
    net_config.queue_depth = m_config.io_queue_depth;
    net_config.enable_nodelay = m_config.enable_tcp_nodelay;
    net_config.enable_keepalive = m_config.enable_keepalive;

    return m_network->Initialize(net_config);
}

void ServerCore::InitializeWorkerThreads() {
    size_t thread_count = m_config.worker_threads;
    if (thread_count == 0) {
        thread_count = std::min(std::thread::hardware_concurrency(), 
                               static_cast<unsigned int>(MAX_WORKER_THREADS));
        if (thread_count == 0) {
            thread_count = DEFAULT_WORKER_THREADS;
        }
    }

    thread_count = std::min(thread_count, static_cast<size_t>(MAX_WORKER_THREADS));

    EmitEvent(ServerState::STARTING, "Starting " + std::to_string(thread_count) + " worker threads");

    m_worker_threads.reserve(thread_count);
    for (size_t i = 0; i < thread_count; ++i) {
        m_worker_threads.emplace_back(&ServerCore::WorkerThreadMain, this, i);
    }
}

void ServerCore::ShutdownWorkerThreads() {
    // Wait for all worker threads to finish
    for (auto& thread : m_worker_threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    m_worker_threads.clear();
}

void ServerCore::ShutdownNetwork() {
    if (m_network) {
        m_network->Shutdown();
    }
}

void ServerCore::StartAcceptLoop() {
    // Start async accept operation
    m_network->AsyncAccept(m_listen_socket, 
        [this](network::NetworkResult result, network::SocketType new_socket) {
            OnAcceptComplete(result, new_socket);
        });
}

void ServerCore::HandleNewConnection(network::SocketType new_socket) {
    if (!IsRunning()) {
        m_network->CloseSocket(new_socket);
        return;
    }

    // Check connection limits
    if (m_session_manager->IsAtCapacity()) {
        m_network->CloseSocket(new_socket);
        m_statistics.failed_connections++;
        return;
    }

    // Get client IP for limiting (simplified - would need proper implementation)
    std::string client_ip = "unknown";

    // Create new session
    auto session = m_session_manager->CreateSession(new_socket, client_ip);
    if (session) {
        m_statistics.total_connections++;
        m_statistics.active_connections++;
        
        size_t current_connections = m_statistics.active_connections.load();
        size_t peak = m_statistics.peak_connections.load();
        if (current_connections > peak) {
            m_statistics.peak_connections = current_connections;
        }

        EmitEvent(ServerState::RUNNING, "New client connected (total: " + 
                 std::to_string(current_connections) + ")");
    } else {
        m_network->CloseSocket(new_socket);
        m_statistics.failed_connections++;
    }
}

void ServerCore::OnAcceptComplete(network::NetworkResult result, network::SocketType new_socket) {
    if (result == network::NetworkResult::SUCCESS) {
        HandleNewConnection(new_socket);
    } else {
        m_statistics.failed_connections++;
        if (result != network::NetworkResult::ERROR_CONNECTION_CLOSED) {
            // Log error but continue accepting
            EmitEvent(ServerState::RUNNING, "Accept error: " + 
                     std::string(network::NetworkResultToString(result)));
        }
    }

    // Continue accepting if server is still running
    if (IsRunning() && !m_shutdown_requested) {
        StartAcceptLoop();
    }
}

void ServerCore::WorkerThreadMain(size_t thread_id) {
    std::ostringstream oss;
    oss << "Worker thread " << thread_id << " started";
    EmitEvent(ServerState::RUNNING, oss.str());

    while (IsRunning() && !m_shutdown_requested) {
        // Process network completions
        size_t events = m_network->ProcessCompletions(10); // 10ms timeout
        
        m_statistics.operations_processed += events;

        // Brief sleep if no events
        if (events == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    oss.str("");
    oss << "Worker thread " << thread_id << " stopped";
    EmitEvent(ServerState::RUNNING, oss.str());
}

void ServerCore::UpdateStatistics() {
    // Update active connections from session manager
    if (m_session_manager) {
        m_statistics.active_connections = m_session_manager->GetSessionCount();
        
        // Get network statistics
        size_t net_active, net_pending;
        uint64_t net_sent, net_received;
        m_network->GetStatistics(net_active, net_pending, net_sent, net_received);
        
        m_statistics.bytes_sent = net_sent;
        m_statistics.bytes_received = net_received;
    }
}

void ServerCore::EmitEvent(ServerState state, const std::string& message) {
    if (m_event_callback) {
        m_event_callback(state, message);
    } else {
        // Use spdlog for default logging
        std::string state_str;
        spdlog::level::level_enum log_level = spdlog::level::info;
        
        switch (state) {
            case ServerState::STARTING: 
                state_str = "STARTING";
                log_level = spdlog::level::info;
                break;
            case ServerState::RUNNING: 
                state_str = "RUNNING";
                log_level = spdlog::level::info;
                break;
            case ServerState::STOPPING: 
                state_str = "STOPPING";
                log_level = spdlog::level::warn;
                break;
            case ServerState::STOPPED: 
                state_str = "STOPPED";
                log_level = spdlog::level::info;
                break;
            case ServerState::FAILED: 
                state_str = "FAILED";
                log_level = spdlog::level::err;
                break;
        }
        
        spdlog::log(log_level, "[{}] {}", state_str, message);
    }
}

void ServerCore::SetState(ServerState new_state) {
    ServerState old_state = m_state.exchange(new_state);
    if (old_state != new_state) {
        // State changed
    }
}

} // namespace server
} // namespace parallelstone