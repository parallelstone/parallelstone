#include "protocol/handlers/login.hpp"
#include "server/session.hpp"
#include "network/packet_view.hpp"
#include "network/buffer.hpp"
#include "protocol/version_config.hpp"
#include <spdlog/spdlog.h>
#include <regex>
#include <algorithm>

namespace protocol {

LoginHandler& GetLoginHandler() {
    static LoginHandler instance;
    return instance;
}

namespace {
    // Constants for validation
    constexpr size_t MIN_USERNAME_LENGTH = 3;
    constexpr size_t MAX_USERNAME_LENGTH = 16;
    constexpr size_t MAX_SHARED_SECRET_LENGTH = 1024;
    constexpr size_t MAX_VERIFY_TOKEN_LENGTH = 1024;

    bool IsValidUsername(const std::string& username) {
        if (username.length() < MIN_USERNAME_LENGTH || username.length() > MAX_USERNAME_LENGTH) {
            return false;
        }
        
        // Minecraft usernames: alphanumeric and underscore only
        std::regex username_pattern("^[a-zA-Z0-9_]+$");
        return std::regex_match(username, username_pattern);
    }

    std::uint64_t GenerateOfflineUUID(const std::string& username) {
        // Generate a deterministic UUID for offline mode based on username
        std::hash<std::string> hasher;
        return hasher(username);
    }
}

bool LoginHandler::HandleLoginStart(std::shared_ptr<Session> session, PacketView& view) {
    if (!session) {
        spdlog::error("LoginHandler::HandleLoginStart: null session");
        return false;
    }

    try {
        // Validate packet has enough data
        if (view.readable_bytes() < 1) {
            spdlog::warn("Session {}: Login Start packet too small", session->GetSessionId());
            session->Disconnect(DisconnectReason::PROTOCOL_ERROR, "Invalid login packet");
            return false;
        }

        std::string username;
        try {
            username = view.read_string();
        } catch (const std::exception& e) {
            spdlog::warn("Session {}: Failed to read username: {}", session->GetSessionId(), e.what());
            session->Disconnect(DisconnectReason::PROTOCOL_ERROR, "Invalid username format");
            return false;
        }

        // Validate username
        if (!IsValidUsername(username)) {
            spdlog::warn("Session {}: Invalid username '{}'", session->GetSessionId(), username);
            session->Disconnect(DisconnectReason::PROTOCOL_ERROR, "Invalid username");
            return false;
        }

        // Check for UUID in newer protocol versions (1.19+)
        bool has_uuid = false;
        std::uint64_t uuid_most = 0, uuid_least = 0;
        
        if (view.readable_bytes() >= 16) { // UUID is 16 bytes
            try {
                has_uuid = view.read_bool();
                if (has_uuid) {
                    auto uuid_pair = view.read_uuid();
                    uuid_most = uuid_pair.first;
                    uuid_least = uuid_pair.second;
                }
            } catch (const std::exception& e) {
                spdlog::debug("Session {}: No UUID in login packet (older client?): {}", 
                             session->GetSessionId(), e.what());
                // Not an error for older clients
            }
        }

        spdlog::info("Session {}: Player '{}' attempting to log in", session->GetSessionId(), username);

        // TODO: In a production server, you would:
        // 1. Check if player is banned
        // 2. Check server capacity
        // 3. Validate player UUID with Mojang servers (online mode)
        // 4. Handle encryption setup (online mode)

        // For now, accept the login immediately (offline mode)
        if (!has_uuid) {
            // Generate offline UUID
            uuid_least = GenerateOfflineUUID(username);
            uuid_most = 0;
        }

        // Update session info
        auto& info = const_cast<SessionInfo&>(session->GetInfo());
        info.player_name = username;
        info.player_uuid = std::to_string(uuid_most) + "-" + std::to_string(uuid_least);

        // Send Login Success packet
        Buffer success_packet;
        success_packet.write_varint(0x02); // Login Success packet ID
        
        // Write UUID
        success_packet.write_uint64(uuid_most);
        success_packet.write_uint64(uuid_least);
        
        success_packet.write_string(username);
        success_packet.write_varint(0); // No properties for offline mode

        session->Send(success_packet);
        session->SetNextState(SessionState::CONFIGURATION);
        
        spdlog::info("Session {}: Player '{}' login successful, transitioning to CONFIGURATION", 
                     session->GetSessionId(), username);
        return true;

    } catch (const std::exception& e) {
        spdlog::error("Session {}: Exception during login start: {}", session->GetSessionId(), e.what());
        session->Disconnect(DisconnectReason::PROTOCOL_ERROR, "Login processing error");
        return false;
    }
}

bool LoginHandler::HandleEncryptionResponse(std::shared_ptr<Session> session, PacketView& view) {
    if (!session) {
        spdlog::error("LoginHandler::HandleEncryptionResponse: null session");
        return false;
    }

    try {
        // Read shared secret length and data
        int32_t shared_secret_length = view.read_varint();
        if (shared_secret_length < 0 || shared_secret_length > MAX_SHARED_SECRET_LENGTH) {
            spdlog::warn("Session {}: Invalid shared secret length: {}", 
                        session->GetSessionId(), shared_secret_length);
            session->Disconnect(DisconnectReason::PROTOCOL_ERROR, "Invalid shared secret length");
            return false;
        }

        if (view.readable_bytes() < static_cast<size_t>(shared_secret_length)) {
            spdlog::warn("Session {}: Not enough data for shared secret", session->GetSessionId());
            session->Disconnect(DisconnectReason::PROTOCOL_ERROR, "Incomplete encryption response");
            return false;
        }

        std::vector<uint8_t> shared_secret(shared_secret_length);
        view.read_bytes(shared_secret.data(), shared_secret_length);

        // Read verify token length and data
        int32_t verify_token_length = view.read_varint();
        if (verify_token_length < 0 || verify_token_length > MAX_VERIFY_TOKEN_LENGTH) {
            spdlog::warn("Session {}: Invalid verify token length: {}", 
                        session->GetSessionId(), verify_token_length);
            session->Disconnect(DisconnectReason::PROTOCOL_ERROR, "Invalid verify token length");
            return false;
        }

        if (view.readable_bytes() < static_cast<size_t>(verify_token_length)) {
            spdlog::warn("Session {}: Not enough data for verify token", session->GetSessionId());
            session->Disconnect(DisconnectReason::PROTOCOL_ERROR, "Incomplete encryption response");
            return false;
        }

        std::vector<uint8_t> verify_token(verify_token_length);
        view.read_bytes(verify_token.data(), verify_token_length);

        // TODO: In online mode, you would:
        // 1. Decrypt shared secret with server's private RSA key
        // 2. Decrypt verify token with server's private RSA key
        // 3. Compare verify token with what was sent in Encryption Request
        // 4. Set up AES encryption for the connection
        // 5. Authenticate with Mojang's session servers

        spdlog::warn("Session {}: Encryption response received, but server is in offline mode", 
                     session->GetSessionId());
        session->Disconnect(DisconnectReason::PROTOCOL_ERROR, "Server is in offline mode");
        return false;

    } catch (const std::exception& e) {
        spdlog::error("Session {}: Exception during encryption response: {}", 
                      session->GetSessionId(), e.what());
        session->Disconnect(DisconnectReason::PROTOCOL_ERROR, "Encryption processing error");
        return false;
    }
}

bool LoginHandler::HandleLoginPluginResponse(std::shared_ptr<Session> session, PacketView& view) {
    if (!session) {
        spdlog::error("LoginHandler::HandleLoginPluginResponse: null session");
        return false;
    }

    try {
        if (view.readable_bytes() < 5) { // At least message ID (varint) + successful (bool)
            spdlog::warn("Session {}: Login plugin response packet too small", session->GetSessionId());
            session->Disconnect(DisconnectReason::PROTOCOL_ERROR, "Invalid plugin response packet");
            return false;
        }

        int32_t message_id = view.read_varint();
        bool successful = view.read_bool();

        std::vector<uint8_t> data;
        if (successful && view.readable_bytes() > 0) {
            data.resize(view.readable_bytes());
            view.read_bytes(data.data(), data.size());
        }

        spdlog::debug("Session {}: Login plugin response - message_id: {}, successful: {}, data_size: {}", 
                      session->GetSessionId(), message_id, successful, data.size());

        // TODO: Handle specific plugin responses
        // For now, we don't support any login plugins
        spdlog::warn("Session {}: Login plugin responses not supported", session->GetSessionId());
        session->Disconnect(DisconnectReason::PROTOCOL_ERROR, "Login plugins not supported");
        return false;

    } catch (const std::exception& e) {
        spdlog::error("Session {}: Exception during login plugin response: {}", 
                      session->GetSessionId(), e.what());
        session->Disconnect(DisconnectReason::PROTOCOL_ERROR, "Plugin response processing error");
        return false;
    }
}

} // namespace protocol
