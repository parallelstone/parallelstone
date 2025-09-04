#include "protocol/dispatcher.hpp"
#include "protocol/version_config.hpp"
#include "protocol/handlers/handshaking.hpp"
#include "protocol/handlers/status.hpp"
#include "protocol/handlers/login.hpp"
#include "protocol/handlers/configuration.hpp"
#include "protocol/handlers/play.hpp"
#include <spdlog/spdlog.h>
#include <iostream>
#include <cstdint>

namespace protocol {

// ============================================================================
// CONSTRUCTOR
// ============================================================================

PacketDispatcher::PacketDispatcher() {
}

// ============================================================================
// GLOBAL DISPATCHER INSTANCE
// ============================================================================

PacketDispatcher& GetPacketDispatcher() {
    static PacketDispatcher instance;
    return instance;
}

// ============================================================================
// MAIN DISPATCH LOGIC
// ============================================================================

bool PacketDispatcher::DispatchPacket(SessionState state, std::uint8_t packet_id, 
                                     std::shared_ptr<Session> session, PacketView& buffer) {
    // Route to appropriate state handler
    switch (state) {
        case SessionState::HANDSHAKING:
            return DispatchHandshaking(packet_id, session, buffer);
            
        case SessionState::STATUS:
            return DispatchStatus(packet_id, session, buffer);
            
        case SessionState::LOGIN:
            return DispatchLogin(packet_id, session, buffer);
            
        case SessionState::CONFIGURATION:
            return DispatchConfiguration(packet_id, session, buffer);
            
        case SessionState::PLAY:
            return DispatchPlay(packet_id, session, buffer);
            
        default:
            spdlog::warn("Packet received for session {} in unhandled state {}", 
                         session->GetSessionId(), static_cast<int>(state));
            return false;
    }
}

// ============================================================================
// STATE DISPATCHERS
// ============================================================================

bool PacketDispatcher::DispatchHandshaking(std::uint8_t packet_id, std::shared_ptr<Session> session, PacketView& buffer) {
    auto& handler = GetHandshakingHandler();
    switch (packet_id) {
        case 0x00: // Handshake
            return handler.HandleHandshake(session, buffer);
            
        case 0xFE: // Legacy Server List Ping
            return handler.HandleLegacyServerListPing(session, buffer);
            
        default:
            spdlog::warn("Unknown packet ID 0x{:02X} in HANDSHAKING state for session {}", 
                         packet_id, session->GetSessionId());
            return false;
    }
}

bool PacketDispatcher::DispatchStatus(std::uint8_t packet_id, std::shared_ptr<Session> session, PacketView& buffer) {
    auto& handler = GetStatusHandler();
    switch (packet_id) {
        case 0x00: // Status Request
            return handler.HandleStatusRequest(session, buffer);
            
        case 0x01: // Ping Request
            return handler.HandlePingRequest(session, buffer);
            
        default:
            spdlog::warn("Unknown packet ID 0x{:02X} in STATUS state for session {}", 
                         packet_id, session->GetSessionId());
            return false;
    }
}

bool PacketDispatcher::DispatchLogin(std::uint8_t packet_id, std::shared_ptr<Session> session, PacketView& buffer) {
    auto& handler = GetLoginHandler();
    switch (packet_id) {
        case 0x00: // Login Start
            return handler.HandleLoginStart(session, buffer);
            
        case 0x01: // Encryption Response
            return handler.HandleEncryptionResponse(session, buffer);
            
        case 0x02: // Login Plugin Response
            return handler.HandleLoginPluginResponse(session, buffer);
            
        default:
            spdlog::warn("Unknown packet ID 0x{:02X} in LOGIN state for session {}", 
                         packet_id, session->GetSessionId());
            return false;
    }
}

bool PacketDispatcher::DispatchConfiguration(std::uint8_t packet_id, std::shared_ptr<Session> session, PacketView& buffer) {
    auto& handler = GetConfigurationHandler();
    switch (packet_id) {
        case 0x00: // Client Information
            return handler.HandleClientInformation(session, buffer);
            
        case 0x01: // Plugin Message
            return handler.HandlePluginMessage(session, buffer);
            
        case 0x02: // Finish Configuration
            return handler.HandleFinishConfiguration(session, buffer);
            
        case 0x03: // Keep Alive
            return handler.HandleKeepAlive(session, buffer);
            
        case 0x04: // Pong
            return handler.HandlePong(session, buffer);
            
        case 0x05: // Resource Pack Response
            return handler.HandleResourcePackResponse(session, buffer);
            
        default:
            spdlog::warn("Unknown packet ID 0x{:02X} in CONFIGURATION state for session {}", 
                         packet_id, session->GetSessionId());
            return false;
    }
}

bool PacketDispatcher::DispatchPlay(std::uint8_t packet_id, std::shared_ptr<Session> session, PacketView& buffer) {
    // For the PLAY state, a more complex handler mapping might be needed
    // This is a simplified example
    auto& handler = GetPlayHandler();
    return handler.HandlePacket(packet_id, session, buffer);
}

} // namespace protocol