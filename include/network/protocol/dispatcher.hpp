#pragma once

#include <memory>
#include <cstdint>
#include "protocol/version_config.hpp"
#include "network/buffer.hpp"
#include "network/packet_view.hpp"
#include "server/session.hpp"

namespace protocol {

// Type aliases for compatibility with existing code
using SessionState = parallelstone::server::SessionState;
using Session = parallelstone::server::Session;
using Buffer = parallelstone::network::Buffer;
using PacketView = parallelstone::network::PacketView;

/**
 * @brief Packet dispatcher for routing packets based on protocol state
 * @details The dispatcher routes incoming packets to appropriate handler functions
 *          based on the current session state and packet ID.
 */
class PacketDispatcher {
public:
    /**
     * @brief Constructor
     */
    PacketDispatcher();

    /**
     * @brief Destructor
     */
    ~PacketDispatcher() = default;

    // Non-copyable, non-movable
    PacketDispatcher(const PacketDispatcher&) = delete;
    PacketDispatcher& operator=(const PacketDispatcher&) = delete;
    PacketDispatcher(PacketDispatcher&&) = delete;
    PacketDispatcher& operator=(PacketDispatcher&&) = delete;

    /**
     * @brief Dispatch a packet to the appropriate handler based on state and packet ID
     * @param state The current session state
     * @param packet_id The packet ID
     * @param session The session that received the packet
     * @param buffer The packet data buffer
     * @return true if the packet was handled, false otherwise
     */
    bool DispatchPacket(SessionState state, std::uint8_t packet_id, 
                       std::shared_ptr<Session> session, PacketView& buffer);

private:
    /**
     * @brief Dispatch handshaking packets
     * @param packet_id The packet ID
     * @param session The session that received the packet
     * @param buffer The packet data buffer
     * @return true if the packet was handled
     */
    bool DispatchHandshaking(std::uint8_t packet_id, std::shared_ptr<Session> session, PacketView& buffer);

    /**
     * @brief Dispatch status packets
     * @param packet_id The packet ID
     * @param session The session that received the packet
     * @param buffer The packet data buffer
     * @return true if the packet was handled
     */
    bool DispatchStatus(std::uint8_t packet_id, std::shared_ptr<Session> session, PacketView& buffer);

    /**
     * @brief Dispatch login packets
     * @param packet_id The packet ID
     * @param session The session that received the packet
     * @param buffer The packet data buffer
     * @return true if the packet was handled
     */
    bool DispatchLogin(std::uint8_t packet_id, std::shared_ptr<Session> session, PacketView& buffer);

    /**
     * @brief Dispatch configuration packets
     * @param packet_id The packet ID
     * @param session The session that received the packet
     * @param buffer The packet data buffer
     * @return true if the packet was handled
     */
    bool DispatchConfiguration(std::uint8_t packet_id, std::shared_ptr<Session> session, PacketView& buffer);

    /**
     * @brief Dispatch play packets
     * @param packet_id The packet ID
     * @param session The session that received the packet
     * @param buffer The packet data buffer
     * @return true if the packet was handled
     */
    bool DispatchPlay(std::uint8_t packet_id, std::shared_ptr<Session> session, PacketView& buffer);
};

/**
 * @brief Get the global packet dispatcher instance
 * @return Reference to the singleton PacketDispatcher
 */
PacketDispatcher& GetPacketDispatcher();

} // namespace protocol