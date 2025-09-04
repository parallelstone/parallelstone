/**
 * @file session_manager.hpp
 * @brief Session management for ParellelStone server
 * @details This header defines the SessionManager class that manages all
 *          active client sessions, handles session lifecycle, provides
 *          session lookup and monitoring capabilities, and coordinates
 *          session-related operations across the server.
 * @author @logpacket (https://github.com/logpacket)
 * @version 1.0.0
 * @date 2025-07-16
 */

#pragma once
#ifndef PARALLELSTONE_SERVER_SESSION_MANAGER_HPP
#define PARALLELSTONE_SERVER_SESSION_MANAGER_HPP

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <chrono>
#include <functional>
#include <thread>
#include <cstddef>
#include <cstdint>
#include <optional>
#include "session.hpp"
#include "network/core.hpp"

/**
 * @namespace parallelstone::server
 * @brief Server-side components for ParellelStone Minecraft server
 */
namespace parallelstone {
namespace server {

/**
 * @struct SessionManagerConfig
 * @brief Configuration for session manager
 * @details Contains parameters for session management including
 *          limits, timeouts, and operational settings.
 */
struct SessionManagerConfig {
    size_t max_sessions = 1000;                            ///< Maximum concurrent sessions
    std::chrono::milliseconds session_timeout{300000};     ///< Session timeout (5 minutes)
    std::chrono::milliseconds cleanup_interval{30000};     ///< Cleanup task interval (30 seconds)
    std::chrono::milliseconds heartbeat_interval{1000};    ///< Heartbeat check interval (1 second)
    size_t max_sessions_per_ip = 5;                        ///< Maximum sessions per IP address
    bool enable_ip_limiting = true;                        ///< Enable IP-based session limiting
    bool enable_auto_cleanup = true;                       ///< Enable automatic cleanup of dead sessions
    
    /**
     * @brief Default constructor with standard values
     */
    SessionManagerConfig() = default;
    
    /**
     * @brief Set maximum concurrent sessions
     * @param max_sessions Maximum number of sessions
     * @return Reference to this config for method chaining
     */
    SessionManagerConfig& SetMaxSessions(size_t max_sessions_count) {
        max_sessions = max_sessions_count;
        return *this;
    }
    
    /**
     * @brief Set session timeout duration
     * @param timeout Timeout duration in milliseconds
     * @return Reference to this config for method chaining
     */
    SessionManagerConfig& SetSessionTimeout(std::chrono::milliseconds timeout) {
        session_timeout = timeout;
        return *this;
    }
    
    /**
     * @brief Set cleanup interval
     * @param interval Cleanup interval in milliseconds
     * @return Reference to this config for method chaining
     */
    SessionManagerConfig& SetCleanupInterval(std::chrono::milliseconds interval) {
        cleanup_interval = interval;
        return *this;
    }
};

/**
 * @struct SessionManagerStatistics
 * @brief Runtime statistics for session manager
 * @details Provides comprehensive statistics for session management
 *          monitoring and performance analysis.
 */
struct SessionManagerStatistics {
    std::atomic<size_t> active_sessions{0};        ///< Current active sessions
    std::atomic<size_t> total_sessions{0};         ///< Total sessions created
    std::atomic<size_t> rejected_sessions{0};      ///< Sessions rejected due to limits
    std::atomic<size_t> timed_out_sessions{0};     ///< Sessions that timed out
    std::atomic<size_t> cleanup_runs{0};           ///< Number of cleanup operations performed
    std::atomic<size_t> peak_sessions{0};          ///< Peak concurrent sessions
    
    // Per-state counters
    std::atomic<size_t> handshaking_sessions{0};   ///< Sessions in handshaking state
    std::atomic<size_t> status_sessions{0};        ///< Sessions in status state
    std::atomic<size_t> login_sessions{0};         ///< Sessions in login state
    std::atomic<size_t> configuration_sessions{0}; ///< Sessions in configuration state
    std::atomic<size_t> play_sessions{0};          ///< Sessions in play state
    
    /**
     * @brief Reset all statistics counters
     */
    void Reset() {
        active_sessions = 0;
        total_sessions = 0;
        rejected_sessions = 0;
        timed_out_sessions = 0;
        cleanup_runs = 0;
        peak_sessions = 0;
        handshaking_sessions = 0;
        status_sessions = 0;
        login_sessions = 0;
        configuration_sessions = 0;
        play_sessions = 0;
    }
};

/**
 * @struct SessionQuery
 * @brief Query structure for session filtering and search
 * @details Provides flexible filtering options for session lookup
 *          and batch operations on sessions.
 */
struct SessionQuery {
    std::optional<SessionState> state;             ///< Filter by session state
    std::optional<std::string> player_name;       ///< Filter by player name
    std::optional<std::string> client_ip;         ///< Filter by client IP
    std::optional<std::chrono::seconds> min_duration; ///< Minimum session duration
    std::optional<std::chrono::seconds> max_idle;  ///< Maximum idle time
    
    /**
     * @brief Set state filter
     * @param session_state State to filter by
     * @return Reference to this query for method chaining
     */
    SessionQuery& WithState(SessionState session_state) {
        state = session_state;
        return *this;
    }
    
    /**
     * @brief Set player name filter
     * @param name Player name to filter by
     * @return Reference to this query for method chaining
     */
    SessionQuery& WithPlayerName(const std::string& name) {
        player_name = name;
        return *this;
    }
    
    /**
     * @brief Set client IP filter
     * @param ip IP address to filter by
     * @return Reference to this query for method chaining
     */
    SessionQuery& WithClientIP(const std::string& ip) {
        client_ip = ip;
        return *this;
    }
};

// Forward declarations
class ServerCore;

/**
 * @class SessionManager
 * @brief Manages all client sessions for the server
 * @details The SessionManager class provides centralized management of all
 *          client sessions including creation, tracking, cleanup, and
 *          coordination. It handles session limits, timeouts, and provides
 *          various lookup and monitoring capabilities.
 * 
 * @example
 * @code
 * SessionManagerConfig config;
 * config.SetMaxSessions(500).SetSessionTimeout(std::chrono::minutes(10));
 * 
 * SessionManager manager(config, network_core);
 * manager.Start();
 * 
 * auto session = manager.CreateSession(socket);
 * @endcode
 */
class SessionManager {
public:
    /**
     * @brief Constructor
     * @param config Session manager configuration
     * @param network_core Network implementation for session operations
     */
    SessionManager(const SessionManagerConfig& config, 
                   std::shared_ptr<network::INetworkCore> network_core);
    
    /**
     * @brief Destructor - ensures proper cleanup
     */
    ~SessionManager();
    
    // Non-copyable and non-movable
    SessionManager(const SessionManager&) = delete;
    SessionManager& operator=(const SessionManager&) = delete;
    SessionManager(SessionManager&&) = delete;
    SessionManager& operator=(SessionManager&&) = delete;
    
    /**
     * @brief Start the session manager
     * @details Initializes cleanup threads and begins session monitoring
     */
    void Start();
    
    /**
     * @brief Stop the session manager
     * @details Stops all sessions and cleanup threads gracefully
     */
    void Stop();
    
    /**
     * @brief Create new session for client connection
     * @param socket Network socket for the new client
     * @param client_ip Client IP address (for limiting)
     * @return Shared pointer to new session, nullptr if creation failed
     */
    std::shared_ptr<Session> CreateSession(network::SocketType socket, const std::string& client_ip = "");
    
    /**
     * @brief Remove session from management
     * @param session_id Unique session identifier
     * @return true if session was found and removed
     */
    bool RemoveSession(const std::string& session_id);
    
    /**
     * @brief Remove session from management
     * @param session Shared pointer to session
     * @return true if session was found and removed
     */
    bool RemoveSession(std::shared_ptr<Session> session);
    
    /**
     * @brief Get session by ID
     * @param session_id Unique session identifier
     * @return Shared pointer to session, nullptr if not found
     */
    std::shared_ptr<Session> GetSession(const std::string& session_id) const;
    
    /**
     * @brief Get session by player name
     * @param player_name Minecraft player name
     * @return Shared pointer to session, nullptr if not found
     */
    std::shared_ptr<Session> GetSessionByPlayer(const std::string& player_name) const;
    
    /**
     * @brief Get session by socket
     * @param socket Network socket handle
     * @return Shared pointer to session, nullptr if not found
     */
    std::shared_ptr<Session> GetSessionBySocket(network::SocketType socket) const;
    
    /**
     * @brief Get all active sessions
     * @return Vector of shared pointers to all active sessions
     */
    std::vector<std::shared_ptr<Session>> GetAllSessions() const;
    
    /**
     * @brief Find sessions matching query criteria
     * @param query Query criteria for filtering
     * @return Vector of sessions matching the criteria
     */
    std::vector<std::shared_ptr<Session>> FindSessions(const SessionQuery& query) const;
    
    /**
     * @brief Get sessions by client IP
     * @param client_ip IP address to search for
     * @return Vector of sessions from the specified IP
     */
    std::vector<std::shared_ptr<Session>> GetSessionsByIP(const std::string& client_ip) const;
    
    /**
     * @brief Broadcast packet to all sessions in specified state
     * @param packet_data Packet data to broadcast
     * @param packet_id Minecraft packet ID
     * @param target_state Target session state (default: PLAY)
     * @return Number of sessions the packet was sent to
     */
    size_t BroadcastPacket(const std::vector<uint8_t>& packet_data, int32_t packet_id,
                          SessionState target_state = SessionState::PLAY);
    
    /**
     * @brief Disconnect all sessions with specified reason
     * @param reason Disconnect reason
     * @param message Optional disconnect message
     * @return Number of sessions disconnected
     */
    size_t DisconnectAllSessions(DisconnectReason reason = DisconnectReason::SERVER_SHUTDOWN,
                                const std::string& message = "Server shutdown");
    
    /**
     * @brief Disconnect sessions by IP address
     * @param client_ip IP address to disconnect
     * @param reason Disconnect reason
     * @param message Optional disconnect message
     * @return Number of sessions disconnected
     */
    size_t DisconnectSessionsByIP(const std::string& client_ip,
                                 DisconnectReason reason = DisconnectReason::BANNED,
                                 const std::string& message = "Disconnected");
    
    /**
     * @brief Process all session events
     * @details Calls ProcessEvents() on all active sessions
     */
    void ProcessAllSessions();
    
    /**
     * @brief Cleanup timed out and dead sessions
     * @return Number of sessions cleaned up
     */
    size_t CleanupSessions();
    
    /**
     * @brief Get current session count
     * @return Number of active sessions
     */
    size_t GetSessionCount() const {
        std::shared_lock<std::shared_mutex> lock(m_sessions_mutex);
        return m_sessions.size();
    }
    
    /**
     * @brief Check if session limit has been reached
     * @return true if at maximum session capacity
     */
    bool IsAtCapacity() const {
        return GetSessionCount() >= m_config.max_sessions;
    }
    
    /**
     * @brief Check if IP has reached session limit
     * @param client_ip IP address to check
     * @return true if IP has reached maximum sessions
     */
    bool IsIPAtLimit(const std::string& client_ip) const;
    
    /**
     * @brief Get session manager configuration
     * @return Current configuration
     */
    const SessionManagerConfig& GetConfig() const { return m_config; }
    
    /**
     * @brief Get session manager statistics
     * @return Current statistics
     */
    const SessionManagerStatistics& GetStatistics() const { return m_statistics; }
    
    /**
     * @brief Get network core instance
     * @return Shared pointer to network implementation
     */
    std::shared_ptr<network::INetworkCore> GetNetworkCore() const { return m_network_core; }
    
    /**
     * @brief Set session event callback
     * @param callback Function to call for session events
     */
    void SetSessionEventCallback(std::function<void(std::shared_ptr<Session>, const std::string&)> callback) {
        m_session_event_callback = std::move(callback);
    }
    
    /**
     * @brief Update statistics counters
     * @details Recalculates statistics based on current session states
     */
    void UpdateStatistics();

private:
    /**
     * @brief Handle session disconnect
     * @param session Session that disconnected
     * @param reason Disconnect reason
     */
    void OnSessionDisconnect(std::shared_ptr<Session> session, DisconnectReason reason);
    
    /**
     * @brief Handle session state change
     * @param session Session that changed state
     * @param old_state Previous state
     * @param new_state New state
     */
    void OnSessionStateChange(std::shared_ptr<Session> session, SessionState old_state, SessionState new_state);
    
    /**
     * @brief Cleanup thread main function
     * @details Periodically cleans up timed out sessions
     */
    void CleanupThreadMain();
    
    /**
     * @brief Heartbeat thread main function
     * @details Periodically processes session events and updates statistics
     */
    void HeartbeatThreadMain();
    
    /**
     * @brief Generate unique session ID
     * @return Unique identifier string
     */
    std::string GenerateSessionId();
    
    /**
     * @brief Add session to internal tracking
     * @param session Session to add
     */
    void AddSessionInternal(std::shared_ptr<Session> session);
    
    /**
     * @brief Remove session from internal tracking
     * @param session_id Session ID to remove
     * @return true if session was found and removed
     */
    bool RemoveSessionInternal(const std::string& session_id);
    
    /**
     * @brief Emit session event
     * @param session Session that triggered the event
     * @param event_message Event description
     */
    void EmitSessionEvent(std::shared_ptr<Session> session, const std::string& event_message);
    
    // Configuration and statistics
    SessionManagerConfig m_config;                 ///< Session manager configuration
    SessionManagerStatistics m_statistics;         ///< Runtime statistics
    
    // Network core
    std::shared_ptr<network::INetworkCore> m_network_core; ///< Network implementation
    
    // Session storage
    mutable std::shared_mutex m_sessions_mutex;    ///< Mutex for session storage
    std::unordered_map<std::string, std::shared_ptr<Session>> m_sessions; ///< Sessions by ID
    std::unordered_map<network::SocketType, std::string> m_socket_to_session; ///< Socket to session ID mapping
    std::unordered_map<std::string, std::string> m_player_to_session; ///< Player name to session ID mapping
    std::unordered_map<std::string, std::unordered_set<std::string>> m_ip_sessions; ///< IP to session IDs mapping
    
    // Threading
    std::atomic<bool> m_running;                   ///< Running flag
    std::thread m_cleanup_thread;                  ///< Cleanup thread
    std::thread m_heartbeat_thread;                ///< Heartbeat thread
    
    // Event handling
    std::function<void(std::shared_ptr<Session>, const std::string&)> m_session_event_callback;
    
    // Session ID generation
    std::atomic<uint64_t> m_next_session_id{1};    ///< Next session ID counter
    
    // Constants
    static constexpr size_t MAX_SESSIONS_ABSOLUTE = 10000;    ///< Absolute maximum sessions
    static constexpr size_t MAX_SESSIONS_PER_IP_ABSOLUTE = 50; ///< Absolute maximum sessions per IP
};

/**
 * @brief Create default session manager configuration
 * @return Default SessionManagerConfig instance
 */
SessionManagerConfig CreateDefaultSessionManagerConfig();

} // namespace server
} // namespace parallelstone

#endif // PARALLELSTONE_SERVER_SESSION_MANAGER_HPP