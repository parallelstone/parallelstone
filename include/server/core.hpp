/**
 * @file core.hpp
 * @brief ParellelStone server core implementation
 * @details This header defines the main server core that manages the overall
 *          server lifecycle, network operations, and client connections.
 *          It integrates the network layer with session management and
 *          Minecraft protocol handling.
 * @author @logpacket (https://github.com/logpacket)
 * @version 1.0.0
 * @date 2025-07-16
 */

#pragma once
#ifndef PARALLELSTONE_SERVER_CORE_HPP
#define PARALLELSTONE_SERVER_CORE_HPP

#include <memory>
#include <string>
#include <atomic>
#include <thread>
#include <chrono>
#include <functional>
#include <unordered_set>
#include <cstddef>
#include <cstdint>
#include <vector>
#include "network/core.hpp"
#include "session_manager.hpp"

/**
 * @namespace parallelstone::server
 * @brief Server-side components for ParellelStone Minecraft server
 */
namespace parallelstone {
namespace server {

/**
 * @enum ServerState
 * @brief Enumeration of possible server states
 * @details Defines the lifecycle states of the server for proper
 *          state management and transitions.
 * @note Uses FAILED instead of ERROR to avoid conflicts with Windows API macros
 */
enum class ServerState {
    STOPPED,        ///< Server is not running
    STARTING,       ///< Server is in the process of starting
    RUNNING,        ///< Server is fully operational
    STOPPING,       ///< Server is in the process of stopping
    FAILED          ///< Server encountered an error and failed to operate
};

/**
 * @struct ServerConfig
 * @brief Configuration structure for server operations
 * @details Contains all configurable parameters for the server including
 *          network settings, performance tuning, and operational parameters.
 */
struct ServerConfig {
    // Network configuration
    std::string bind_address = "0.0.0.0";      ///< Address to bind to (default: all interfaces)
    uint16_t port = 25565;                      ///< Port to listen on (default: Minecraft port)
    size_t max_connections = 1000;              ///< Maximum concurrent connections
    
    // Performance settings
    size_t worker_threads = 0;                  ///< Number of worker threads (0 = auto-detect)
    size_t io_queue_depth = 256;                ///< I/O queue depth for async operations
    bool enable_tcp_nodelay = true;             ///< Disable Nagle's algorithm for lower latency
    bool enable_keepalive = true;               ///< Enable TCP keepalive
    
    // Timeouts and limits
    std::chrono::milliseconds accept_timeout{5000};        ///< Accept operation timeout
    std::chrono::milliseconds session_timeout{30000};      ///< Session idle timeout
    std::chrono::milliseconds shutdown_timeout{10000};     ///< Graceful shutdown timeout
    size_t max_packet_size = 2097151;                      ///< Maximum packet size (2MB - 1)
    
    // Protocol settings
    int32_t protocol_version = 765;             ///< Minecraft protocol version (1.20.4)
    std::string server_name = "ParellelStone"; ///< Server name for status response
    std::string motd = "A ParellelStone Minecraft Server"; ///< Message of the day
    size_t max_players = 100;                   ///< Maximum players allowed
    bool online_mode = true;                    ///< Enable Mojang authentication
    
    /**
     * @brief Default constructor with standard configuration values
     */
    ServerConfig() = default;
    
    /**
     * @brief Set bind address for the server
     * @param address IP address to bind to
     * @return Reference to this config for method chaining
     */
    ServerConfig& SetBindAddress(const std::string& address) {
        bind_address = address;
        return *this;
    }
    
    /**
     * @brief Set listening port for the server
     * @param server_port Port number to listen on
     * @return Reference to this config for method chaining
     */
    ServerConfig& SetPort(uint16_t server_port) {
        port = server_port;
        return *this;
    }
    
    /**
     * @brief Set maximum concurrent connections
     * @param max_conn Maximum number of concurrent connections
     * @return Reference to this config for method chaining
     */
    ServerConfig& SetMaxConnections(size_t max_conn) {
        max_connections = max_conn;
        return *this;
    }
    
    /**
     * @brief Set number of worker threads
     * @param threads Number of worker threads (0 for auto-detect)
     * @return Reference to this config for method chaining
     */
    ServerConfig& SetWorkerThreads(size_t threads) {
        worker_threads = threads;
        return *this;
    }
    
    /**
     * @brief Set server message of the day
     * @param message MOTD string
     * @return Reference to this config for method chaining
     */
    ServerConfig& SetMotd(const std::string& message) {
        motd = message;
        return *this;
    }
};

/**
 * @struct ServerStatistics
 * @brief Runtime statistics for server monitoring
 * @details Provides comprehensive statistics for server performance
 *          monitoring and debugging purposes.
 */
struct ServerStatistics {
    // Connection statistics
    std::atomic<size_t> active_connections{0};      ///< Current active connections
    std::atomic<size_t> total_connections{0};       ///< Total connections since start
    std::atomic<size_t> failed_connections{0};      ///< Failed connection attempts
    
    // Traffic statistics
    std::atomic<uint64_t> bytes_sent{0};            ///< Total bytes sent
    std::atomic<uint64_t> bytes_received{0};        ///< Total bytes received
    std::atomic<uint64_t> packets_sent{0};          ///< Total packets sent
    std::atomic<uint64_t> packets_received{0};      ///< Total packets received
    
    // Performance statistics
    std::atomic<uint64_t> operations_processed{0};  ///< Total I/O operations processed
    std::chrono::steady_clock::time_point start_time; ///< Server start time
    std::atomic<size_t> peak_connections{0};        ///< Peak concurrent connections
    
    /**
     * @brief Get server uptime in seconds
     * @return Server uptime duration
     */
    std::chrono::seconds GetUptime() const {
        return std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - start_time);
    }
    
    /**
     * @brief Reset all statistics counters
     */
    void Reset() {
        active_connections = 0;
        total_connections = 0;
        failed_connections = 0;
        bytes_sent = 0;
        bytes_received = 0;
        packets_sent = 0;
        packets_received = 0;
        operations_processed = 0;
        peak_connections = 0;
        start_time = std::chrono::steady_clock::now();
    }
};

// Forward declarations
class Session;
class SessionManager;

/**
 * @class ServerCore
 * @brief Main server core managing the entire server lifecycle
 * @details The ServerCore class is the central component that orchestrates
 *          all server operations including network management, session handling,
 *          and protocol processing. It provides a high-level interface for
 *          server control and monitoring.
 * 
 * @example
 * @code
 * ServerConfig config;
 * config.SetPort(25565).SetMaxConnections(500);
 * 
 * ServerCore server(config);
 * if (server.Start() == network::NetworkResult::SUCCESS) {
 *     server.Run(); // Blocks until stopped
 * }
 * @endcode
 */
class ServerCore {
public:
    /**
     * @brief Constructor with configuration
     * @param config Server configuration parameters
     */
    explicit ServerCore(const ServerConfig& config = ServerConfig{});
    
    /**
     * @brief Destructor - ensures proper cleanup
     */
    ~ServerCore();
    
    // Non-copyable and non-movable
    ServerCore(const ServerCore&) = delete;
    ServerCore& operator=(const ServerCore&) = delete;
    ServerCore(ServerCore&&) = delete;
    ServerCore& operator=(ServerCore&&) = delete;
    
    /**
     * @brief Start the server
     * @return NetworkResult indicating success or failure
     * @details Initializes network subsystem, starts listening for connections,
     *          and begins processing client requests.
     */
    network::NetworkResult Start();
    
    /**
     * @brief Stop the server gracefully
     * @details Stops accepting new connections, closes existing sessions,
     *          and shuts down the network subsystem.
     */
    void Stop();
    
    /**
     * @brief Run the server main loop (blocking)
     * @details Processes network events and manages sessions until stopped.
     *          This method blocks the calling thread.
     */
    void Run();
    
    /**
     * @brief Run the server main loop with custom tick rate
     * @param tick_rate Number of ticks per second
     * @details Processes events at specified rate for precise timing control
     */
    void RunWithTicks(size_t tick_rate = 20);
    
    /**
     * @brief Process server events (non-blocking)
     * @return Number of events processed
     * @details Processes pending network events and session updates.
     *          Returns immediately after processing available events.
     */
    size_t ProcessEvents();
    
    /**
     * @brief Get current server state
     * @return Current ServerState
     */
    ServerState GetState() const { return m_state.load(); }
    
    /**
     * @brief Check if server is running
     * @return true if server is in RUNNING state
     */
    bool IsRunning() const { return GetState() == ServerState::RUNNING; }
    
    /**
     * @brief Get server configuration
     * @return Current server configuration
     */
    const ServerConfig& GetConfig() const { return m_config; }
    
    /**
     * @brief Get server statistics
     * @return Current server statistics
     */
    const ServerStatistics& GetStatistics() const { return m_statistics; }
    
    /**
     * @brief Get session manager instance
     * @return Reference to session manager
     */
    SessionManager& GetSessionManager() { return *m_session_manager; }
    
    /**
     * @brief Get session manager instance (const)
     * @return Const reference to session manager
     */
    const SessionManager& GetSessionManager() const { return *m_session_manager; }
    
    /**
     * @brief Set custom event callback for server events
     * @param callback Callback function for server events
     * @details Allows external monitoring of server state changes
     */
    void SetEventCallback(std::function<void(ServerState, const std::string&)> callback) {
        m_event_callback = std::move(callback);
    }
    
    /**
     * @brief Force disconnect all clients
     * @param reason Disconnect reason message
     * @details Immediately disconnects all active sessions
     */
    void DisconnectAllClients(const std::string& reason = "Server shutdown");
    
    /**
     * @brief Get number of active connections
     * @return Current number of active connections
     */
    size_t GetActiveConnectionCount() const {
        return m_statistics.active_connections.load();
    }

private:
    /**
     * @brief Initialize network subsystem
     * @return NetworkResult indicating success or failure
     */
    network::NetworkResult InitializeNetwork();
    
    /**
     * @brief Initialize worker threads
     * @details Sets up worker threads for parallel processing
     */
    void InitializeWorkerThreads();
    
    /**
     * @brief Shutdown network subsystem
     */
    void ShutdownNetwork();
    
    /**
     * @brief Shutdown worker threads
     */
    void ShutdownWorkerThreads();
    
    /**
     * @brief Handle new client connection
     * @param new_socket Socket for new connection
     */
    void HandleNewConnection(network::SocketType new_socket);
    
    /**
     * @brief Start accepting new connections
     */
    void StartAcceptLoop();
    
    /**
     * @brief Handle connection acceptance
     * @param result Result of accept operation
     * @param new_socket New socket handle or error
     */
    void OnAcceptComplete(network::NetworkResult result, network::SocketType new_socket);
    
    /**
     * @brief Worker thread main function
     * @param thread_id Unique identifier for this worker thread
     */
    void WorkerThreadMain(size_t thread_id);
    
    /**
     * @brief Update server statistics
     */
    void UpdateStatistics();
    
    /**
     * @brief Emit server event
     * @param state New server state
     * @param message Event message
     */
    void EmitEvent(ServerState state, const std::string& message);
    
    /**
     * @brief Set server state atomically
     * @param new_state New server state
     */
    void SetState(ServerState new_state);
    
    // Configuration and state
    ServerConfig m_config;                              ///< Server configuration
    std::atomic<ServerState> m_state;                   ///< Current server state
    ServerStatistics m_statistics;                      ///< Runtime statistics
    
    // Network components
    std::shared_ptr<network::INetworkCore> m_network;   ///< Network implementation
    network::SocketType m_listen_socket;                ///< Listening socket
    
    // Session management
    std::unique_ptr<SessionManager> m_session_manager;  ///< Session manager instance
    
    // Threading
    std::vector<std::thread> m_worker_threads;          ///< Worker thread pool
    std::atomic<bool> m_shutdown_requested;             ///< Shutdown flag
    
    // Event handling
    std::function<void(ServerState, const std::string&)> m_event_callback; ///< External event callback
    
    // Constants
    static constexpr size_t DEFAULT_WORKER_THREADS = 4; ///< Default number of worker threads
    static constexpr size_t MAX_WORKER_THREADS = 64;    ///< Maximum number of worker threads
};

} // namespace server
} // namespace parallelstone

#endif // PARALLELSTONE_SERVER_CORE_HPP