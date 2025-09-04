/**
 * @file rio.hpp
 * @brief Windows RIO (Registered I/O) implementation for high-performance networking
 * @details This header implements the Windows-specific network layer using
 *          RIO (Registered I/O) for maximum performance on Windows platforms.
 *          RIO provides zero-copy networking with kernel bypass capabilities.
 * @author @logpacket (https://github.com/logpacket)
 * @version 1.0.0
 * @date 2025-07-16
 * @note Only available on Windows platforms with RIO support
 */

#pragma once
#ifndef PARALLELSTONE_RIO_NETWORK_HPP
#define PARALLELSTONE_RIO_NETWORK_HPP

#ifdef _WIN32

#include "network/core.hpp"
#include <winsock2.h>
#include <mswsock.h>
#include <vector>
#include <memory>
#include <unordered_map>
#include <mutex>

namespace parallelstone {
namespace network {

// Forward declaration
class Buffer;

/**
 * @class RIONetworkCore
 * @brief Windows RIO (Registered I/O) network implementation
 * @details Implements the INetworkCore interface using Windows RIO for
 *          high-performance, low-latency networking.
 */
class RIONetworkCore : public INetworkCore {
public:
    RIONetworkCore();
    ~RIONetworkCore() override;

    // Disable copy and move
    RIONetworkCore(const RIONetworkCore&) = delete;
    RIONetworkCore& operator=(const RIONetworkCore&) = delete;

    // INetworkCore interface implementation
    NetworkResult Initialize(const NetworkConfig& config) override;
    void Shutdown() override;
    bool IsInitialized() const override;
    NetworkResult CreateSocket(SocketType& socket, int family, int type) override;
    NetworkResult BindSocket(SocketType socket, const void* addr, size_t addr_len) override;
    NetworkResult ListenSocket(SocketType socket, int backlog) override;
    NetworkResult CloseSocket(SocketType socket) override;
    NetworkResult AsyncAccept(SocketType listen_socket, std::function<void(NetworkResult, SocketType)> callback) override;
    NetworkResult AsyncReceive(SocketType socket, Buffer& buffer, std::function<void(NetworkResult, size_t)> callback) override;
    NetworkResult AsyncSend(SocketType socket, Buffer& buffer, std::function<void(NetworkResult, size_t)> callback) override;
    size_t ProcessCompletions(int timeout_ms) override;
    const NetworkConfig& GetConfig() const override;
    const char* GetImplementationName() const override;
    void GetStatistics(size_t& active_connections, size_t& pending_operations,
                       uint64_t& bytes_sent, uint64_t& bytes_received) const override;

private:
    /**
     * @struct RIORequest
     * @brief Context for a single RIO operation
     */
    struct RIORequest {
        OVERLAPPED overlapped;
        SocketType socket;
        SocketType accept_socket; // For AcceptEx
        Buffer* buffer;
        std::function<void(NetworkResult, size_t)> callback_bytes;
        std::function<void(NetworkResult, SocketType)> callback_socket;
        enum class Type { Accept, Receive, Send } type;
    };

    bool LoadRIOFunctions();
    RIO_RQ GetRequestQueue(SOCKET socket);

    bool m_initialized;
    NetworkConfig m_config;
    WSAData m_wsa_data;
    RIO_EXTENSION_FUNCTION_TABLE m_rio_fns;
    LPFN_ACCEPTEX m_accept_ex;

    // RIO uses a single registered buffer pool for zero-copy
    std::vector<char> m_buffer_pool;
    RIO_BUFFERID m_buffer_id;

    RIO_CQ m_completion_queue;
    
    std::mutex m_request_queues_mutex;
    std::unordered_map<SOCKET, RIO_RQ> m_request_queues;

    std::mutex m_pending_requests_mutex;
    std::vector<std::unique_ptr<RIORequest>> m_pending_requests;
    
    // Statistics
    std::atomic<size_t> m_active_connections;
    std::atomic<size_t> m_pending_operations;
    std::atomic<uint64_t> m_bytes_sent;
    std::atomic<uint64_t> m_bytes_received;
};

} // namespace network
} // namespace parallelstone

#endif // _WIN32
#endif // PARALLELSTONE_RIO_NETWORK_HPP