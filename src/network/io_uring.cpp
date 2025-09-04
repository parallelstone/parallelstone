/**
 * @file io_uring.cpp
 * @brief Linux io_uring network implementation
 * @details This file implements the Linux-specific network layer using
 *          io_uring for efficient asynchronous I/O operations.
 * @author @logpacket (https://github.com/logpacket)
 * @version 1.0.0
 * @date 2025-07-16
 */

#ifdef __linux__

#include "network/io_uring.hpp"
#include <spdlog/spdlog.h>
#include <iostream>
#include <algorithm>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <cstddef>
#include <cstdint>

namespace parallelstone {
namespace network {

IOUringNetworkCore::IOUringNetworkCore()
    : m_initialized(false)
    , m_config()
    , m_queue_depth(DEFAULT_QUEUE_DEPTH)
    , m_active_connections(0)
    , m_pending_operations(0)
    , m_bytes_sent(0)
    , m_bytes_received(0) {
    memset(&m_ring, 0, sizeof(m_ring));
}

IOUringNetworkCore::~IOUringNetworkCore() {
    if (m_initialized) {
        Shutdown();
    }
}

NetworkResult IOUringNetworkCore::Initialize(const NetworkConfig& config) {
    if (m_initialized) {
        return NetworkResult::SUCCESS;
    }

    m_config = config;
    m_queue_depth = std::min(static_cast<unsigned>(config.queue_depth), MAX_QUEUE_DEPTH);

    // Initialize io_uring
    if (!SetupRing()) {
        return NetworkResult::ERROR_INITIALIZATION;
    }

    // Pre-allocate request objects for better performance
    m_requests.reserve(m_queue_depth);
    for (unsigned i = 0; i < m_queue_depth; ++i) {
        auto request = std::make_unique<IOUringRequest>();
        request->type = IOUringOpType::CLOSE; // Default state
        request->fd = -1;
        request->buffer = nullptr;
        request->length = 0;
        request->addr_len = 0;
        m_free_requests.push(request.get());
        m_requests.push_back(std::move(request));
    }

    m_initialized = true;
    return NetworkResult::SUCCESS;
}

void IOUringNetworkCore::Shutdown() {
    if (!m_initialized) {
        return;
    }

    // Clear all pending requests
    while (!m_free_requests.empty()) {
        m_free_requests.pop();
    }
    m_requests.clear();

    // Deregister all buffers
    m_registered_buffers.clear();

    // Cleanup io_uring
    CleanupRing();
    
    m_initialized = false;
}

bool IOUringNetworkCore::SetupRing() {
    struct io_uring_params params;
    memset(&params, 0, sizeof(params));
    
    // Initialize io_uring with specified queue depth
    int ret = io_uring_queue_init_params(m_queue_depth, &m_ring, &params);
    if (ret < 0) {
        spdlog::error("Failed to initialize io_uring: {}", strerror(-ret));
        return false;
    }

    // Check if required features are supported
    if (!(params.features & IORING_FEAT_FAST_POLL)) {
        spdlog::warn("Warning: IORING_FEAT_FAST_POLL not supported");
    }

    return true;
}

void IOUringNetworkCore::CleanupRing() {
    if (m_ring.ring_fd >= 0) {
        io_uring_queue_exit(&m_ring);
        memset(&m_ring, 0, sizeof(m_ring));
    }
}

NetworkResult IOUringNetworkCore::CreateSocket(SocketType& socket, int family, int type) {
    if (!m_initialized) {
        return NetworkResult::ERROR_INITIALIZATION;
    }

    int sock = ::socket(family, type, 0);
    if (sock < 0) {
        return NetworkResult::ERROR_SOCKET_CREATION;
    }

    // Set socket options
    if (m_config.enable_nodelay && type == SOCK_STREAM) {
        int nodelay = 1;
        setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay));
    }

    if (m_config.enable_keepalive) {
        int keepalive = 1;
        setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &keepalive, sizeof(keepalive));
    }

    // Set to non-blocking
    SetNonBlocking(sock);

    socket = static_cast<SocketType>(sock);
    return NetworkResult::SUCCESS;
}

NetworkResult IOUringNetworkCore::BindSocket(SocketType socket, const void* addr, size_t addr_len) {
    if (!m_initialized) {
        return NetworkResult::ERROR_INITIALIZATION;
    }

    int sock = static_cast<int>(socket);
    int result = bind(sock, static_cast<const struct sockaddr*>(addr), 
                     static_cast<socklen_t>(addr_len));
    
    return (result == 0) ? NetworkResult::SUCCESS : NetworkResult::ERROR_BIND;
}

NetworkResult IOUringNetworkCore::ListenSocket(SocketType socket, int backlog) {
    if (!m_initialized) {
        return NetworkResult::ERROR_INITIALIZATION;
    }

    int sock = static_cast<int>(socket);
    int result = listen(sock, backlog);
    
    return (result == 0) ? NetworkResult::SUCCESS : NetworkResult::ERROR_LISTEN;
}

NetworkResult IOUringNetworkCore::CloseSocket(SocketType socket) {
    if (!m_initialized) {
        return NetworkResult::ERROR_INITIALIZATION;
    }

    int sock = static_cast<int>(socket);
    int result = close(sock);
    
    if (result == 0) {
        m_active_connections--;
        return NetworkResult::SUCCESS;
    }
    
    return NetworkResult::ERROR_UNKNOWN;
}

NetworkResult IOUringNetworkCore::RegisterBuffer(NetworkBuffer& buffer) {
    if (!m_initialized) {
        return NetworkResult::ERROR_INITIALIZATION;
    }

    if (!buffer.data || buffer.capacity == 0) {
        return NetworkResult::ERROR_INVALID_ARGUMENT;
    }

    // For io_uring, we can register buffers for optimized access
    // For simplicity, we'll just track them for now
    buffer.buffer_id = buffer.data;
    m_registered_buffers[buffer.data] = buffer;
    
    return NetworkResult::SUCCESS;
}

NetworkResult IOUringNetworkCore::DeregisterBuffer(const NetworkBuffer& buffer) {
    if (!m_initialized) {
        return NetworkResult::ERROR_INITIALIZATION;
    }

    auto it = m_registered_buffers.find(buffer.data);
    if (it == m_registered_buffers.end()) {
        return NetworkResult::ERROR_INVALID_ARGUMENT;
    }

    m_registered_buffers.erase(it);
    return NetworkResult::SUCCESS;
}

NetworkResult IOUringNetworkCore::AsyncAccept(SocketType listen_socket, 
                                            std::function<void(NetworkResult, SocketType)> callback) {
    if (!m_initialized) {
        return NetworkResult::ERROR_INITIALIZATION;
    }

    IOUringRequest* request = CreateRequest(IOUringOpType::ACCEPT, static_cast<int>(listen_socket));
    if (!request) {
        return NetworkResult::ERROR_BUFFER_FULL;
    }

    request->callback = [callback](int result, int value) {
        if (result >= 0) {
            callback(NetworkResult::SUCCESS, static_cast<SocketType>(result));
        } else {
            callback(NetworkResult::ERROR_ACCEPT, 0);
        }
    };

    // Prepare accept operation
    struct io_uring_sqe* sqe = io_uring_get_sqe(&m_ring);
    if (!sqe) {
        return NetworkResult::ERROR_BUFFER_FULL;
    }

    request->addr_len = sizeof(request->addr);
    io_uring_prep_accept(sqe, request->fd, 
                        reinterpret_cast<struct sockaddr*>(&request->addr),
                        &request->addr_len, 0);
    io_uring_sqe_set_data(sqe, request);

    // Submit the operation
    int ret = io_uring_submit(&m_ring);
    if (ret < 0) {
        return NetworkResult::ERROR_ACCEPT;
    }

    m_pending_operations++;
    return NetworkResult::SUCCESS;
}

NetworkResult IOUringNetworkCore::AsyncReceive(SocketType socket, NetworkBuffer& buffer,
                                             std::function<void(NetworkResult, size_t)> callback) {
    if (!m_initialized) {
        return NetworkResult::ERROR_INITIALIZATION;
    }

    IOUringRequest* request = CreateRequest(IOUringOpType::RECEIVE, static_cast<int>(socket));
    if (!request) {
        return NetworkResult::ERROR_BUFFER_FULL;
    }

    request->buffer = buffer.data;
    request->length = buffer.capacity;
    request->callback = [callback](int result, int bytes) {
        if (result >= 0) {
            callback(NetworkResult::SUCCESS, static_cast<size_t>(result));
        } else {
            NetworkResult error = (result == -ECONNRESET) ? 
                                 NetworkResult::ERROR_CONNECTION_CLOSED : 
                                 NetworkResult::ERROR_RECEIVE;
            callback(error, 0);
        }
    };

    // Prepare receive operation
    struct io_uring_sqe* sqe = io_uring_get_sqe(&m_ring);
    if (!sqe) {
        return NetworkResult::ERROR_BUFFER_FULL;
    }

    io_uring_prep_recv(sqe, request->fd, request->buffer, request->length, 0);
    io_uring_sqe_set_data(sqe, request);

    // Submit the operation
    int ret = io_uring_submit(&m_ring);
    if (ret < 0) {
        return NetworkResult::ERROR_RECEIVE;
    }

    m_pending_operations++;
    return NetworkResult::SUCCESS;
}

NetworkResult IOUringNetworkCore::AsyncSend(SocketType socket, const NetworkBuffer& buffer,
                                          std::function<void(NetworkResult, size_t)> callback) {
    if (!m_initialized) {
        return NetworkResult::ERROR_INITIALIZATION;
    }

    IOUringRequest* request = CreateRequest(IOUringOpType::SEND, static_cast<int>(socket));
    if (!request) {
        return NetworkResult::ERROR_BUFFER_FULL;
    }

    request->buffer = buffer.data;
    request->length = buffer.length;
    request->callback = [callback](int result, int bytes) {
        if (result >= 0) {
            callback(NetworkResult::SUCCESS, static_cast<size_t>(result));
        } else {
            NetworkResult error = (result == -EPIPE || result == -ECONNRESET) ? 
                                 NetworkResult::ERROR_CONNECTION_CLOSED : 
                                 NetworkResult::ERROR_SEND;
            callback(error, 0);
        }
    };

    // Prepare send operation
    struct io_uring_sqe* sqe = io_uring_get_sqe(&m_ring);
    if (!sqe) {
        return NetworkResult::ERROR_BUFFER_FULL;
    }

    io_uring_prep_send(sqe, request->fd, request->buffer, request->length, 0);
    io_uring_sqe_set_data(sqe, request);

    // Submit the operation
    int ret = io_uring_submit(&m_ring);
    if (ret < 0) {
        return NetworkResult::ERROR_SEND;
    }

    m_pending_operations++;
    return NetworkResult::SUCCESS;
}

size_t IOUringNetworkCore::ProcessCompletions(int timeout_ms) {
    if (!m_initialized) {
        return 0;
    }

    struct io_uring_cqe* cqe;
    size_t completed = 0;
    
    // Set up timeout if specified
    struct __kernel_timespec timeout;
    struct __kernel_timespec* timeout_ptr = nullptr;
    
    if (timeout_ms >= 0) {
        timeout.tv_sec = timeout_ms / 1000;
        timeout.tv_nsec = (timeout_ms % 1000) * 1000000;
        timeout_ptr = &timeout;
    }

    // Process completions
    while (io_uring_wait_cqe_timeout(&m_ring, &cqe, timeout_ptr) == 0) {
        IOUringRequest* request = static_cast<IOUringRequest*>(io_uring_cqe_get_data(cqe));
        
        if (request && request->callback) {
            // Execute callback with result
            request->callback(cqe->res, cqe->res);
            
            // Update statistics
            if (cqe->res >= 0) {
                if (request->type == IOUringOpType::SEND) {
                    m_bytes_sent += cqe->res;
                } else if (request->type == IOUringOpType::RECEIVE) {
                    m_bytes_received += cqe->res;
                } else if (request->type == IOUringOpType::ACCEPT) {
                    m_active_connections++;
                    // Set new socket to non-blocking
                    if (cqe->res >= 0) {
                        SetNonBlocking(cqe->res);
                    }
                }
            }
            
            // Return request to free pool
            m_free_requests.push(request);
            m_pending_operations--;
        }
        
        // Mark completion as seen
        io_uring_cqe_seen(&m_ring, cqe);
        completed++;
        
        // Break on timeout (no more completions available)
        timeout_ptr = nullptr; // Don't wait for subsequent completions
    }

    return completed;
}

void IOUringNetworkCore::SetNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags >= 0) {
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }
}

IOUringRequest* IOUringNetworkCore::CreateRequest(IOUringOpType type, int fd) {
    if (m_free_requests.empty()) {
        return nullptr;
    }

    IOUringRequest* request = m_free_requests.front();
    m_free_requests.pop();

    request->type = type;
    request->fd = fd;
    request->buffer = nullptr;
    request->length = 0;
    request->callback = nullptr;
    request->addr_len = 0;
    memset(&request->addr, 0, sizeof(request->addr));

    return request;
}

void IOUringNetworkCore::GetStatistics(size_t& active_connections, size_t& pending_operations,
                                     uint64_t& bytes_sent, uint64_t& bytes_received) const {
    active_connections = m_active_connections;
    pending_operations = m_pending_operations;
    bytes_sent = m_bytes_sent;
    bytes_received = m_bytes_received;
}

} // namespace network
} // namespace parallelstone

#endif // __linux__