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
 * @brief Handles packets for the Status state.
 */
class StatusHandler {
public:
    StatusHandler() = default;
    ~StatusHandler() = default;

    StatusHandler(const StatusHandler&) = delete;
    StatusHandler& operator=(const StatusHandler&) = delete;

    /**
     * @brief Handles the Status Request packet (0x00).
     * @param session The client session.
     * @param view The packet data view.
     * @return True if the packet was handled successfully.
     */
    bool HandleStatusRequest(std::shared_ptr<Session> session, PacketView& view);

    /**
     * @brief Handles the Ping Request packet (0x01).
     * @param session The client session.
     * @param view The packet data view.
     * @return True if the packet was handled successfully.
     */
    bool HandlePingRequest(std::shared_ptr<Session> session, PacketView& view);
};

/**
 * @brief Get the global status handler instance.
 * @return Reference to the singleton StatusHandler.
 */
StatusHandler& GetStatusHandler();

} // namespace protocol
