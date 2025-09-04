/**
 * @file rio.cpp
 * @brief Windows RIO (Registered I/O) network implementation
 * @details This file implements the Windows-specific network layer using
 *          RIO for high-performance networking with zero-copy capabilities.
 * @author @logpacket (https://github.com/logpacket)
 * @version 1.0.0
 * @date 2025-07-16
 */

#ifdef _WIN32

#include "network/rio.hpp"
#include "network/buffer.hpp"
#include <stdexcept>
#include <vector>
#include <iostream>

#pragma comment(lib, "ws2_32.lib")

namespace parallelstone {
namespace network {

RIONetworkCore::RIONetworkCore() 
    : m_initialized(false), m_buffer_id(RIO_INVALID_BUFFERID), 
      m_completion_queue(RIO_INVALID_CQ), m_accept_ex(nullptr) {}

RIONetworkCore::~RIONetworkCore() {
    if (m_initialized) {
        Shutdown();
    }
}

NetworkResult RIONetworkCore::Initialize(const NetworkConfig& config) {
    if (m_initialized) return NetworkResult::SUCCESS;

    m_config = config;

    if (WSAStartup(MAKEWORD(2, 2), &m_wsa_data) != 0) {
        return NetworkResult::ERROR_INITIALIZATION;
    }

    if (!LoadRIOFunctions()) {
        WSACleanup();
        return NetworkResult::ERROR_INITIALIZATION;
    }

    m_completion_queue = m_rio_fns.RIOCreateCompletionQueue(static_cast<DWORD>(m_config.queue_depth), nullptr);
    if (m_completion_queue == RIO_INVALID_CQ) {
        WSACleanup();
        return NetworkResult::ERROR_INITIALIZATION;
    }

    m_buffer_pool.resize(m_config.buffer_pool_size);
    m_buffer_id = m_rio_fns.RIORegisterBuffer(m_buffer_pool.data(), static_cast<DWORD>(m_buffer_pool.size()));
    if (m_buffer_id == RIO_INVALID_BUFFERID) {
        m_rio_fns.RIOCloseCompletionQueue(m_completion_queue);
        WSACleanup();
        return NetworkResult::ERROR_INITIALIZATION;
    }

    m_initialized = true;
    return NetworkResult::SUCCESS;
}

void RIONetworkCore::Shutdown() {
    if (!m_initialized) return;

    m_request_queues.clear();

    if (m_buffer_id != RIO_INVALID_BUFFERID) {
        m_rio_fns.RIODeregisterBuffer(m_buffer_id);
        m_buffer_id = RIO_INVALID_BUFFERID;
    }
    m_buffer_pool.clear();

    if (m_completion_queue != RIO_INVALID_CQ) {
        m_rio_fns.RIOCloseCompletionQueue(m_completion_queue);
        m_completion_queue = RIO_INVALID_CQ;
    }

    WSACleanup();
    m_initialized = false;
}

bool RIONetworkCore::LoadRIOFunctions() {
    SOCKET sock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (sock == INVALID_SOCKET) return false;

    GUID rio_guid = WSAID_MULTIPLE_RIO;
    DWORD bytes = 0;
    if (WSAIoctl(sock, SIO_GET_MULTIPLE_EXTENSION_FUNCTION_POINTER, &rio_guid, sizeof(rio_guid), 
                 &m_rio_fns, sizeof(m_rio_fns), &bytes, NULL, NULL) != 0) {
        closesocket(sock);
        return false;
    }

    GUID acceptex_guid = WSAID_ACCEPTEX;
    bytes = 0;
    if(WSAIoctl(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &acceptex_guid, sizeof(acceptex_guid),
                &m_accept_ex, sizeof(m_accept_ex), &bytes, NULL, NULL) != 0) {
        closesocket(sock);
        return false;
    }

    closesocket(sock);
    return true;
}

NetworkResult RIONetworkCore::CreateSocket(SocketType& out_socket, int family, int type) {
    SOCKET sock = WSASocket(family, type, IPPROTO_TCP, NULL, 0, WSA_FLAG_REGISTERED_IO | WSA_FLAG_OVERLAPPED);
    if (sock == INVALID_SOCKET) {
        return NetworkResult::ERROR_SOCKET_CREATION;
    }
    out_socket = static_cast<SocketType>(sock);
    return NetworkResult::SUCCESS;
}

NetworkResult RIONetworkCore::BindSocket(SocketType socket, const void* addr, size_t addr_len) {
    if (bind(static_cast<SOCKET>(socket), (const sockaddr*)addr, (int)addr_len) != 0) {
        return NetworkResult::ERROR_BIND;
    }
    return NetworkResult::SUCCESS;
}

NetworkResult RIONetworkCore::ListenSocket(SocketType socket, int backlog) {
    if (listen(static_cast<SOCKET>(socket), backlog) != 0) {
        return NetworkResult::ERROR_LISTEN;
    }
    return NetworkResult::SUCCESS;
}

NetworkResult RIONetworkCore::CloseSocket(SocketType socket) {
    SOCKET sock = static_cast<SOCKET>(socket);
    {
        std::lock_guard<std::mutex> lock(m_request_queues_mutex);
        m_request_queues.erase(sock);
    }
    closesocket(sock);
    return NetworkResult::SUCCESS;
}

NetworkResult RIONetworkCore::AsyncAccept(SocketType listen_socket, std::function<void(NetworkResult, SocketType)> callback) {
    SocketType new_socket_handle;
    CreateSocket(new_socket_handle, AF_INET, SOCK_STREAM);
    SOCKET new_sock = static_cast<SOCKET>(new_socket_handle);

    auto* req = new RIORequest{ {}, listen_socket, new_sock, nullptr, nullptr, callback, RIORequest::Type::Accept };
    
    DWORD bytes_received = 0;
    if (!m_accept_ex(static_cast<SOCKET>(listen_socket), new_sock, req->buffer->raw_data(), 0, 
                     sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16, &bytes_received, &req->overlapped)) {
        if (WSAGetLastError() != ERROR_IO_PENDING) {
            delete req;
            closesocket(new_sock);
            return NetworkResult::ERROR_ACCEPT;
        }
    }
    
    std::lock_guard<std::mutex> lock(m_pending_requests_mutex);
    m_pending_requests.push_back(std::unique_ptr<RIORequest>(req));
    return NetworkResult::SUCCESS;
}

NetworkResult RIONetworkCore::AsyncReceive(SocketType socket, Buffer& buffer, std::function<void(NetworkResult, size_t)> callback) {
    auto* req = new RIORequest{ {}, socket, 0, &buffer, callback, nullptr, RIORequest::Type::Receive };
    
    RIO_RQ rq = GetRequestQueue(static_cast<SOCKET>(socket));
    if (rq == RIO_INVALID_RQ) {
        delete req;
        return NetworkResult::ERROR_UNKNOWN;
    }

    RIO_BUF rio_buf;
    rio_buf.BufferId = m_buffer_id;
    rio_buf.Offset = reinterpret_cast<const char*>(buffer.write_ptr()) - m_buffer_pool.data();
    rio_buf.Length = static_cast<DWORD>(buffer.writable_bytes());

    if (!m_rio_fns.RIOReceive(rq, &rio_buf, 1, 0, &req->overlapped)) {
        if (WSAGetLastError() != WSA_IO_PENDING) {
            delete req;
            return NetworkResult::ERROR_RECEIVE;
        }
    }
    
    std::lock_guard<std::mutex> lock(m_pending_requests_mutex);
    m_pending_requests.push_back(std::unique_ptr<RIORequest>(req));
    return NetworkResult::SUCCESS;
}

NetworkResult RIONetworkCore::AsyncSend(SocketType socket, Buffer& buffer, std::function<void(NetworkResult, size_t)> callback) {
    auto* req = new RIORequest{ {}, socket, 0, &buffer, callback, nullptr, RIORequest::Type::Send };

    RIO_RQ rq = GetRequestQueue(static_cast<SOCKET>(socket));
    if (rq == RIO_INVALID_RQ) {
        delete req;
        return NetworkResult::ERROR_UNKNOWN;
    }

    RIO_BUF rio_buf;
    rio_buf.BufferId = m_buffer_id;
    rio_buf.Offset = reinterpret_cast<const char*>(buffer.data()) - m_buffer_pool.data();
    rio_buf.Length = static_cast<DWORD>(buffer.readable_bytes());

    if (!m_rio_fns.RIOSend(rq, &rio_buf, 1, 0, &req->overlapped)) {
        if (WSAGetLastError() != WSA_IO_PENDING) {
            delete req;
            return NetworkResult::ERROR_SEND;
        }
    }

    std::lock_guard<std::mutex> lock(m_pending_requests_mutex);
    m_pending_requests.push_back(std::unique_ptr<RIORequest>(req));
    return NetworkResult::SUCCESS;
}

size_t RIONetworkCore::ProcessCompletions(int timeout_ms) {
    RIORESULT results[128];
    ULONG num_results = m_rio_fns.RIODequeueCompletion(m_completion_queue, results, 128);

    if (num_results == RIO_CORRUPT_CQ) {
        std::cerr << "RIO CQ is corrupt" << std::endl;
        return 0;
    }
    
    if (num_results == 0) {
        // Could implement waiting logic with RIONotify if timeout_ms is not 0
        return 0;
    }

    for (ULONG i = 0; i < num_results; ++i) {
        RIORequest* req = reinterpret_cast<RIORequest*>(results[i].RequestContext);
        
        if (results[i].Status == 0) {
            switch (req->type) {
                case RIORequest::Type::Accept:
                    if (req->callback_socket) req->callback_socket(NetworkResult::SUCCESS, req->accept_socket);
                    break;
                case RIORequest::Type::Receive:
                    req->buffer->advance_write_position(results[i].BytesTransferred);
                    if (req->callback_bytes) req->callback_bytes(NetworkResult::SUCCESS, results[i].BytesTransferred);
                    break;
                case RIORequest::Type::Send:
                    req->buffer->skip_bytes(results[i].BytesTransferred);
                    if (req->callback_bytes) req->callback_bytes(NetworkResult::SUCCESS, results[i].BytesTransferred);
                    break;
            }
        } else {
            // Handle error
            if (req->callback_socket) req->callback_socket(NetworkResult::ERROR_UNKNOWN, 0);
            if (req->callback_bytes) req->callback_bytes(NetworkResult::ERROR_UNKNOWN, 0);
        }
    }

    // This cleanup is simplified. A real implementation needs a more robust way
    // to manage the lifecycle of RIORequest objects.
    std::lock_guard<std::mutex> lock(m_pending_requests_mutex);
    m_pending_requests.erase(std::remove_if(m_pending_requests.begin(), m_pending_requests.end(), 
        [results, num_results](const auto& p) {
            for (ULONG i = 0; i < num_results; ++i) {
                if (p.get() == reinterpret_cast<RIORequest*>(results[i].RequestContext)) {
                    return true;
                }
            }
            return false;
        }), m_pending_requests.end());

    return num_results;
}

RIO_RQ RIONetworkCore::GetRequestQueue(SOCKET socket) {
    std::lock_guard<std::mutex> lock(m_request_queues_mutex);
    auto it = m_request_queues.find(socket);
    if (it != m_request_queues.end()) {
        return it->second;
    }

    RIO_RQ rq = m_rio_fns.RIOCreateRequestQueue(socket, m_config.queue_depth / 2, 1, m_config.queue_depth / 2, 1, m_completion_queue, m_completion_queue, (void*)socket);
    if (rq != RIO_INVALID_RQ) {
        m_request_queues[socket] = rq;
    }
    return rq;
}

bool RIONetworkCore::IsInitialized() const {
    return m_initialized;
}

const NetworkConfig& RIONetworkCore::GetConfig() const {
    return m_config;
}

const char* RIONetworkCore::GetImplementationName() const {
    return "RIO";
}

void RIONetworkCore::GetStatistics(size_t& active_connections, size_t& pending_operations,
                                 uint64_t& bytes_sent, uint64_t& bytes_received) const {
    active_connections = m_active_connections;
    pending_operations = m_pending_operations;
    bytes_sent = m_bytes_sent;
    bytes_received = m_bytes_received;
}

} // namespace network
} // namespace parallelstone

#endif // _WIN32