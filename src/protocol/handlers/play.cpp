#include "protocol/handlers/play.hpp"
#include "server/session.hpp"
#include "network/packet_view.hpp"
#include "network/buffer.hpp"
#include "protocol/version_config.hpp"
#include <spdlog/spdlog.h>
#include <cmath>
#include <algorithm>

namespace protocol {

PlayHandler& GetPlayHandler() {
    static PlayHandler instance;
    return instance;
}

bool PlayHandler::HandlePacket(uint8_t packet_id, std::shared_ptr<Session> session, PacketView& view) {
    if (!session) {
        spdlog::error("PlayHandler::HandlePacket: null session");
        return false;
    }

    try {
        switch (packet_id) {
            case 0x00: return HandleConfirmTeleportation(session, view);
            case 0x12: return HandleKeepAlive(session, view);
            case 0x13: return HandleSetPlayerPosition(session, view);
            case 0x14: return HandleSetPlayerPositionAndRotation(session, view);
            case 0x15: return HandleSetPlayerRotation(session, view);
            case 0x16: return HandleSetPlayerOnGround(session, view);
            case 0x05: return HandleChatMessage(session, view);
            case 0x08: return HandleClientInformation(session, view);
            case 0x1D: return HandlePlayerAction(session, view);
            case 0x2E: return HandleUseItemOn(session, view);
            case 0x2F: return HandleUseItem(session, view);
            case 0x30: return HandleSwingArm(session, view);
            default:
                spdlog::debug("Session {}: Unhandled Play packet ID: 0x{:02X}", 
                             session->GetSessionId(), packet_id);
                // Skip remaining bytes to prevent buffer issues
                view.skip_bytes(view.readable_bytes());
                return true;
        }
    } catch (const std::exception& e) {
        spdlog::error("Session {}: Exception in Play packet handler (ID: 0x{:02X}): {}", 
                      session->GetSessionId(), packet_id, e.what());
        return false;
    }
}

bool PlayHandler::HandleConfirmTeleportation(std::shared_ptr<Session> session, PacketView& view) {
    if (!session) return false;

    try {
        if (view.readable_bytes() < 1) {
            spdlog::warn("Session {}: Confirm Teleportation packet too small", session->GetSessionId());
            return false;
        }

        int32_t teleport_id = view.read_varint();
        
        spdlog::debug("Session {}: Confirmed teleportation with ID: {}", 
                      session->GetSessionId(), teleport_id);

        session->UpdateActivity();
        return true;

    } catch (const std::exception& e) {
        spdlog::error("Session {}: Exception in ConfirmTeleportation: {}", 
                      session->GetSessionId(), e.what());
        return false;
    }
}

bool PlayHandler::HandleKeepAlive(std::shared_ptr<Session> session, PacketView& view) {
    if (!session) return false;

    try {
        if (view.readable_bytes() < 8) {
            spdlog::warn("Session {}: Keep Alive packet too small", session->GetSessionId());
            return false;
        }

        int64_t keep_alive_id = view.read_int64();
        
        spdlog::debug("Session {}: Keep alive response: {}", session->GetSessionId(), keep_alive_id);

        session->UpdateActivity();
        return true;

    } catch (const std::exception& e) {
        spdlog::error("Session {}: Exception in KeepAlive: {}", 
                      session->GetSessionId(), e.what());
        return false;
    }
}

bool PlayHandler::HandleSetPlayerPosition(std::shared_ptr<Session> session, PacketView& view) {
    if (!session) return false;

    try {
        if (view.readable_bytes() < 25) { // 3 doubles + 1 bool = 25 bytes
            spdlog::warn("Session {}: Set Player Position packet too small", session->GetSessionId());
            return false;
        }

        double x = view.read_double();
        double feet_y = view.read_double();
        double z = view.read_double();
        bool on_ground = view.read_bool();

        // Validate coordinates (prevent extreme values)
        if (std::isnan(x) || std::isnan(feet_y) || std::isnan(z) || 
            std::isinf(x) || std::isinf(feet_y) || std::isinf(z)) {
            spdlog::warn("Session {}: Invalid coordinates received", session->GetSessionId());
            session->Disconnect(parallelstone::server::DisconnectReason::PROTOCOL_ERROR, "Invalid coordinates");
            return false;
        }

        // Check for reasonable coordinate bounds (30 million blocks from origin)
        constexpr double MAX_COORD = 30000000.0;
        if (std::abs(x) > MAX_COORD || std::abs(z) > MAX_COORD || 
            feet_y < -2048 || feet_y > 2048) {
            spdlog::warn("Session {}: Coordinates out of bounds: ({}, {}, {})", 
                        session->GetSessionId(), x, feet_y, z);
            session->Disconnect(parallelstone::server::DisconnectReason::PROTOCOL_ERROR, "Coordinates out of bounds");
            return false;
        }

        spdlog::debug("Session {}: Player position: ({:.2f}, {:.2f}, {:.2f}), on_ground: {}", 
                      session->GetSessionId(), x, feet_y, z, on_ground);

        session->UpdateActivity();
        return true;

    } catch (const std::exception& e) {
        spdlog::error("Session {}: Exception in SetPlayerPosition: {}", 
                      session->GetSessionId(), e.what());
        return false;
    }
}

bool PlayHandler::HandleSetPlayerPositionAndRotation(std::shared_ptr<Session> session, PacketView& view) {
    if (!session) return false;

    try {
        if (view.readable_bytes() < 33) { // 3 doubles + 2 floats + 1 bool = 33 bytes
            spdlog::warn("Session {}: Set Player Position and Rotation packet too small", 
                        session->GetSessionId());
            return false;
        }

        double x = view.read_double();
        double feet_y = view.read_double();
        double z = view.read_double();
        float yaw = view.read_float();
        float pitch = view.read_float();
        bool on_ground = view.read_bool();

        // Validate coordinates
        if (std::isnan(x) || std::isnan(feet_y) || std::isnan(z) || 
            std::isinf(x) || std::isinf(feet_y) || std::isinf(z)) {
            spdlog::warn("Session {}: Invalid coordinates received", session->GetSessionId());
            session->Disconnect(parallelstone::server::DisconnectReason::PROTOCOL_ERROR, "Invalid coordinates");
            return false;
        }

        // Validate rotation
        if (std::isnan(yaw) || std::isnan(pitch) || std::isinf(yaw) || std::isinf(pitch)) {
            spdlog::warn("Session {}: Invalid rotation received", session->GetSessionId());
            session->Disconnect(parallelstone::server::DisconnectReason::PROTOCOL_ERROR, "Invalid rotation");
            return false;
        }

        // Check coordinate bounds
        constexpr double MAX_COORD = 30000000.0;
        if (std::abs(x) > MAX_COORD || std::abs(z) > MAX_COORD || 
            feet_y < -2048 || feet_y > 2048) {
            spdlog::warn("Session {}: Coordinates out of bounds", session->GetSessionId());
            session->Disconnect(parallelstone::server::DisconnectReason::PROTOCOL_ERROR, "Coordinates out of bounds");
            return false;
        }

        // Normalize pitch to [-90, 90] degrees
        if (pitch < -90.0f || pitch > 90.0f) {
            pitch = std::clamp(pitch, -90.0f, 90.0f);
        }

        spdlog::debug("Session {}: Player pos+rot: ({:.2f}, {:.2f}, {:.2f}), yaw: {:.2f}, pitch: {:.2f}", 
                      session->GetSessionId(), x, feet_y, z, yaw, pitch);

        session->UpdateActivity();
        return true;

    } catch (const std::exception& e) {
        spdlog::error("Session {}: Exception in SetPlayerPositionAndRotation: {}", 
                      session->GetSessionId(), e.what());
        return false;
    }
}

bool PlayHandler::HandleSetPlayerRotation(std::shared_ptr<Session> session, PacketView& view) {
    if (!session) return false;

    try {
        if (view.readable_bytes() < 9) { // 2 floats + 1 bool = 9 bytes
            spdlog::warn("Session {}: Set Player Rotation packet too small", session->GetSessionId());
            return false;
        }

        float yaw = view.read_float();
        float pitch = view.read_float();
        bool on_ground = view.read_bool();

        // Validate rotation
        if (std::isnan(yaw) || std::isnan(pitch) || std::isinf(yaw) || std::isinf(pitch)) {
            spdlog::warn("Session {}: Invalid rotation received", session->GetSessionId());
            session->Disconnect(parallelstone::server::DisconnectReason::PROTOCOL_ERROR, "Invalid rotation");
            return false;
        }

        // Normalize pitch
        if (pitch < -90.0f || pitch > 90.0f) {
            pitch = std::clamp(pitch, -90.0f, 90.0f);
        }

        spdlog::debug("Session {}: Player rotation: yaw: {:.2f}, pitch: {:.2f}, on_ground: {}", 
                      session->GetSessionId(), yaw, pitch, on_ground);

        session->UpdateActivity();
        return true;

    } catch (const std::exception& e) {
        spdlog::error("Session {}: Exception in SetPlayerRotation: {}", 
                      session->GetSessionId(), e.what());
        return false;
    }
}

bool PlayHandler::HandleSetPlayerOnGround(std::shared_ptr<Session> session, PacketView& view) {
    if (!session) return false;

    try {
        if (view.readable_bytes() < 1) {
            spdlog::warn("Session {}: Set Player On Ground packet too small", session->GetSessionId());
            return false;
        }

        bool on_ground = view.read_bool();

        spdlog::debug("Session {}: Player on ground: {}", session->GetSessionId(), on_ground);

        session->UpdateActivity();
        return true;

    } catch (const std::exception& e) {
        spdlog::error("Session {}: Exception in SetPlayerOnGround: {}", 
                      session->GetSessionId(), e.what());
        return false;
    }
}

bool PlayHandler::HandleChatMessage(std::shared_ptr<Session> session, PacketView& view) {
    if (!session) return false;

    try {
        if (view.readable_bytes() < 1) {
            spdlog::warn("Session {}: Chat Message packet too small", session->GetSessionId());
            return false;
        }

        std::string message = view.read_string();
        
        // Validate message length
        if (message.length() > 256) {
            spdlog::warn("Session {}: Chat message too long: {} characters", 
                        session->GetSessionId(), message.length());
            return false;
        }

        // Check for empty message
        if (message.empty()) {
            spdlog::debug("Session {}: Empty chat message received", session->GetSessionId());
            return true;
        }

        // Read additional fields if available (timestamp, salt, signature, etc.)
        if (view.readable_bytes() >= 16) { // timestamp + salt
            int64_t timestamp = view.read_int64();
            int64_t salt = view.read_int64();
            
            // Signature (optional in some versions)
            if (view.readable_bytes() >= 1) {
                bool has_signature = view.read_bool();
                if (has_signature && view.readable_bytes() >= 256) {
                    std::vector<uint8_t> signature(256);
                    view.read_bytes(signature.data(), 256);
                }
            }
        }

        spdlog::info("Session {}: Chat message: '{}'", session->GetSessionId(), message);

        session->UpdateActivity();
        return true;

    } catch (const std::exception& e) {
        spdlog::error("Session {}: Exception in ChatMessage: {}", 
                      session->GetSessionId(), e.what());
        return false;
    }
}

bool PlayHandler::HandleClientInformation(std::shared_ptr<Session> session, PacketView& view) {
    if (!session) return false;

    try {
        if (view.readable_bytes() < 10) {
            spdlog::warn("Session {}: Client Information packet too small", session->GetSessionId());
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
            view_distance = 10;
        }
        if (chat_mode < 0 || chat_mode > 2) {
            chat_mode = 0;
        }
        if (main_hand < 0 || main_hand > 1) {
            main_hand = 1;
        }

        spdlog::info("Session {}: Client info updated - locale: {}, view_distance: {}", 
                     session->GetSessionId(), locale, view_distance);

        session->UpdateActivity();
        return true;

    } catch (const std::exception& e) {
        spdlog::error("Session {}: Exception in ClientInformation: {}", 
                      session->GetSessionId(), e.what());
        return false;
    }
}

bool PlayHandler::HandlePlayerAction(std::shared_ptr<Session> session, PacketView& view) {
    if (!session) return false;

    try {
        if (view.readable_bytes() < 17) { // varint + position + byte + varint = min 17 bytes
            spdlog::warn("Session {}: Player Action packet too small", session->GetSessionId());
            return false;
        }

        int32_t status = view.read_varint();
        
        // Read position (encoded as long)
        int64_t position = view.read_int64();
        int32_t x = static_cast<int32_t>(position >> 38);
        int32_t y = static_cast<int32_t>(position & 0xFFF);
        int32_t z = static_cast<int32_t>((position >> 12) & 0x3FFFFFF);
        
        // Sign extension for x and z
        if (x >= 0x2000000) x -= 0x4000000;
        if (z >= 0x2000000) z -= 0x4000000;
        
        uint8_t face = view.read_byte();
        int32_t sequence = view.read_varint();

        // Validate action status
        if (status < 0 || status > 6) {
            spdlog::warn("Session {}: Invalid player action status: {}", session->GetSessionId(), status);
            return false;
        }

        // Validate face direction
        if (face > 5) {
            spdlog::warn("Session {}: Invalid block face: {}", session->GetSessionId(), face);
            return false;
        }

        const char* action_names[] = {
            "START_DESTROY_BLOCK", "ABORT_DESTROY_BLOCK", "STOP_DESTROY_BLOCK",
            "DROP_ALL_ITEMS", "DROP_ITEM", "RELEASE_USE_ITEM", "SWAP_ITEM_WITH_OFFHAND"
        };

        spdlog::debug("Session {}: Player action {} at ({}, {}, {}), face: {}, sequence: {}", 
                      session->GetSessionId(), action_names[status], x, y, z, face, sequence);

        session->UpdateActivity();
        return true;

    } catch (const std::exception& e) {
        spdlog::error("Session {}: Exception in PlayerAction: {}", 
                      session->GetSessionId(), e.what());
        return false;
    }
}

bool PlayHandler::HandleUseItemOn(std::shared_ptr<Session> session, PacketView& view) {
    if (!session) return false;

    try {
        if (view.readable_bytes() < 18) { // varint + position + varint + float*3 + bool + varint
            spdlog::warn("Session {}: Use Item On packet too small", session->GetSessionId());
            return false;
        }

        int32_t hand = view.read_varint();
        int64_t position = view.read_int64();
        int32_t face = view.read_varint();
        float cursor_x = view.read_float();
        float cursor_y = view.read_float();
        float cursor_z = view.read_float();
        bool inside_block = view.read_bool();
        int32_t sequence = view.read_varint();

        // Validate hand
        if (hand < 0 || hand > 1) {
            spdlog::warn("Session {}: Invalid hand: {}", session->GetSessionId(), hand);
            return false;
        }

        // Validate face
        if (face < 0 || face > 5) {
            spdlog::warn("Session {}: Invalid face: {}", session->GetSessionId(), face);
            return false;
        }

        // Validate cursor position
        if (cursor_x < 0.0f || cursor_x > 1.0f || 
            cursor_y < 0.0f || cursor_y > 1.0f || 
            cursor_z < 0.0f || cursor_z > 1.0f) {
            spdlog::warn("Session {}: Invalid cursor position", session->GetSessionId());
            return false;
        }

        spdlog::debug("Session {}: Use item on block, hand: {}, face: {}", 
                      session->GetSessionId(), hand, face);

        session->UpdateActivity();
        return true;

    } catch (const std::exception& e) {
        spdlog::error("Session {}: Exception in UseItemOn: {}", 
                      session->GetSessionId(), e.what());
        return false;
    }
}

bool PlayHandler::HandleUseItem(std::shared_ptr<Session> session, PacketView& view) {
    if (!session) return false;

    try {
        if (view.readable_bytes() < 8) { // varint + varint
            spdlog::warn("Session {}: Use Item packet too small", session->GetSessionId());
            return false;
        }

        int32_t hand = view.read_varint();
        int32_t sequence = view.read_varint();

        // Validate hand
        if (hand < 0 || hand > 1) {
            spdlog::warn("Session {}: Invalid hand: {}", session->GetSessionId(), hand);
            return false;
        }

        spdlog::debug("Session {}: Use item, hand: {}, sequence: {}", 
                      session->GetSessionId(), hand, sequence);

        session->UpdateActivity();
        return true;

    } catch (const std::exception& e) {
        spdlog::error("Session {}: Exception in UseItem: {}", 
                      session->GetSessionId(), e.what());
        return false;
    }
}

bool PlayHandler::HandleSwingArm(std::shared_ptr<Session> session, PacketView& view) {
    if (!session) return false;

    try {
        if (view.readable_bytes() < 1) {
            spdlog::warn("Session {}: Swing Arm packet too small", session->GetSessionId());
            return false;
        }

        int32_t hand = view.read_varint();

        // Validate hand
        if (hand < 0 || hand > 1) {
            spdlog::warn("Session {}: Invalid hand: {}", session->GetSessionId(), hand);
            return false;
        }

        spdlog::debug("Session {}: Swing arm, hand: {}", session->GetSessionId(), hand);

        session->UpdateActivity();
        return true;

    } catch (const std::exception& e) {
        spdlog::error("Session {}: Exception in SwingArm: {}", 
                      session->GetSessionId(), e.what());
        return false;
    }
}

} // namespace protocol