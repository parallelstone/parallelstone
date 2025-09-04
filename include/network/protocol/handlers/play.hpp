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
 * @brief Handles packets for the Play state.
 */
class PlayHandler {
public:
    PlayHandler() = default;
    ~PlayHandler() = default;

    PlayHandler(const PlayHandler&) = delete;
    PlayHandler& operator=(const PlayHandler&) = delete;

    /**
     * @brief Handle any packet in the Play state.
     * @param packet_id The packet ID.
     * @param session The client session.
     * @param view The packet data view.
     * @return True if the packet was handled successfully.
     */
    bool HandlePacket(uint8_t packet_id, std::shared_ptr<Session> session, PacketView& view);

private:
    bool HandleConfirmTeleportation(std::shared_ptr<Session> session, PacketView& view);
    bool HandleKeepAlive(std::shared_ptr<Session> session, PacketView& view);
    bool HandleSetPlayerPosition(std::shared_ptr<Session> session, PacketView& view);
    bool HandleSetPlayerPositionAndRotation(std::shared_ptr<Session> session, PacketView& view);
    bool HandleSetPlayerRotation(std::shared_ptr<Session> session, PacketView& view);
    bool HandleSetPlayerOnGround(std::shared_ptr<Session> session, PacketView& view);
    bool HandleChatMessage(std::shared_ptr<Session> session, PacketView& view);
    bool HandleClientInformation(std::shared_ptr<Session> session, PacketView& view);
    bool HandlePlayerAction(std::shared_ptr<Session> session, PacketView& view);
    bool HandleUseItemOn(std::shared_ptr<Session> session, PacketView& view);
    bool HandleUseItem(std::shared_ptr<Session> session, PacketView& view);
    bool HandleSwingArm(std::shared_ptr<Session> session, PacketView& view);
};

PlayHandler& GetPlayHandler();

} // namespace protocol