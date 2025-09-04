#pragma once

#include <memory>
#include "server/session.hpp"
#include "network/packet_view.hpp"

namespace protocol {

// Forward declarations for types from other namespaces
using Session = parallelstone::server::Session;
using PacketView = parallelstone::network::PacketView;
using DisconnectReason = parallelstone::server::DisconnectReason;
using Buffer = parallelstone::network::Buffer;

/**
 * @brief Handles packets for the Handshaking state.
 */
class HandshakingHandler {
public:
    HandshakingHandler() = default;
    ~HandshakingHandler() = default;

    // Non-copyable, non-movable
    HandshakingHandler(const HandshakingHandler&) = delete;
    HandshakingHandler& operator=(const HandshakingHandler&) = delete;

    /**
     * @brief Handles the main Handshake packet (0x00).
     * @param session The client session.
     * @param view The packet data view.
     * @return True if the packet was handled successfully.
     */
    bool HandleHandshake(std::shared_ptr<Session> session, PacketView& view);

    /**
     * @brief Handles the Legacy Server List Ping packet (0xFE).
     * @param session The client session.
     * @param view The packet data view.
     * @return True if the packet was handled successfully.
     */
    bool HandleLegacyServerListPing(std::shared_ptr<Session> session, PacketView& view);
};

/**
 * @brief Get the global handshaking handler instance.
 * @return Reference to the singleton HandshakingHandler.
 */
HandshakingHandler& GetHandshakingHandler();

} // namespace protocol