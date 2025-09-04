/**
 * @file kqueue.cpp
 * @brief macOS kqueue network implementation
 * @details This file implements the macOS-specific network layer using
 *          kqueue for efficient event-driven I/O operations.
 * @author @logpacket (https://github.com/logpacket)
 * @version 1.0.0
 * @date 2025-07-16
 */

#ifdef __APPLE__

#include "network/kqueue.hpp"
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

KqueueNetworkCore::KqueueNetworkCore()
    : m_initialized(false)
    , m_config()
    , m_kqueue_fd(INVALID_FD)
    , m_active_connections(0)
    , m_pending_operations(0)
    , m_bytes_sent(0)
    , m_bytes_received(0) {
    m_events.reserve(MAX_EVENTS);
}

KqueueNetworkCore::~KqueueNetworkCore() {
    if (m_initialized) {
        Shutdown();
    }
}

NetworkResult KqueueNetworkCore::Initialize(const NetworkConfig& config) {
    if (m_initialized) {
        return NetworkResult::SUCCESS;
    }

    m_config = config;

    // Create kqueue instance
    m_kqueue_fd = kqueue();
    if (m_kqueue_fd < 0) {
        return NetworkResult::ERROR_INITIALIZATION;
    }

    m_initialized = true;
    return NetworkResult::SUCCESS;
}

void KqueueNetworkCore::Shutdown() {
    if (!m_initialized) {
        return;
    }

    // Close all tracked sockets and clean up requests
    for (auto& pair : m_requests) {
        RemoveSocket(pair.first);
    }
    m_requests.clear();

    // Close kqueue
    if (m_kqueue_fd >= 0) {
        close(m_kqueue_fd);
        m_kqueue_fd = INVALID_FD;
    }

    m_initialized = false;
}

NetworkResult KqueueNetworkCore::CreateSocket(SocketType& socket, int family, int type) {
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

NetworkResult KqueueNetworkCore::BindSocket(SocketType socket, const void* addr, size_t addr_len) {
    if (!m_initialized) {
        return NetworkResult::ERROR_INITIALIZATION;
    }

    int sock = static_cast<int>(socket);
    int result = bind(sock, static_cast<const struct sockaddr*>(addr), 
                     static_cast<socklen_t>(addr_len));
    
    return (result == 0) ? NetworkResult::SUCCESS : NetworkResult::ERROR_BIND;
}

NetworkResult KqueueNetworkCore::ListenSocket(SocketType socket, int backlog) {
    if (!m_initialized) {
        return NetworkResult::ERROR_INITIALIZATION;
    }

    int sock = static_cast<int>(socket);
    int result = listen(sock, backlog);
    
    return (result == 0) ? NetworkResult::SUCCESS : NetworkResult::ERROR_LISTEN;
}

NetworkResult KqueueNetworkCore::CloseSocket(SocketType socket) {
    if (!m_initialized) {
        return NetworkResult::ERROR_INITIALIZATION;
    }

    int sock = static_cast<int>(socket);
    RemoveSocket(sock);
    int result = close(sock);
    
    if (result == 0) {
        m_active_connections--;
        return NetworkResult::SUCCESS;
    }
    
    return NetworkResult::ERROR_UNKNOWN;
}

NetworkResult KqueueNetworkCore::RegisterBuffer(NetworkBuffer& buffer) {
    if (!m_initialized) {
        return NetworkResult::ERROR_INITIALIZATION;
    }

    if (!buffer.data || buffer.capacity == 0) {
        return NetworkResult::ERROR_INVALID_ARGUMENT;
    }

    // For kqueue, we don't need to register buffers explicitly
    // Just track them for reference
    buffer.buffer_id = buffer.data;
    
    return NetworkResult::SUCCESS;
}

NetworkResult KqueueNetworkCore::DeregisterBuffer(const NetworkBuffer& buffer) {
    if (!m_initialized) {
        return NetworkResult::ERROR_INITIALIZATION;
    }

    // Nothing special needed for kqueue buffer deregistration
    return NetworkResult::SUCCESS;
}

NetworkResult KqueueNetworkCore::AsyncAccept(SocketType listen_socket, 
                                            std::function<void(NetworkResult, SocketType)> callback) {
    if (!m_initialized) {
        return NetworkResult::ERROR_INITIALIZATION;
    }

    int listen_fd = static_cast<int>(listen_socket);
    
    KqueueRequest* request = CreateRequest(KqueueEventType::ACCEPT, listen_fd);
    if (!request) {
        return NetworkResult::ERROR_BUFFER_FULL;
    }

    request->callback = [callback](int result, ssize_t value) {
        if (result >= 0) {
            callback(NetworkResult::SUCCESS, static_cast<SocketType>(value));
        } else {
            callback(NetworkResult::ERROR_ACCEPT, 0);
        }
    };

    // Add read event for accept
    if (!AddEvent(listen_fd, EVFILT_READ, EV_ADD | EV_ENABLE, request)) {
        return NetworkResult::ERROR_ACCEPT;
    }

    m_pending_operations++;
    return NetworkResult::SUCCESS;
}

NetworkResult KqueueNetworkCore::AsyncReceive(SocketType socket, Buffer& buffer,
                                             std::function<void(NetworkResult, size_t)> callback) {
    if (!m_initialized) {
        return NetworkResult::ERROR_INITIALIZATION;
    }

    int sock = static_cast<int>(socket);
    
    KqueueRequest* request = CreateRequest(KqueueEventType::READ, sock);
    if (!request) {
        return NetworkResult::ERROR_BUFFER_FULL;
    }

    request->buffer_ptr = &buffer;
    request->callback = [callback](int result, ssize_t bytes) {
        if (result >= 0) {
            callback(NetworkResult::SUCCESS, static_cast<size_t>(bytes));
        } else {
            NetworkResult error = (result == -ECONNRESET) ? 
                                 NetworkResult::ERROR_CONNECTION_CLOSED : 
                                 NetworkResult::ERROR_RECEIVE;
            callback(error, 0);
        }
    };

    // Add read event
    if (!AddEvent(sock, EVFILT_READ, EV_ADD | EV_ENABLE, request)) {
        return NetworkResult::ERROR_RECEIVE;
    }

    m_pending_operations++;
    return NetworkResult::SUCCESS;
}

NetworkResult KqueueNetworkCore::AsyncSend(SocketType socket, Buffer& buffer,
                                          std::function<void(NetworkResult, size_t)> callback) {
    if (!m_initialized) {
        return NetworkResult::ERROR_INITIALIZATION;
    }

    int sock = static_cast<int>(socket);
    
    KqueueRequest* request = CreateRequest(KqueueEventType::WRITE, sock);
    if (!request) {
        return NetworkResult::ERROR_BUFFER_FULL;
    }

    request->buffer_ptr = &buffer;
    request->callback = [callback](int result, ssize_t bytes) {
        if (result >= 0) {
            callback(NetworkResult::SUCCESS, static_cast<size_t>(bytes));
        } else {
            NetworkResult error = (result == -EPIPE || result == -ECONNRESET) ? 
                                 NetworkResult::ERROR_CONNECTION_CLOSED : 
                                 NetworkResult::ERROR_SEND;
            callback(error, 0);
        }
    };

    // Add write event
    if (!AddEvent(sock, EVFILT_WRITE, EV_ADD | EV_ENABLE, request)) {
        return NetworkResult::ERROR_SEND;
    }

    m_pending_operations++;
    return NetworkResult::SUCCESS;
}

size_t KqueueNetworkCore::ProcessCompletions(int timeout_ms) {
    if (!m_initialized) {
        return 0;
    }

    return ProcessEvents(timeout_ms);
}

void KqueueNetworkCore::ProcessEvents(int timeout_ms) {
    if (!m_initialized) {
        return;
    }

    struct timespec timeout;
    struct timespec* timeout_ptr = nullptr;
    
    if (timeout_ms >= 0) {
        timeout.tv_sec = timeout_ms / 1000;
        timeout.tv_nsec = (timeout_ms % 1000) * 1000000;
        timeout_ptr = &timeout;
    }

    m_events.resize(MAX_EVENTS);
    int nevents = kevent(m_kqueue_fd, nullptr, 0, m_events.data(), MAX_EVENTS, timeout_ptr);
    
    if (nevents < 0) {
        if (errno != EINTR) {
            spdlog::error("kevent error: {}", strerror(errno));
        }
        return;
    }

    for (int i = 0; i < nevents; ++i) {
        struct kevent& event = m_events[i];
        KqueueRequest* request = static_cast<KqueueRequest*>(event.udata);
        
        if (!request) {
            continue;
        }

        ssize_t result = 0;
        NetworkResult net_result = NetworkResult::SUCCESS;

        if (event.flags & EV_ERROR) {
            // Error occurred
            result = -static_cast<ssize_t>(event.data);
            net_result = NetworkResult::ERROR_UNKNOWN;
        } else if (request->type == KqueueEventType::ACCEPT) {
            // Accept new connection
            sockaddr_storage addr;
            socklen_t addr_len = sizeof(addr);
            int new_sock = accept(request->fd, reinterpret_cast<sockaddr*>(&addr), &addr_len);
            
            if (new_sock >= 0) {
                SetNonBlocking(new_sock);
                result = new_sock;
                m_active_connections++;
            } else {
                result = -errno;
                net_result = NetworkResult::ERROR_ACCEPT;
            }
        } else if (request->type == KqueueEventType::READ) {
            // Read data
            ssize_t bytes_read = recv(request->fd, request->buffer_ptr->write_ptr(), request->buffer_ptr->writable_bytes(), 0);
            
            if (bytes_read > 0) {
                request->buffer_ptr->advance_write_position(bytes_read);
                result = bytes_read;
                m_bytes_received += bytes_read;
            } else if (bytes_read == 0) {
                result = 0; // Connection closed
                net_result = NetworkResult::ERROR_CONNECTION_CLOSED;
            } else {
                result = -errno;
                net_result = NetworkResult::ERROR_RECEIVE;
            }
        } else if (request->type == KqueueEventType::WRITE) {
            // Send data
            ssize_t bytes_sent = send(request->fd, request->buffer_ptr->data(), request->buffer_ptr->readable_bytes(), 0);
            
            if (bytes_sent > 0) {
                request->buffer_ptr->skip_bytes(bytes_sent);
                result = bytes_sent;
                m_bytes_sent += bytes_sent;
            } else {
                result = -errno;
                net_result = NetworkResult::ERROR_SEND;
            }
        }

        // Execute callback
        if (request->callback) {
            request->callback(static_cast<int>(result), result);
        }

        // Clean up request
        RemoveEvent(request->fd, event.filter);
        m_requests.erase(request->fd);
        m_pending_operations--;
    }
}

void KqueueNetworkCore::SetNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags >= 0) {
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }
}

void KqueueNetworkCore::RemoveSocket(int fd) {
    auto it = m_requests.find(fd);
    if (it != m_requests.end()) {
        RemoveEvent(fd, EVFILT_READ);
        RemoveEvent(fd, EVFILT_WRITE);
        m_requests.erase(it);
    }
}

bool KqueueNetworkCore::MonitorClose(int socket_fd, std::function<void(int, ssize_t)> callback) {
    KqueueRequest* request = CreateRequest(KqueueEventType::CLOSE, socket_fd);
    if (!request) {
        return false;
    }

    request->callback = std::move(callback);
    
    // Monitor for socket closure (using EVFILT_READ with EV_EOF)
    return AddEvent(socket_fd, EVFILT_READ, EV_ADD | EV_ENABLE, request);
}

bool KqueueNetworkCore::AddEvent(int fd, short filter, unsigned short flags, void* udata) {
    struct kevent event;
    EV_SET(&event, fd, filter, flags, 0, 0, udata);
    
    int result = kevent(m_kqueue_fd, &event, 1, nullptr, 0, nullptr);
    return result == 0;
}

bool KqueueNetworkCore::RemoveEvent(int fd, short filter) {
    struct kevent event;
    EV_SET(&event, fd, filter, EV_DELETE, 0, 0, nullptr);
    
    int result = kevent(m_kqueue_fd, &event, 1, nullptr, 0, nullptr);
    return result == 0;
}

KqueueRequest* KqueueNetworkCore::CreateRequest(KqueueEventType type, int fd) {
    auto it = m_requests.find(fd);
    if (it != m_requests.end()) {
        // Reuse existing request for this fd
        it->second->type = type;
        it->second->buffer_ptr = nullptr;
        it->second->callback = nullptr;
        return it->second.get();
    }

    // Create new request
    auto request = std::make_unique<KqueueRequest>();
    request->type = type;
    request->fd = fd;
    request->buffer_ptr = nullptr;
    memset(&request->addr, 0, sizeof(request->addr));

    KqueueRequest* request_ptr = request.get();
    m_requests[fd] = std::move(request);
    
    return request_ptr;
}

void KqueueNetworkCore::GetStatistics(size_t& active_connections, size_t& pending_operations,
                                     uint64_t& bytes_sent, uint64_t& bytes_received) const {
    active_connections = m_active_connections;
    pending_operations = m_pending_operations;
    bytes_sent = m_bytes_sent;
    bytes_received = m_bytes_received;
}

} // namespace network
} // namespace parallelstone

#endif // __APPLE__