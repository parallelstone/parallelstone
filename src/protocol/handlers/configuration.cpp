#include "protocol/handlers/configuration.hpp"
#include "server/session.hpp"
#include "network/packet_view.hpp"
#include "network/buffer.hpp"
#include "protocol/version_config.hpp"
#include <spdlog/spdlog.h>

namespace protocol {

ConfigurationHandler& GetConfigurationHandler() {
    static ConfigurationHandler instance;
    return instance;
}

bool ConfigurationHandler::HandleClientInformation(std::shared_ptr<Session> session, PacketView& view) {
    if (!session) {
        spdlog::error("ConfigurationHandler::HandleClientInformation: null session");
        return false;
    }

    try {
        if (view.readable_bytes() < 10) { // Minimum expected size
            spdlog::warn("Session {}: Client Information packet too small", session->GetSessionId());
            session->Disconnect(DisconnectReason::PROTOCOL_ERROR, "Invalid client information packet");
            return false;
        }

        std::string locale = view.read_string();
        int8_t view_distance = view.read_int8();
        int32_t chat_mode = view.read_varint();
        bool chat_colors = view.read_bool();
        uint8_t displayed_skin_parts = view.read_byte();
        int32_t main_hand = view.read_varint();
        bool enable_text_filtering = view.read_bool();
        bool allow_server_listings = view.read_bool();

        // Validate values
        if (view_distance < 2 || view_distance > 32) {
            view_distance = 10; // Default to reasonable value
        }
        
        if (chat_mode < 0 || chat_mode > 2) {
            chat_mode = 0; // Default to enabled
        }

        if (main_hand < 0 || main_hand > 1) {
            main_hand = 1; // Default to right hand
        }

        spdlog::info("Session {}: Client info - locale: {}, view_distance: {}, chat_mode: {}", 
                     session->GetSessionId(), locale, view_distance, chat_mode);

        // Store client settings (you would typically store these in the session)
        // For now, we just acknowledge the packet
        return true;

    } catch (const std::exception& e) {
        spdlog::error("Session {}: Exception during client information: {}", 
                      session->GetSessionId(), e.what());
        session->Disconnect(DisconnectReason::PROTOCOL_ERROR, "Client information processing error");
        return false;
    }
}

bool ConfigurationHandler::HandlePluginMessage(std::shared_ptr<Session> session, PacketView& view) {
    if (!session) {
        spdlog::error("ConfigurationHandler::HandlePluginMessage: null session");
        return false;
    }

    try {
        if (view.readable_bytes() < 1) {
            spdlog::warn("Session {}: Plugin message packet too small", session->GetSessionId());
            session->Disconnect(DisconnectReason::PROTOCOL_ERROR, "Invalid plugin message packet");
            return false;
        }

        std::string channel = view.read_string();
        
        // Validate channel name
        if (channel.empty() || channel.length() > 256) {
            spdlog::warn("Session {}: Invalid plugin channel: {}", session->GetSessionId(), channel);
            return false;
        }

        std::vector<uint8_t> data;
        if (view.readable_bytes() > 0) {
            data.resize(view.readable_bytes());
            view.read_bytes(data.data(), data.size());
        }

        spdlog::debug("Session {}: Plugin message on channel '{}', size: {}", 
                      session->GetSessionId(), channel, data.size());

        // TODO: Handle specific plugin channels
        // Common channels: minecraft:brand, minecraft:register, etc.
        if (channel == "minecraft:brand") {
            // Client is telling us their brand (mod loader, etc.)
            if (!data.empty()) {
                std::string brand(data.begin(), data.end());
                spdlog::info("Session {}: Client brand: {}", session->GetSessionId(), brand);
            }
        }

        return true;

    } catch (const std::exception& e) {
        spdlog::error("Session {}: Exception during plugin message: {}", 
                      session->GetSessionId(), e.what());
        return false;
    }
}

bool ConfigurationHandler::HandleFinishConfiguration(std::shared_ptr<Session> session, PacketView& view) {
    if (!session) {
        spdlog::error("ConfigurationHandler::HandleFinishConfiguration: null session");
        return false;
    }

    try {
        spdlog::info("Session {}: Client finished configuration, transitioning to PLAY", 
                     session->GetSessionId());

        // Send Login (Play) packet
        Buffer login_packet;
        login_packet.write_varint(0x28); // Login (Play) packet ID

        // Entity ID (player's entity ID)
        login_packet.write_int32(1); // TODO: Use actual entity ID

        // Is hardcore
        login_packet.write_bool(false);

        // Game mode (0=Survival, 1=Creative, 2=Adventure, 3=Spectator)
        login_packet.write_byte(1); // Creative for now

        // Previous game mode (-1 if no previous)
        login_packet.write_int8(-1);

        // Dimension count
        login_packet.write_varint(1);

        // Dimension names
        login_packet.write_string("minecraft:overworld");

        // Registry codec (simplified - in real implementation, this would be complex NBT)
        // For now, send empty NBT compound
        login_packet.write_byte(0x0A); // NBT Compound
        login_packet.write_uint16(0);  // Empty name
        login_packet.write_byte(0x00); // End tag

        // Dimension type
        login_packet.write_string("minecraft:overworld");

        // Dimension name
        login_packet.write_string("minecraft:overworld");

        // Hashed seed
        login_packet.write_int64(0);

        // Max players
        login_packet.write_varint(100);

        // View distance
        login_packet.write_varint(10);

        // Simulation distance
        login_packet.write_varint(10);

        // Reduced debug info
        login_packet.write_bool(false);

        // Enable respawn screen
        login_packet.write_bool(true);

        // Do limited crafting
        login_packet.write_bool(false);

        // Dimension type (again, for some reason)
        login_packet.write_string("minecraft:overworld");

        // Death location (optional)
        login_packet.write_bool(false); // No death location

        session->Send(login_packet);
        session->SetNextState(SessionState::PLAY);

        return true;

    } catch (const std::exception& e) {
        spdlog::error("Session {}: Exception during finish configuration: {}", 
                      session->GetSessionId(), e.what());
        session->Disconnect(DisconnectReason::PROTOCOL_ERROR, "Configuration finish error");
        return false;
    }
}

bool ConfigurationHandler::HandleKeepAlive(std::shared_ptr<Session> session, PacketView& view) {
    if (!session) {
        spdlog::error("ConfigurationHandler::HandleKeepAlive: null session");
        return false;
    }

    try {
        if (view.readable_bytes() < 8) {
            spdlog::warn("Session {}: Keep alive packet too small", session->GetSessionId());
            return false;
        }

        int64_t keep_alive_id = view.read_int64();

        spdlog::debug("Session {}: Keep alive response: {}", session->GetSessionId(), keep_alive_id);

        // TODO: Validate keep alive ID matches what we sent
        // For now, just acknowledge it
        session->UpdateActivity();
        return true;

    } catch (const std::exception& e) {
        spdlog::error("Session {}: Exception during keep alive: {}", 
                      session->GetSessionId(), e.what());
        return false;
    }
}

bool ConfigurationHandler::HandlePong(std::shared_ptr<Session> session, PacketView& view) {
    if (!session) {
        spdlog::error("ConfigurationHandler::HandlePong: null session");
        return false;
    }

    try {
        if (view.readable_bytes() < 4) {
            spdlog::warn("Session {}: Pong packet too small", session->GetSessionId());
            return false;
        }

        int32_t pong_id = view.read_int32();

        spdlog::debug("Session {}: Pong response: {}", session->GetSessionId(), pong_id);

        // TODO: Validate pong ID matches what we sent in ping
        session->UpdateActivity();
        return true;

    } catch (const std::exception& e) {
        spdlog::error("Session {}: Exception during pong: {}", 
                      session->GetSessionId(), e.what());
        return false;
    }
}

bool ConfigurationHandler::HandleResourcePackResponse(std::shared_ptr<Session> session, PacketView& view) {
    if (!session) {
        spdlog::error("ConfigurationHandler::HandleResourcePackResponse: null session");
        return false;
    }

    try {
        if (view.readable_bytes() < 1) {
            spdlog::warn("Session {}: Resource pack response packet too small", session->GetSessionId());
            return false;
        }

        int32_t result = view.read_varint();

        const char* result_name = "UNKNOWN";
        switch (result) {
            case 0: result_name = "SUCCESSFULLY_LOADED"; break;
            case 1: result_name = "DECLINED"; break;
            case 2: result_name = "FAILED_DOWNLOAD"; break;
            case 3: result_name = "ACCEPTED"; break;
            case 4: result_name = "DOWNLOADED"; break;
            case 5: result_name = "INVALID_URL"; break;
            case 6: result_name = "FAILED_TO_RELOAD"; break;
            case 7: result_name = "DISCARDED"; break;
        }

        spdlog::info("Session {}: Resource pack response: {} ({})", 
                     session->GetSessionId(), result_name, result);

        // TODO: Handle different response types appropriately
        return true;

    } catch (const std::exception& e) {
        spdlog::error("Session {}: Exception during resource pack response: {}", 
                      session->GetSessionId(), e.what());
        return false;
    }
}

} // namespace protocol