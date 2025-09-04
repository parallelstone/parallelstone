/**
 * @file kqueue.hpp
 * @brief macOS kqueue implementation for high-performance networking
 * @details This header implements the macOS-specific network layer using
 *          kqueue for efficient event-driven I/O operations with excellent
 *          performance characteristics on macOS and BSD systems.
 * @author @logpacket (https://github.com/logpacket)
 * @version 1.0.0
 * @date 2025-07-16
 * @note Only available on macOS and BSD platforms with kqueue support
 */

#pragma once
#ifndef PARALLELSTONE_KQUEUE_NETWORK_HPP
#define PARALLELSTONE_KQUEUE_NETWORK_HPP

#ifdef __APPLE__

#include "network/core.hpp"
#include <sys/event.h>
#include <vector>
#include <functional>
#include <memory>
#include <unordered_map>

namespace parallelstone {
namespace network {

// Forward declaration
class Buffer;

/**
 * @enum KqueueEventType
 * @brief Type of kqueue event
 */
enum class KqueueEventType {
    ACCEPT,
    READ,
    WRITE,
    CLOSE
};

/**
 * @struct KqueueRequest
 * @brief Request structure for kqueue operations
 * @details Contains all information needed for a kqueue-based network operation
 *          including event type, file descriptor, buffer, and completion callback.
 */
struct KqueueRequest {
    KqueueEventType type;                           ///< Type of event (ACCEPT, READ, WRITE, CLOSE)
    int fd;                                         ///< File descriptor for the operation
    Buffer* buffer_ptr;                             ///< Pointer to buffer for data operations
    std::function<void(int, ssize_t)> callback;     ///< Completion callback (result, bytes_transferred)
    sockaddr_storage addr;                          ///< Socket address for accept operations
};

/**
 * @class KqueueNetworkCore
 * @brief macOS kqueue network implementation
 * @details Provides high-performance networking using macOS kqueue for
 *          efficient event-driven I/O operations. Manages kqueue instance
 *          and event monitoring for optimal performance on macOS/BSD systems.
 * 
 * @note Requires macOS or BSD system with kqueue support
 * 
 * @example
 * @code
 * KqueueNetworkCore kqueue;
 * if (kqueue.Initialize()) {
 *     int sock;
 *     kqueue.CreateSocket(sock);
 *     // Use socket for event-driven I/O
 * }
 * @endcode
 */
class KqueueNetworkCore : public INetworkCore {
public:
    /**
     * @brief Constructor
     * @details Initializes kqueue network core without starting it
     */
    KqueueNetworkCore();
    
    /**
     * @brief Destructor
     * @details Automatically calls Shutdown() if not already called
     */
    ~KqueueNetworkCore();
    
    /**
     * @brief Initialize the kqueue network subsystem
     * @return true if initialization successful, false otherwise
     * @details Creates kqueue instance and sets up event monitoring
     * @note Must be called before any other operations
     */
    NetworkResult Initialize(const NetworkConfig& config = NetworkConfig{}) override;
    
    /**
     * @brief Shutdown the kqueue network subsystem
     * @details Cleans up all resources and closes the kqueue instance
     * @note Safe to call multiple times
     */
    void Shutdown() override;
    
    /**
     * @brief Create a new socket
     * @param socket_fd Reference to store the created socket file descriptor
     * @param family Address family (default: AF_INET)
     * @param type Socket type (default: SOCK_STREAM)
     * @return true if socket created successfully, false otherwise
     */
    NetworkResult CreateSocket(SocketType& socket, int family = AF_INET, int type = SOCK_STREAM) override;
    
    /**
     * @brief Bind socket to address
     * @param socket_fd Socket file descriptor to bind
     * @param addr Address to bind to
     * @param addrlen Length of address structure
     * @return true if bind successful, false otherwise
     */
    NetworkResult BindSocket(SocketType socket, const void* addr, size_t addr_len) override;
    
    /**
     * @brief Set socket to listening state
     * @param socket_fd Socket to set to listening
     * @param backlog Maximum pending connections (default: SOMAXCONN)
     * @return true if listen successful, false otherwise
     */
    NetworkResult ListenSocket(SocketType socket, int backlog = 128) override;
    
    /**
     * @brief Start monitoring socket for accept events
     * @param listen_fd Listening socket file descriptor
     * @param callback Completion callback (result, new_socket_fd)
     * @return true if monitoring started successfully, false otherwise
     * @details Callback will be invoked when new connection is ready to accept
     */
    NetworkResult CloseSocket(SocketType socket) override;
    
    NetworkResult RegisterBuffer(NetworkBuffer& buffer) override;
    
    NetworkResult DeregisterBuffer(const NetworkBuffer& buffer) override;
    
    NetworkResult AsyncAccept(SocketType listen_socket, 
                            std::function<void(NetworkResult, SocketType)> callback) override;
    
    /**
     * @brief Start monitoring socket for read events
     * @param socket_fd Socket file descriptor to monitor
     * @param buffer Buffer to receive data into
     * @param length Maximum bytes to receive
     * @param callback Completion callback (result, bytes_available)
     * @return true if monitoring started successfully, false otherwise
     * @details Callback will be invoked when data is available for reading
     */
    NetworkResult AsyncReceive(SocketType socket, NetworkBuffer& buffer,
                             std::function<void(NetworkResult, size_t)> callback) override;
    
    /**
     * @brief Start monitoring socket for write events
     * @param socket_fd Socket file descriptor to monitor
     * @param buffer Buffer containing data to send
     * @param length Number of bytes to send
     * @param callback Completion callback (result, bytes_writable)
     * @return true if monitoring started successfully, false otherwise
     * @details Callback will be invoked when socket is ready for writing
     */
    NetworkResult AsyncSend(SocketType socket, const NetworkBuffer& buffer,
                          std::function<void(NetworkResult, size_t)> callback) override;
    
    /**
     * @brief Start monitoring socket for close events
     * @param socket_fd Socket file descriptor to monitor
     * @param callback Completion callback (result, 0)
     * @return true if monitoring started successfully, false otherwise
     * @details Callback will be invoked when socket is closed by peer
     */
    
    /**
     * @brief Process kqueue events
     * @param timeout_ms Timeout in milliseconds (-1 for blocking)
     * @details Checks for events and invokes callbacks for ready operations
     * @note Should be called regularly to process events
     */
    size_t ProcessCompletions(int timeout_ms = 0) override;
    
    /**
     * @brief Check if kqueue subsystem is initialized
     * @return true if initialized, false otherwise
     */
    bool IsInitialized() const override { return m_initialized; }
    
    const NetworkConfig& GetConfig() const override { return m_config; }
    
    const char* GetImplementationName() const override { return "kqueue"; }
    
    void GetStatistics(size_t& active_connections, size_t& pending_operations,
                     uint64_t& bytes_sent, uint64_t& bytes_received) const override;
    
    /**
     * @brief Set socket to non-blocking mode
     * @param fd File descriptor to set non-blocking
     * @details Required for proper async operation with kqueue
     */
    void ProcessEvents(int timeout_ms = -1);
    
    void SetNonBlocking(int fd);
    
    /**
     * @brief Remove socket from kqueue monitoring
     * @param fd File descriptor to remove
     * @details Stops monitoring events for the specified socket
     */
    void RemoveSocket(int fd);
    
    bool MonitorClose(int socket_fd, std::function<void(int, ssize_t)> callback);

private:
    /**
     * @brief Add event to kqueue for monitoring
     * @param fd File descriptor to monitor
     * @param filter Event filter (EVFILT_READ, EVFILT_WRITE, etc.)
     * @param flags Event flags (EV_ADD, EV_ENABLE, etc.)
     * @param udata User data pointer for the event
     * @return true if event added successfully, false otherwise
     */
    bool AddEvent(int fd, short filter, unsigned short flags, void* udata);
    
    /**
     * @brief Remove event from kqueue monitoring
     * @param fd File descriptor to stop monitoring
     * @param filter Event filter to remove
     * @return true if event removed successfully, false otherwise
     */
    bool RemoveEvent(int fd, short filter);
    
    /**
     * @brief Create new request structure
     * @param type Type of event
     * @param fd File descriptor for operation
     * @return Pointer to new request, nullptr if allocation failed
     */
    KqueueRequest* CreateRequest(KqueueEventType type, int fd);
    
    bool m_initialized;     ///< Initialization state flag
    NetworkConfig m_config; ///< Current configuration
    int m_kqueue_fd;        ///< kqueue file descriptor
    
    // Statistics
    mutable size_t m_active_connections;    ///< Number of active connections
    mutable size_t m_pending_operations;    ///< Number of pending operations
    mutable uint64_t m_bytes_sent;          ///< Total bytes sent
    mutable uint64_t m_bytes_received;      ///< Total bytes received
    
    std::unordered_map<int, std::unique_ptr<KqueueRequest>> m_requests; ///< Active request objects by fd
    std::vector<struct kevent> m_events;                                 ///< Event buffer for kevent() calls
    
    static constexpr int MAX_EVENTS = 256;  ///< Maximum events per kevent() call
    static constexpr int INVALID_FD = -1;   ///< Invalid file descriptor value
};

} // namespace network
} // namespace parallelstone

#endif // __APPLE__
#endif // PARALLELSTONE_KQUEUE_NETWORK_HPP