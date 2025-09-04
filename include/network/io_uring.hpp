/**
 * @file io_uring.hpp
 * @brief Linux io_uring implementation for high-performance networking
 * @details This header implements the Linux-specific network layer using
 *          io_uring for efficient asynchronous I/O operations with minimal
 *          system call overhead and optimal performance.
 * @author @logpacket (https://github.com/logpacket)
 * @version 1.0.0
 * @date 2025-07-16
 * @note Only available on Linux platforms with io_uring support (kernel 5.1+)
 */

#pragma once
#ifndef PARALLELSTONE_IO_URING_NETWORK_HPP
#define PARALLELSTONE_IO_URING_NETWORK_HPP

#ifdef __linux__

#include <liburing.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <memory>
#include <functional>
#include <vector>
#include <queue>
#include <unordered_map>
#include "core.hpp"

/**
 * @namespace parallelstone::network
 * @brief Linux io_uring networking implementation
 */
namespace parallelstone {
namespace network {

// Forward declaration of interface
class INetworkCore;

/**
 * @enum IOUringOpType
 * @brief Types of io_uring operations supported
 * @details Defines the different asynchronous operations that can be
 *          performed using the io_uring interface.
 */
enum class IOUringOpType {
    ACCEPT,     ///< Accept incoming connection
    RECEIVE,    ///< Receive data from socket
    SEND,       ///< Send data to socket
    CLOSE       ///< Close socket connection
};

/**
 * @struct IOUringRequest
 * @brief Request structure for io_uring operations
 * @details Contains all information needed for an asynchronous io_uring operation
 *          including operation type, file descriptor, buffer, and completion callback.
 */
struct IOUringRequest {
    IOUringOpType type;                       ///< Type of operation (ACCEPT, RECEIVE, SEND, CLOSE)
    int fd;                                   ///< File descriptor for the operation
    void* buffer;                             ///< Buffer for data operations
    size_t length;                            ///< Length of buffer or operation
    std::function<void(int, int)> callback;   ///< Completion callback (result, bytes_transferred)
    sockaddr_storage addr;                    ///< Socket address for accept operations
    socklen_t addr_len;                       ///< Length of socket address
};

/**
 * @class IOUringNetworkCore
 * @brief Linux io_uring-based network implementation
 * @details Provides high-performance networking using Linux io_uring for
 *          efficient asynchronous I/O with minimal system call overhead.
 *          Manages submission and completion queues for optimal performance.
 * 
 * @note Requires Linux kernel 5.1+ with io_uring support
 * 
 * @example
 * @code
 * IOUringNetworkCore uring;
 * if (uring.Initialize(512)) {
 *     int sock;
 *     uring.CreateSocket(sock);
 *     // Use socket for high-performance async I/O
 * }
 * @endcode
 */
class IOUringNetworkCore : public INetworkCore {
public:
    /**
     * @brief Constructor
     * @details Initializes io_uring network core without starting it
     */
    IOUringNetworkCore();
    
    /**
     * @brief Destructor
     * @details Automatically calls Shutdown() if not already called
     */
    ~IOUringNetworkCore();
    
    /**
     * @brief Initialize the io_uring network subsystem
     * @param queue_depth Size of submission/completion queues (default: 256)
     * @return true if initialization successful, false otherwise
     * @details Sets up io_uring instance with specified queue depth
     * @note Must be called before any other operations
     */
    NetworkResult Initialize(const NetworkConfig& config = NetworkConfig{}) override;
    
    /**
     * @brief Shutdown the io_uring network subsystem
     * @details Cleans up all resources and closes the io_uring instance
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
     * @brief Start asynchronous accept operation
     * @param listen_fd Listening socket file descriptor
     * @param callback Completion callback (result, new_socket_fd)
     * @return true if operation started successfully, false otherwise
     * @details Callback will be invoked when new connection is accepted
     */
    NetworkResult CloseSocket(SocketType socket) override;
    
    NetworkResult RegisterBuffer(NetworkBuffer& buffer) override;
    
    NetworkResult DeregisterBuffer(const NetworkBuffer& buffer) override;
    
    NetworkResult AsyncAccept(SocketType listen_socket, 
                            std::function<void(NetworkResult, SocketType)> callback) override;
    
    /**
     * @brief Start asynchronous receive operation
     * @param socket_fd Socket file descriptor to receive from
     * @param buffer Buffer to receive data into
     * @param length Maximum bytes to receive
     * @param callback Completion callback (result, bytes_received)
     * @return true if operation started successfully, false otherwise
     * @details Callback will be invoked when data is received or error occurs
     */
    NetworkResult AsyncReceive(SocketType socket, NetworkBuffer& buffer,
                             std::function<void(NetworkResult, size_t)> callback) override;
    
    /**
     * @brief Start asynchronous send operation
     * @param socket_fd Socket file descriptor to send from
     * @param buffer Buffer containing data to send
     * @param length Number of bytes to send
     * @param callback Completion callback (result, bytes_sent)
     * @return true if operation started successfully, false otherwise
     * @details Callback will be invoked when data is sent or error occurs
     */
    NetworkResult AsyncSend(SocketType socket, const NetworkBuffer& buffer,
                          std::function<void(NetworkResult, size_t)> callback) override;
    
    /**
     * @brief Start asynchronous close operation
     * @param socket_fd Socket file descriptor to close
     * @param callback Completion callback (result, 0)
     * @return true if operation started successfully, false otherwise
     * @details Callback will be invoked when socket is closed
     */
    
    /**
     * @brief Process completed I/O operations
     * @details Checks completion queue and invokes callbacks for finished operations
     * @note Should be called regularly to process async operations
     */
    size_t ProcessCompletions(int timeout_ms = 0) override;
    
    /**
     * @brief Check if io_uring subsystem is initialized
     * @return true if initialized, false otherwise
     */
    bool IsInitialized() const override { return m_initialized; }
    
    const NetworkConfig& GetConfig() const override { return m_config; }
    
    const char* GetImplementationName() const override { return "io_uring"; }
    
    void GetStatistics(size_t& active_connections, size_t& pending_operations,
                     uint64_t& bytes_sent, uint64_t& bytes_received) const override;
    
    /**
     * @brief Get current queue depth
     * @return Current queue depth setting
     */
    int GetQueueDepth() const { return m_queue_depth; }
    
    /**
     * @brief Set socket to non-blocking mode
     * @param fd File descriptor to set non-blocking
     * @details Required for proper async operation with io_uring
     */
    void SetNonBlocking(int fd);

private:
    /**
     * @brief Setup io_uring instance
     * @return true if setup successful, false otherwise
     * @details Initializes submission and completion queues
     */
    bool SetupRing();
    
    /**
     * @brief Cleanup io_uring instance
     * @details Frees all io_uring resources
     */
    void CleanupRing();
    
    /**
     * @brief Create new request structure
     * @param type Type of operation
     * @param fd File descriptor for operation
     * @return Pointer to new request, nullptr if allocation failed
     */
    IOUringRequest* CreateRequest(IOUringOpType type, int fd);
    
    bool m_initialized;         ///< Initialization state flag
    NetworkConfig m_config;     ///< Current configuration
    struct io_uring m_ring;     ///< io_uring instance
    unsigned m_queue_depth;     ///< Configured queue depth
    
    // Statistics
    mutable size_t m_active_connections;    ///< Number of active connections
    mutable size_t m_pending_operations;    ///< Number of pending operations
    mutable uint64_t m_bytes_sent;          ///< Total bytes sent
    mutable uint64_t m_bytes_received;      ///< Total bytes received
    
    std::vector<std::unique_ptr<IOUringRequest>> m_requests;  ///< Active request objects
    std::queue<IOUringRequest*> m_free_requests;               ///< Pool of free request objects
    
    static constexpr unsigned DEFAULT_QUEUE_DEPTH = 256;   ///< Default queue depth
    static constexpr unsigned MAX_QUEUE_DEPTH = 4096;      ///< Maximum allowed queue depth
};

} // namespace network
} // namespace parallelstone

#endif // __linux__
#endif // PARALLELSTONE_IO_URING_NETWORK_HPP