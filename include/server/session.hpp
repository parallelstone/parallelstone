/**
 * @file session.hpp
 * @brief Client session management for ParellelStone server
 * @details This header defines the Session class that represents individual
 *          client connections and handles Minecraft protocol communication,
 *          authentication, and state management for each connected player.
 * @author @logpacket (https://github.com/logpacket)
 * @version 1.0.0
 * @date 2025-07-16
 */

#pragma once
#ifndef PARALLELSTONE_SERVER_SESSION_HPP
#define PARALLELSTONE_SERVER_SESSION_HPP

#include <memory>
#include <string>
#include <atomic>
#include <chrono>
#include <functional>
#include <queue>
#include <mutex>
#include <cstdint>
#include "network/core.hpp"
#include "network/buffer.hpp"
#include "network/packet_view.hpp"

// Forward declaration of global protocol namespace
namespace protocol {
    class PacketDispatcher;
}

/**
 * @namespace parallelstone::server
 * @brief Server-side components for ParellelStone Minecraft server
 */
namespace parallelstone {
namespace server {

// Forward declarations  
class SessionManager;

/**
 * @enum SessionState
 * @brief Enumeration of possible session states
 * @details Defines the lifecycle states of a client session following
 *          the Minecraft protocol state machine.
 */
enum class SessionState {
    CONNECTING,     ///< Initial connection state
    HANDSHAKING,    ///< Processing handshake packet
    STATUS,         ///< Server status query state
    LOGIN,          ///< Login and authentication state
    CONFIGURATION,  ///< Client configuration state (1.20.2+)
    PLAY,           ///< Active gameplay state
    DISCONNECTING,  ///< Graceful disconnect in progress
    DISCONNECTED    ///< Session has been terminated
};

/**
 * @enum DisconnectReason
 * @brief Enumeration of disconnect reasons
 * @details Provides categorized reasons for session termination
 *          for logging and debugging purposes.
 */
enum class DisconnectReason {
    UNKNOWN,                ///< Unknown or unspecified reason
    CLIENT_DISCONNECT,      ///< Client initiated disconnect
    SERVER_SHUTDOWN,        ///< Server is shutting down
    TIMEOUT,               ///< Connection timeout
    PROTOCOL_ERROR,        ///< Invalid protocol data
    AUTHENTICATION_FAILED, ///< Authentication failure
    SERVER_FULL,           ///< Server at maximum capacity
    BANNED,                ///< Client is banned
    NETWORK_ERROR,         ///< Network layer error
    INTERNAL_ERROR         ///< Internal server error
};

/**
 * @struct SessionInfo
 * @brief Information about a client session
 * @details Contains metadata and statistics for a client session
 *          including connection details and player information.
 */
struct SessionInfo {
    std::string session_id;                 ///< Unique session identifier
    std::string client_ip;                  ///< Client IP address
    uint16_t client_port;                   ///< Client port number
    std::string player_name;                ///< Minecraft player name
    std::string player_uuid;                ///< Minecraft player UUID
    int32_t protocol_version;               ///< Client protocol version
    std::chrono::steady_clock::time_point connect_time; ///< Connection timestamp
    std::chrono::steady_clock::time_point last_activity; ///< Last activity timestamp
    
    // Statistics
    std::atomic<uint64_t> bytes_sent{0};        ///< Total bytes sent to client
    std::atomic<uint64_t> bytes_received{0};    ///< Total bytes received from client
    std::atomic<uint64_t> packets_sent{0};      ///< Total packets sent to client
    std::atomic<uint64_t> packets_received{0};  ///< Total packets received from client
    
    /**
     * @brief Get session duration
     * @return Duration since connection establishment
     */
    std::chrono::seconds GetDuration() const {
        return std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - connect_time);
    }
    
    /**
     * @brief Get time since last activity
     * @return Duration since last activity
     */
    std::chrono::seconds GetIdleTime() const {
        return std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - last_activity);
    }
};

/**
 * @class Session
 * @brief Represents a client connection and handles protocol communication
 * @details The Session class manages individual client connections throughout
 *          their lifecycle, from initial handshake through gameplay or status
 *          queries. It handles packet processing, state transitions, and
 *          provides an interface for game logic interaction.
 * 
 * @example
 * @code
 * auto session = std::make_shared<Session>(socket, session_manager);
 * session->Start();
 * session->SetDisconnectCallback([](auto session, auto reason) {
 *     // Handle disconnect
 * });
 * @endcode
 */
class Session : public std::enable_shared_from_this<Session> {
protected:
    /**
     * @brief Default constructor for mocking
     */
    Session();

public:
    /**
     * @brief Constructor
     * @param socket Network socket for this session
     * @param manager Reference to session manager
     */
    Session(parallelstone::network::SocketType socket, SessionManager& manager);
    
    /**
     * @brief Destructor - ensures proper cleanup
     */
    virtual ~Session();
    
    // Non-copyable and non-movable
    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;
    Session(Session&&) = delete;
    Session& operator=(Session&&) = delete;
    
    /**
     * @brief Start the session
     * @details Begins packet processing and state management for the session
     */
    void Start();
    
    /**
     * @brief Disconnect the session
     * @param reason Reason for disconnection
     * @param message Optional disconnect message for client
     */
    virtual void Disconnect(DisconnectReason reason = DisconnectReason::UNKNOWN, 
                   const std::string& message = "");
    
    /**
     * @brief Send a packet to the client.
     * @param packet The buffer containing the packet data (ID + payload).
     */
    void Send(parallelstone::network::Buffer& packet);
    
    /**
     * @brief Process session events (called by session manager)
     * @details Handles incoming data, packet processing, and state updates
     */
    void ProcessEvents();
    
    /**
     * @brief Get current session state
     * @return Current SessionState
     */
    SessionState GetState() const { return m_state.load(); }
    
    /**
     * @brief Set the next protocol state for this session
     * @param state The new session state
     */
    virtual void SetNextState(SessionState state);
    
    /**
     * @brief Check if session is active
     * @return true if session is in an active state
     */
    bool IsActive() const {
        SessionState state = GetState();
        return state != SessionState::DISCONNECTING && state != SessionState::DISCONNECTED;
    }
    
    /**
     * @brief Get session information
     * @return SessionInfo structure with current session data
     */
    const SessionInfo& GetInfo() const { return m_info; }
    
    /**
     * @brief Get socket handle
     * @return Network socket for this session
     */
    parallelstone::network::SocketType GetSocket() const { return m_socket; }
    
    /**
     * @brief Get unique session ID (string version)
     * @return Session identifier string
     */
    virtual const std::string& GetSessionId() const;
    
    /**
     * @brief Get the remote IP address of this session
     * @return Remote IP address as string
     */
    virtual const std::string& GetRemoteAddress() const;
    
    /**
     * @brief Get the remote port of this session
     * @return Remote port number
     */
    virtual uint16_t GetRemotePort() const;
    
    /**
     * @brief Send a disconnect message to the client
     * @param reason The reason for disconnection
     */
    void SendDisconnect(const std::string& reason);
    
    /**
     * @brief Set disconnect callback
     * @param callback Function to call when session disconnects
     */
    void SetDisconnectCallback(std::function<void(std::shared_ptr<Session>, DisconnectReason)> callback) {
        m_disconnect_callback = std::move(callback);
    }
    
    /**
     * @brief Set packet callback for received packets
     * @param callback Function to call for each received packet
     */
    void SetPacketCallback(std::function<void(std::shared_ptr<Session>, int32_t, parallelstone::network::PacketView&)> callback) {
        m_packet_callback = std::move(callback);
    }
    
    /**
     * @brief Set state change callback
     * @param callback Function to call when session state changes
     */
    void SetStateChangeCallback(std::function<void(std::shared_ptr<Session>, SessionState, SessionState)> callback) {
        m_state_change_callback = std::move(callback);
    }
    
    /**
     * @brief Update last activity timestamp
     * @details Called automatically when data is received
     */
    void UpdateActivity() {
        m_info.last_activity = std::chrono::steady_clock::now();
    }
    
    /**
     * @brief Check if session has timed out
     * @param timeout_duration Timeout threshold
     * @return true if session has exceeded timeout
     */
    bool HasTimedOut(std::chrono::milliseconds timeout_duration) const {
        return GetInfo().GetIdleTime() > std::chrono::duration_cast<std::chrono::seconds>(timeout_duration);
    }
    
    /**
     * @brief Force flush pending outgoing data
     * @details Immediately sends any queued outgoing packets
     */
    void FlushOutgoing();

private:
    /**
     * @brief Handle incoming data from network
     * @param result Network operation result
     * @param bytes_received Number of bytes received
     */
    void OnDataReceived(parallelstone::network::NetworkResult result, size_t bytes_received);
    
    /**
     * @brief Handle outgoing data completion
     * @param result Network operation result
     * @param bytes_sent Number of bytes sent
     */
    void OnDataSent(parallelstone::network::NetworkResult result, size_t bytes_sent);
    
    /**
     * @brief Process all complete packets in the receive buffer.
     */
    void ProcessReceivedPackets();
    
    /**
     * @brief Transition to new session state
     * @param new_state Target session state
     */
    void TransitionToState(SessionState new_state);
    
    /**
     * @brief Initialize session from socket
     * @details Extracts client information from socket
     */
    void InitializeFromSocket();
    
    /**
     * @brief Start receiving data
     * @details Initiates async receive operation
     */
    void StartReceive();
    
    /**
     * @brief Start sending queued data
     * @details Initiates async send operation for queued packets
     */
    void StartSend();
    
    /**
     * @brief Generate unique session ID
     * @return Unique identifier string
     */
    std::string GenerateSessionId();
    
    /**
     * @brief Handle network error
     * @param result Network error result
     */
    void HandleNetworkError(parallelstone::network::NetworkResult result);
    
    /**
     * @brief Cleanup session resources
     */
    void Cleanup();
    
    // Core session data
    parallelstone::network::SocketType m_socket;                  ///< Network socket
    SessionManager& m_session_manager;             ///< Reference to session manager
    std::atomic<SessionState> m_state;             ///< Current session state
    SessionInfo m_info;                            ///< Session information and statistics
    ::protocol::PacketDispatcher& m_dispatcher;      ///< Packet dispatcher
    
    // Packet processing
    parallelstone::network::Buffer m_receive_buffer;              ///< Buffer for incoming data
    std::queue<parallelstone::network::Buffer> m_outgoing_queue;  ///< Queue of outgoing packets
    std::mutex m_outgoing_mutex;                   ///< Mutex for outgoing packet queue
    std::atomic<bool> m_is_sending;                ///< Flag indicating send operation in progress
    
    // Callbacks
    std::function<void(std::shared_ptr<Session>, DisconnectReason)> m_disconnect_callback;
    std::function<void(std::shared_ptr<Session>, int32_t, parallelstone::network::PacketView&)> m_packet_callback;
    std::function<void(std::shared_ptr<Session>, SessionState, SessionState)> m_state_change_callback;
    
    // Configuration
    static constexpr size_t MAX_PACKET_SIZE = 2097151;     ///< Maximum packet size (2MB - 1)
    static constexpr size_t RECEIVE_BUFFER_SIZE = 8192;    ///< Receive buffer size
    static constexpr size_t MAX_QUEUED_PACKETS = 100;      ///< Maximum queued outgoing packets
};

/**
 * @brief Convert SessionState to string
 * @param state SessionState to convert
 * @return String representation of the state
 */
const char* SessionStateToString(SessionState state);

/**
 * @brief Convert DisconnectReason to string
 * @param reason DisconnectReason to convert
 * @return String representation of the reason
 */
const char* DisconnectReasonToString(DisconnectReason reason);

} // namespace server
} // namespace parallelstone

// ============================================================================
// PROTOCOL NAMESPACE COMPATIBILITY LAYER
// ============================================================================

/**
 * @namespace protocol
 * @brief Protocol-specific types and functions for backward compatibility
 */
namespace protocol {

// Type aliases for backward compatibility
using SessionState = parallelstone::server::SessionState;
using Session = parallelstone::server::Session;
using PacketView = parallelstone::network::PacketView; // PacketView 별칭 추가

} // namespace protocol

#endif // PARALLELSTONE_SERVER_SESSION_HPP