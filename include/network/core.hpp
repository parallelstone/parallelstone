/**
 * @file core.hpp
 * @brief Cross-platform network core abstraction layer for ParellelStone
 * @details This header provides a unified interface for high-performance networking
 *          across different platforms using platform-specific implementations:
 *          - Windows: RIO (Registered I/O) for maximum performance
 *          - Linux: io_uring for efficient async I/O operations
 *          - macOS: kqueue for event-driven network operations
 * @author @logpacket (https://github.com/logpacket)
 * @version 1.0.0
 * @date 2025-07-16
 */

#pragma once
#ifndef PARALLELSTONE_NETWORK_CORE_HPP
#define PARALLELSTONE_NETWORK_CORE_HPP

#include <cstdint>
#include <cstddef>
#include <functional>
#include <memory>

namespace parallelstone {
namespace network {

// Forward declaration
class Buffer;

/**
 * @enum NetworkResult
 * @brief Enumeration of network operation results
 * @details Provides standardized error codes for all network operations
 *          across different platforms. Used for consistent error handling
 *          and debugging throughout the networking layer.
 */
enum class NetworkResult {
    SUCCESS,                    ///< Operation completed successfully
    ERROR_INITIALIZATION,      ///< Failed to initialize network subsystem
    ERROR_SOCKET_CREATION,     ///< Failed to create socket
    ERROR_BIND,                ///< Failed to bind socket to address
    ERROR_LISTEN,              ///< Failed to set socket to listening state
    ERROR_ACCEPT,              ///< Failed to accept incoming connection
    ERROR_SEND,                ///< Failed to send data
    ERROR_RECEIVE,             ///< Failed to receive data
    ERROR_INVALID_ARGUMENT,   ///< Invalid argument provided to function
    ERROR_BUFFER_FULL,         ///< Buffer is full, cannot accept more data
    ERROR_CONNECTION_CLOSED,   ///< Connection was closed by peer
    ERROR_UNKNOWN              ///< Unknown or unspecified error occurred
};

/**
 * @brief Convert NetworkResult enum to human-readable string
 * @param result The NetworkResult value to convert
 * @return const char* String representation of the result
 * @details Provides string representations for all NetworkResult values
 *          for logging and debugging purposes.
 * 
 * @example
 * @code
 * NetworkResult result = NetworkResult::ERROR_BIND;
 * std::cout << "Error: " << NetworkResultToString(result) << std::endl;
 * // Output: "Error: Failed to bind socket to address"
 * @endcode
 */
const char* NetworkResultToString(NetworkResult result);

// Type definitions
using SocketType = uintptr_t;

/**
 * @interface INetworkCore
 * @brief Abstract interface for cross-platform network operations
 * @details Defines the common interface that all platform-specific network
 *          implementations must provide. This ensures consistent API across
 *          Windows (RIO), Linux (io_uring), and macOS (kqueue).
 * 
 * @note All async operations use callbacks with signature (NetworkResult, size_t)
 *       where the second parameter is bytes transferred or new socket handle
 */
class INetworkCore {
public:
    /**
     * @brief Virtual destructor for proper cleanup
     */
    virtual ~INetworkCore() = default;

    /**
     * @brief Initialize the network subsystem
     * @param config Configuration parameters for initialization
     * @return NetworkResult::SUCCESS if successful, error code otherwise
     * @details Must be called before any other operations
     */
    virtual NetworkResult Initialize(const struct NetworkConfig& config) = 0;
    
    /**
     * @brief Shutdown the network subsystem
     * @details Cleans up all resources and closes connections
     * @note Safe to call multiple times
     */
    virtual void Shutdown() = 0;
    
    /**
     * @brief Check if network subsystem is initialized
     * @return true if initialized and ready for operations
     */
    virtual bool IsInitialized() const = 0;
    
    // Socket management operations
    
    /**
     * @brief Create a new socket
     * @param socket Reference to store the created socket
     * @param family Address family (AF_INET, AF_INET6)
     * @param type Socket type (SOCK_STREAM, SOCK_DGRAM)
     * @return NetworkResult indicating success or failure
     */
    virtual NetworkResult CreateSocket(SocketType& socket, int family = 2, int type = 1) = 0; // AF_INET, SOCK_STREAM
    
    /**
     * @brief Bind socket to address
     * @param socket Socket to bind
     * @param addr Address structure
     * @param addr_len Length of address structure
     * @return NetworkResult indicating success or failure
     */
    virtual NetworkResult BindSocket(SocketType socket, const void* addr, size_t addr_len) = 0;
    
    /**
     * @brief Set socket to listening state
     * @param socket Socket to set to listening
     * @param backlog Maximum pending connections
     * @return NetworkResult indicating success or failure
     */
    virtual NetworkResult ListenSocket(SocketType socket, int backlog = 128) = 0;
    
    /**
     * @brief Close a socket
     * @param socket Socket to close
     * @return NetworkResult indicating success or failure
     */
    virtual NetworkResult CloseSocket(SocketType socket) = 0;
    
    // Asynchronous I/O operations
    
    /**
     * @brief Start asynchronous accept operation
     * @param listen_socket Listening socket
     * @param callback Completion callback (result, new_socket)
     * @return NetworkResult indicating if operation was started successfully
     */
    virtual NetworkResult AsyncAccept(SocketType listen_socket, 
                                      std::function<void(NetworkResult, SocketType)> callback) = 0;

    /**
     * @brief Start asynchronous receive operation
     * @param socket Socket to receive from
     * @param buffer Buffer to receive data into
     * @param callback Completion callback (result, bytes_received)
     * @return NetworkResult indicating if operation was started successfully
     */
    virtual NetworkResult AsyncReceive(SocketType socket, Buffer& buffer,
                                       std::function<void(NetworkResult, size_t)> callback) = 0;

    /**
     * @brief Start asynchronous send operation
     * @param socket Socket to send from
     * @param buffer Buffer containing data to send
     * @param callback Completion callback (result, bytes_sent)
     * @return NetworkResult indicating if operation was started successfully
     */
    virtual NetworkResult AsyncSend(SocketType socket, Buffer& buffer,
                                    std::function<void(NetworkResult, size_t)> callback) = 0;

    // Event processing
    
    /**
     * @brief Process completed operations
     * @param timeout_ms Timeout in milliseconds (0 = no wait, -1 = infinite)
     * @return Number of operations processed
     * @details Should be called regularly to handle async operation completions
     */
    virtual size_t ProcessCompletions(int timeout_ms = 0) = 0;
    
    // Configuration and status
    
    /**
     * @brief Get current configuration
     * @return Current network configuration
     */
    virtual const struct NetworkConfig& GetConfig() const = 0;
    
    /**
     * @brief Get platform-specific implementation name
     * @return String identifying the implementation ("RIO", "io_uring", "kqueue")
     */
    virtual const char* GetImplementationName() const = 0;
    
    /**
     * @brief Get current statistics
     * @param active_connections Number of active connections
     * @param pending_operations Number of pending async operations
     * @param bytes_sent Total bytes sent since initialization
     * @param bytes_received Total bytes received since initialization
     */
    virtual void GetStatistics(size_t& active_connections, size_t& pending_operations,
                             uint64_t& bytes_sent, uint64_t& bytes_received) const = 0;
};

/**
 * @struct NetworkConfig
 * @brief Configuration for the network core
 */
struct NetworkConfig {
    size_t queue_depth = 1024;          ///< Depth of completion/request queues
    size_t buffer_pool_size = 1024 * 1024 * 16; ///< 16MB buffer pool for RIO/io_uring
    size_t buffer_segment_size = 4096;  ///< 4KB segments for RIO/io_uring
    bool enable_nodelay = true;         ///< Enable TCP_NODELAY
    bool enable_keepalive = true;       ///< Enable TCP keep-alive
};

/**
 * @brief Create platform-specific network core implementation
 * @return std::unique_ptr to platform-appropriate implementation
 * @details Factory function that creates the optimal network implementation
 *          for the current platform:
 *          - Windows: RIONetworkCore
 *          - Linux: IOUringNetworkCore
 *          - macOS: KqueueNetworkCore
 * 
 * @example
 * @code
 * auto network = CreateNetworkCore();
 * if (network->Initialize() == NetworkResult::SUCCESS) {
 *     // Use network for operations
 * }
 * @endcode
 */
std::unique_ptr<INetworkCore> CreateNetworkCore();

} // namespace network
} // namespace parallelstone

// Include platform-specific implementations
#if defined(_WIN32)
    #include "rio.hpp"
#elif defined(__linux__)
    #include "io_uring.hpp"
#elif defined(__APPLE__)
    #include "kqueue.hpp"
#else
    #error "Unsupported platform for ParallelStone Network Core"
#endif

#endif // PARALLELSTONE_NETWORK_CORE_HPP