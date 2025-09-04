#include "protocol/handlers/handshaking.hpp"
#include "server/session.hpp"
#include "network/packet_view.hpp"
#include "network/buffer.hpp"
#include "protocol/version_config.hpp"
#include <spdlog/spdlog.h>
#include <regex>

namespace protocol {

// ============================================================================
// GLOBAL HANDLER INSTANCE
// ============================================================================

HandshakingHandler& GetHandshakingHandler() {
    static HandshakingHandler instance;
    return instance;
}

// ============================================================================
// VALIDATION HELPERS
// ============================================================================

namespace {
    constexpr size_t MAX_SERVER_ADDRESS_LENGTH = 255;
    constexpr uint16_t MIN_PORT = 1;
    constexpr uint16_t MAX_PORT = 65535;

    bool IsValidServerAddress(const std::string& address) {
        if (address.empty() || address.length() > MAX_SERVER_ADDRESS_LENGTH) {
            return false;
        }
        
        // Basic validation: no control characters, reasonable ASCII
        for (char c : address) {
            if (c < 32 || c > 126) {
                return false;
            }
        }
        
        return true;
    }

    bool IsValidPort(uint16_t port) {
        return port >= MIN_PORT && port <= MAX_PORT;
    }

    bool IsSupportedProtocol(int32_t protocol_version) {
        // Only accept the exact protocol version defined in version_config
        return protocol_version == GetProtocolVersion();
    }

    const char* GetNextStateName(int32_t next_state) {
        switch (next_state) {
            case 1: return "STATUS";
            case 2: return "LOGIN";
            default: return "UNKNOWN";
        }
    }
}

// ============================================================================
// PUBLIC HANDLER METHODS
// ============================================================================

bool HandshakingHandler::HandleHandshake(std::shared_ptr<Session> session, PacketView& view) {
    if (!session) {
        spdlog::error("HandshakingHandler::HandleHandshake: null session");
        return false;
    }

    try {
        // Validate minimum packet size
        if (view.readable_bytes() < 7) { // Minimum: varint + 1 char string + uint16 + varint
            spdlog::warn("Session {}: Handshake packet too small ({} bytes)", 
                        session->GetSessionId(), view.readable_bytes());
            session->Disconnect(DisconnectReason::PROTOCOL_ERROR, "Invalid handshake packet");
            return false;
        }

        // Read handshake fields
        int32_t protocol_version;
        std::string server_address;
        uint16_t server_port;
        int32_t next_state;

        try {
            protocol_version = view.read_varint();
            server_address = view.read_string();
            server_port = view.read_uint16();
            next_state = view.read_varint();
        } catch (const std::exception& e) {
            spdlog::warn("Session {}: Failed to parse handshake fields: {}", 
                        session->GetSessionId(), e.what());
            session->Disconnect(DisconnectReason::PROTOCOL_ERROR, "Malformed handshake packet");
            return false;
        }

        // Validate protocol version - only accept the configured version
        if (!IsSupportedProtocol(protocol_version)) {
            spdlog::warn("Session {}: Unsupported protocol version: {} (expected: {} for {})", 
                        session->GetSessionId(), protocol_version, 
                        GetProtocolVersion(), GetVersionString());
            session->Disconnect(DisconnectReason::PROTOCOL_ERROR, 
                              "Unsupported client version. Please use " + std::string(GetVersionString()));
            return false;
        }

        // Validate server address
        if (!IsValidServerAddress(server_address)) {
            spdlog::warn("Session {}: Invalid server address: '{}'", 
                        session->GetSessionId(), server_address);
            session->Disconnect(DisconnectReason::PROTOCOL_ERROR, "Invalid server address");
            return false;
        }

        // Validate server port
        if (!IsValidPort(server_port)) {
            spdlog::warn("Session {}: Invalid server port: {}", 
                        session->GetSessionId(), server_port);
            session->Disconnect(DisconnectReason::PROTOCOL_ERROR, "Invalid server port");
            return false;
        }

        // Validate next state
        if (next_state != 1 && next_state != 2) {
            spdlog::warn("Session {}: Invalid next state {} in handshake", 
                        session->GetSessionId(), next_state);
            session->Disconnect(DisconnectReason::PROTOCOL_ERROR, 
                              "Invalid next state in handshaking");
            return false;
        }

        // Update session info
        auto& info = const_cast<parallelstone::server::SessionInfo&>(session->GetInfo());
        info.protocol_version = protocol_version;

        // Set next state
        if (next_state == 1) {
            session->SetNextState(SessionState::STATUS);
            spdlog::info("Session {}: Handshake complete, transitioning to STATUS (protocol: {} - {})", 
                        session->GetSessionId(), protocol_version, GetVersionString());
        } else if (next_state == 2) {
            session->SetNextState(SessionState::LOGIN);
            spdlog::info("Session {}: Handshake complete, transitioning to LOGIN (protocol: {} - {})", 
                        session->GetSessionId(), protocol_version, GetVersionString());
        }

        // Log handshake details
        spdlog::debug("Session {}: Handshake details - address: '{}', port: {}, next_state: {} ({})", 
                     session->GetSessionId(), server_address, server_port, 
                     next_state, GetNextStateName(next_state));

        session->UpdateActivity();
        return true;

    } catch (const std::exception& e) {
        spdlog::error("Session {}: Exception during handshake: {}", 
                      session->GetSessionId(), e.what());
        session->Disconnect(DisconnectReason::PROTOCOL_ERROR, "Handshake processing error");
        return false;
    }
}

bool HandshakingHandler::HandleLegacyServerListPing(std::shared_ptr<Session> session, PacketView& view) {
    if (!session) {
        spdlog::error("HandshakingHandler::HandleLegacyServerListPing: null session");
        return false;
    }

    try {
        spdlog::info("Session {}: Received legacy server list ping", session->GetSessionId());

        // Legacy ping packets can vary in format
        // For safety, we'll just skip all remaining data
        if (view.readable_bytes() > 0) {
            spdlog::debug("Session {}: Legacy ping packet has {} bytes of data", 
                         session->GetSessionId(), view.readable_bytes());
            view.skip_bytes(view.readable_bytes());
        }

        // In a full implementation, you might want to send a legacy response:
        /*
        std::string legacy_response = "§1\x00127\x00ParallelStone Server\x000\x00100";
        Buffer response_buffer;
        response_buffer.write_byte(0xFF); // Disconnect/Kick packet
        response_buffer.write_string(legacy_response);
        session->Send(response_buffer);
        */

        // For now, we gracefully decline legacy pings
        spdlog::info("Session {}: Legacy ping not supported, disconnecting", 
                     session->GetSessionId());
        session->Disconnect(DisconnectReason::PROTOCOL_ERROR, 
                          "Legacy ping is not supported by this server");
        return true;

    } catch (const std::exception& e) {
        spdlog::error("Session {}: Exception during legacy ping: {}", 
                      session->GetSessionId(), e.what());
        session->Disconnect(DisconnectReason::PROTOCOL_ERROR, 
                          "Legacy ping processing error");
        return false;
    }
}

} // namespace protocol