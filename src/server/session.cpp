/**
 * @file session.cpp
 * @brief Implementation of client session management
 * @details This file implements the Session class for handling individual
 *          client connections and Minecraft protocol communication.
 * @author @logpacket (https://github.com/logpacket)
 * @version 1.0.0
 * @date 2025-07-16
 */

#include "server/session.hpp"
#include "server/session_manager.hpp"
#include "protocol/dispatcher.hpp"
#include "network/packet_view.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <spdlog/spdlog.h>
#include <random>
#include <algorithm>
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

Session::Session() 
    : m_socket(0), 
      m_session_manager(*reinterpret_cast<SessionManager*>(nullptr)), 
      m_state(SessionState::CONNECTING), 
      m_is_sending(false), 
      m_dispatcher(::protocol::GetPacketDispatcher()) {
    // Default constructor for mocking
}

Session::Session(parallelstone::network::SocketType socket, SessionManager& manager)
    : m_socket(socket)
    , m_session_manager(manager)
    , m_state(SessionState::CONNECTING)
    , m_is_sending(false)
    , m_dispatcher(::protocol::GetPacketDispatcher()) {
    
    // Initialize session info
    m_info.session_id = GenerateSessionId();
    m_info.connect_time = std::chrono::steady_clock::now();
    m_info.last_activity = m_info.connect_time;
    m_info.protocol_version = 0; // Will be set during handshake
    
    // Initialize session from socket
    InitializeFromSocket();
}

Session::~Session() {
    Cleanup();
}

void Session::Start() {
    if (GetState() != SessionState::CONNECTING) {
        return;
    }
    
    TransitionToState(SessionState::HANDSHAKING);
    
    // Start receiving data
    StartReceive();
}

void Session::Disconnect(DisconnectReason reason, const std::string& message) {
    if (GetState() == SessionState::DISCONNECTED || GetState() == SessionState::DISCONNECTING) {
        return;
    }
    
    TransitionToState(SessionState::DISCONNECTING);
    
    // TODO: Send disconnect packet with message if needed
    
    // Close socket
    m_session_manager.GetNetworkCore()->CloseSocket(m_socket);
    
    // Notify disconnect callback
    if (m_disconnect_callback) {
        m_disconnect_callback(shared_from_this(), reason);
    }
    
    TransitionToState(SessionState::DISCONNECTED);
}

void Session::Send(network::Buffer& packet) {
    if (GetState() == SessionState::DISCONNECTED || GetState() == SessionState::DISCONNECTING) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(m_outgoing_mutex);
    
    // Check queue size limit
    if (m_outgoing_queue.size() >= MAX_QUEUED_PACKETS) {
        return;
    }
    
    // Move packet to queue (zero-copy)
    m_outgoing_queue.push(std::move(packet));
    m_info.packets_sent++;
    
    // Start sending if not already sending
    if (!m_is_sending.load()) {
        StartSend();
    }
}

void Session::ProcessEvents() {
    if (GetState() == SessionState::DISCONNECTED) {
        return;
    }
    
    UpdateActivity();
    
    // Check for timeout
    if (HasTimedOut(std::chrono::seconds(30))) { // 30 second timeout
        Disconnect(DisconnectReason::TIMEOUT, "Session timeout");
        return;
    }
    
    // Process any pending outgoing data
    if (!m_outgoing_queue.empty() && !m_is_sending.load()) {
        StartSend();
    }
}

void Session::FlushOutgoing() {
    if (m_outgoing_queue.empty() || m_is_sending.load()) {
        return;
    }
    
    StartSend();
}

void Session::OnDataReceived(network::NetworkResult result, size_t bytes_received) {
    if (result != network::NetworkResult::SUCCESS) {
        HandleNetworkError(result);
        return;
    }
    
    if (bytes_received == 0) {
        // Connection closed by peer
        Disconnect(DisconnectReason::CLIENT_DISCONNECT, "Connection closed by client");
        return;
    }
    
    // Update statistics
    m_info.bytes_received += bytes_received;
    UpdateActivity();
    
    // Data was already written directly to m_receive_buffer by network layer
    // Advance the write position to reflect received data
    try {
        m_receive_buffer.advance_write_position(bytes_received);
    } catch (const std::exception& e) {
        spdlog::error("Session {}: Failed to advance buffer write position: {}", GetSessionId(), e.what());
        Disconnect(DisconnectReason::INTERNAL_ERROR, "Buffer error");
        return;
    }
    
    // Process all complete packets in buffer
    ProcessReceivedPackets();
    
    // Continue receiving if still connected
    if (IsActive()) {
        StartReceive();
    }
}

void Session::OnDataSent(network::NetworkResult result, size_t bytes_sent) {
    m_is_sending = false;
    
    if (result != network::NetworkResult::SUCCESS) {
        HandleNetworkError(result);
        return;
    }
    
    // Update statistics
    m_info.bytes_sent += bytes_sent;
    
    // Send next packet if available
    std::lock_guard<std::mutex> lock(m_outgoing_mutex);
    if (!m_outgoing_queue.empty()) {
        m_outgoing_queue.pop(); // Remove sent packet
        
        if (!m_outgoing_queue.empty()) {
            StartSend(); // Send next packet
        }
    }
}

void Session::ProcessReceivedPackets() {
    try {
        // Process all complete packets in the receive buffer
        while (m_receive_buffer.has_complete_packet()) {
            // Read packet length first
            auto packet_length_result = m_receive_buffer.peek_packet_length();
            if (!packet_length_result.has_value()) {
                // Not enough data for complete packet length
                break;
            }
            
            int32_t packet_length = packet_length_result.value();
            
            // Validate packet length
            if (packet_length <= 0 || packet_length > MAX_PACKET_SIZE) {
                spdlog::error("Session {}: Invalid packet length: {}", GetSessionId(), packet_length);
                Disconnect(DisconnectReason::PROTOCOL_ERROR, "Invalid packet length");
                return;
            }
            
            // Check if we have the complete packet data
            if (!m_receive_buffer.has_bytes_available(packet_length)) {
                // Wait for more data
                break;
            }
            
            // Skip the length prefix (already validated)
            m_receive_buffer.skip_packet_length();
            
            // Create PacketView for the packet data (SAFE: buffer remains valid during processing)
            network::PacketView packet_view(m_receive_buffer.current_read_ptr(), packet_length);
            
            // Extract packet ID safely
            int32_t packet_id;
            try {
                packet_id = packet_view.read_varint();
            } catch (const std::exception& e) {
                spdlog::error("Session {}: Failed to read packet ID: {}", GetSessionId(), e.what());
                Disconnect(DisconnectReason::PROTOCOL_ERROR, "Invalid packet ID");
                return;
            }
            
            // Update statistics
            m_info.packets_received++;
            
            spdlog::debug("Session {}: Processing packet ID 0x{:02X}, length: {}, state: {}", 
                         GetSessionId(), packet_id, packet_length, static_cast<int>(GetState()));
            
            // Dispatch packet immediately (synchronous - safe for PacketView)
            bool handled = false;
            try {
                handled = m_dispatcher.DispatchPacket(GetState(), static_cast<uint8_t>(packet_id), 
                                                    shared_from_this(), packet_view);
            } catch (const std::exception& e) {
                spdlog::error("Session {}: Exception during packet dispatch: {}", GetSessionId(), e.what());
                handled = false;
            }
            
            if (!handled) {
                spdlog::warn("Session {}: Unhandled packet ID 0x{:02X} in state {}", 
                           GetSessionId(), packet_id, static_cast<int>(GetState()));
            }
            
            // Notify packet callback if set
            if (m_packet_callback) {
                try {
                    m_packet_callback(shared_from_this(), packet_id, packet_view);
                } catch (const std::exception& e) {
                    spdlog::error("Session {}: Exception in packet callback: {}", GetSessionId(), e.what());
                }
            }
            
            // Remove processed packet from buffer
            m_receive_buffer.advance_read_position(packet_length);
            
            // PacketView goes out of scope here - safe since we don't store it
        }
        
    } catch (const std::exception& e) {
        spdlog::error("Session {}: Exception during packet processing: {}", GetSessionId(), e.what());
        Disconnect(DisconnectReason::INTERNAL_ERROR, "Packet processing error");
    }
}

void Session::TransitionToState(SessionState new_state) {
    SessionState old_state = m_state.exchange(new_state);
    
    if (old_state != new_state) {
        spdlog::info("Session {}: State transition {} -> {}", 
                     GetSessionId(), static_cast<int>(old_state), static_cast<int>(new_state));
        
        // Notify state change callback
        if (m_state_change_callback) {
            try {
                m_state_change_callback(shared_from_this(), old_state, new_state);
            } catch (const std::exception& e) {
                spdlog::error("Session {}: Exception in state change callback: {}", GetSessionId(), e.what());
            }
        }
    }
}

void Session::InitializeFromSocket() {
    // Get client address information
    struct sockaddr_storage addr;
    socklen_t addr_len = sizeof(addr);
    
#ifdef _WIN32
    SOCKET sock = static_cast<SOCKET>(m_socket);
    if (getpeername(sock, reinterpret_cast<sockaddr*>(&addr), &addr_len) == 0) {
#else
    int sock = static_cast<int>(m_socket);
    if (getpeername(sock, reinterpret_cast<sockaddr*>(&addr), &addr_len) == 0) {
#endif
        if (addr.ss_family == AF_INET) {
            auto* addr_in = reinterpret_cast<sockaddr_in*>(&addr);
            char ip_str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &addr_in->sin_addr, ip_str, INET_ADDRSTRLEN);
            m_info.client_ip = ip_str;
            m_info.client_port = ntohs(addr_in->sin_port);
        }
    }
    
    if (m_info.client_ip.empty()) {
        m_info.client_ip = "unknown";
        m_info.client_port = 0;
    }
    
    spdlog::info("Session {}: Initialized from {}:{}", GetSessionId(), m_info.client_ip, m_info.client_port);
}

void Session::StartReceive() {
    if (GetState() == SessionState::DISCONNECTED || GetState() == SessionState::DISCONNECTING) {
        return;
    }
    
    // Ensure buffer has space for incoming data
    if (m_receive_buffer.writable_bytes() == 0) {
        // Buffer is full - compact it to reclaim space from processed packets
        m_receive_buffer.compact();
        
        if (m_receive_buffer.writable_bytes() == 0) {
            spdlog::error("Session {}: Receive buffer full after compaction, disconnecting", GetSessionId());
            Disconnect(DisconnectReason::INTERNAL_ERROR, "Buffer overflow");
            return;
        }
    }
    
    // Use m_receive_buffer directly for network operations (zero-copy approach)
    auto self = shared_from_this();
    m_session_manager.GetNetworkCore()->AsyncReceive(m_socket, m_receive_buffer,
        [self](network::NetworkResult result, size_t bytes) {
            self->OnDataReceived(result, bytes);
        });
}

void Session::StartSend() {
    if (GetState() == SessionState::DISCONNECTED || GetState() == SessionState::DISCONNECTING) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(m_outgoing_mutex);
    if (m_outgoing_queue.empty() || m_is_sending.load()) {
        return;
    }
    
    m_is_sending = true;
    
    // Send the front packet directly (zero-copy)
    auto& packet = m_outgoing_queue.front();
    auto self = shared_from_this();
    m_session_manager.GetNetworkCore()->AsyncSend(m_socket, packet,
        [self](network::NetworkResult result, size_t bytes) {
            self->OnDataSent(result, bytes);
        });
}

std::string Session::GenerateSessionId() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    
    std::ostringstream oss;
    oss << "session_";
    for (int i = 0; i < 16; ++i) {
        oss << std::hex << dis(gen);
    }
    
    return oss.str();
}

void Session::HandleNetworkError(network::NetworkResult result) {
    DisconnectReason reason = DisconnectReason::NETWORK_ERROR;
    
    switch (result) {
        case network::NetworkResult::ERROR_CONNECTION_CLOSED:
            reason = DisconnectReason::CLIENT_DISCONNECT;
            break;
        case network::NetworkResult::ERROR_INVALID_ARGUMENT:
            reason = DisconnectReason::PROTOCOL_ERROR;
            break;
        default:
            reason = DisconnectReason::NETWORK_ERROR;
            break;
    }
    
    Disconnect(reason, "Network error: " + std::string(network::NetworkResultToString(result)));
}

void Session::Cleanup() {
    // Clear outgoing packet queue
    std::lock_guard<std::mutex> lock(m_outgoing_mutex);
    while (!m_outgoing_queue.empty()) {
        m_outgoing_queue.pop();
    }
}

const std::string& Session::GetSessionId() const {
    return m_info.session_id;
}

const std::string& Session::GetRemoteAddress() const {
    return m_info.client_ip;
}

uint16_t Session::GetRemotePort() const {
    return m_info.client_port;
}

void Session::SetNextState(SessionState state) {
    TransitionToState(state);
}

void Session::SendDisconnect(const std::string& reason) {
    Disconnect(DisconnectReason::PROTOCOL_ERROR, reason);
}

// Utility functions for enum to string conversion
const char* SessionStateToString(SessionState state) {
    switch (state) {
        case SessionState::CONNECTING: return "CONNECTING";
        case SessionState::HANDSHAKING: return "HANDSHAKING";
        case SessionState::STATUS: return "STATUS";
        case SessionState::LOGIN: return "LOGIN";
        case SessionState::CONFIGURATION: return "CONFIGURATION";
        case SessionState::PLAY: return "PLAY";
        case SessionState::DISCONNECTED: return "DISCONNECTED";
        default: return "UNKNOWN";
    }
}

const char* DisconnectReasonToString(DisconnectReason reason) {
    switch (reason) {
        case DisconnectReason::CLIENT_DISCONNECT: return "CLIENT_DISCONNECT";
        case DisconnectReason::SERVER_SHUTDOWN: return "SERVER_SHUTDOWN";
        case DisconnectReason::NETWORK_ERROR: return "NETWORK_ERROR";
        case DisconnectReason::PROTOCOL_ERROR: return "PROTOCOL_ERROR";
        case DisconnectReason::TIMEOUT: return "TIMEOUT";
        case DisconnectReason::AUTHENTICATION_FAILED: return "AUTHENTICATION_FAILED";
        case DisconnectReason::BANNED: return "BANNED";
        case DisconnectReason::SERVER_FULL: return "SERVER_FULL";
        default: return "UNKNOWN";
    }
}

} // namespace server
} // namespace parallelstone