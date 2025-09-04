/**
 * @file core.cpp
 * @brief Implementation of cross-platform network core functionality
 * @details This file implements the common network interface functions
 *          and provides the factory function for creating platform-specific
 *          network implementations.
 * @author @logpacket (https://github.com/logpacket)
 * @version 1.0.0
 * @date 2025-07-16
 */

#include "network/core.hpp"
#include <stdexcept>
#include <cstddef>
#include <cstdint>

namespace parallelstone {
namespace network {

/**
 * @brief Convert NetworkResult enum to human-readable string
 * @param result The NetworkResult value to convert
 * @return const char* String representation of the result
 */
const char* NetworkResultToString(NetworkResult result) {
    switch (result) {
        case NetworkResult::SUCCESS:
            return "Operation completed successfully";
        case NetworkResult::ERROR_INITIALIZATION:
            return "Failed to initialize network subsystem";
        case NetworkResult::ERROR_SOCKET_CREATION:
            return "Failed to create socket";
        case NetworkResult::ERROR_BIND:
            return "Failed to bind socket to address";
        case NetworkResult::ERROR_LISTEN:
            return "Failed to set socket to listening state";
        case NetworkResult::ERROR_ACCEPT:
            return "Failed to accept incoming connection";
        case NetworkResult::ERROR_SEND:
            return "Failed to send data";
        case NetworkResult::ERROR_RECEIVE:
            return "Failed to receive data";
        case NetworkResult::ERROR_INVALID_ARGUMENT:
            return "Invalid parameter provided to function";
        case NetworkResult::ERROR_BUFFER_FULL:
            return "Buffer is full, cannot accept more data";
        case NetworkResult::ERROR_CONNECTION_CLOSED:
            return "Connection was closed by peer";
        case NetworkResult::ERROR_UNKNOWN:
        default:
            return "Unknown or unspecified error occurred";
    }
}

/**
 * @brief Create platform-specific network core implementation
 * @return std::unique_ptr to platform-appropriate implementation
 */
std::unique_ptr<INetworkCore> CreateNetworkCore() {
#if defined(_WIN32)
    // Windows: Use RIO implementation
    return std::make_unique<RIONetworkCore>();
#elif defined(__linux__)
    // Linux: Use io_uring implementation
    return std::make_unique<IOUringNetworkCore>();
#elif defined(__APPLE__)
    // macOS: Use kqueue implementation
    return std::make_unique<KqueueNetworkCore>();
#else
    throw std::runtime_error("Unsupported platform for ParallelStone Network Core");
#endif
}

} // namespace network
} // namespace parallelstone