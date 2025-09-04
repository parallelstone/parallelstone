#include "protocol/handlers/status.hpp"
#include "server/session.hpp"
#include "network/packet_view.hpp"
#include "network/buffer.hpp"
#include "protocol/version_config.hpp"
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

namespace protocol {

StatusHandler& GetStatusHandler() {
    static StatusHandler instance;
    return instance;
}

bool StatusHandler::HandleStatusRequest(std::shared_ptr<Session> session, PacketView& view) {
    if (!session) {
        spdlog::error("StatusHandler::HandleStatusRequest: null session");
        return false;
    }

    try {
        // Status Request packet should be empty, but let's be defensive
        if (view.readable_bytes() > 0) {
            spdlog::debug("Session {}: Status Request packet has unexpected data ({} bytes)", 
                         session->GetSessionId(), view.readable_bytes());
            // Skip unexpected data
            view.skip_bytes(view.readable_bytes());
        }

        spdlog::info("Session {}: Handling status request", session->GetSessionId());

        // Build status response JSON
        nlohmann::json response;
        
        // Version information
        response["version"]["name"] = GetVersionString();
        response["version"]["protocol"] = GetProtocolVersion();
        
        // Player information
        response["players"]["max"] = 100;  // TODO: Get from server config
        response["players"]["online"] = 0; // TODO: Get actual player count
        response["players"]["sample"] = nlohmann::json::array(); // Empty sample for now
        
        // Server description (MOTD)
        response["description"]["text"] = "ParallelStone Minecraft Server";
        
        // Optional fields
        response["enforcesSecureChat"] = false;  // We don't enforce secure chat
        response["previewsChat"] = false;        // We don't preview chat
        
        // TODO: Add favicon if available
        // response["favicon"] = "data:image/png;base64,<base64_data>";

        std::string response_str;
        try {
            response_str = response.dump();
        } catch (const std::exception& e) {
            spdlog::error("Session {}: Failed to serialize status response: {}", 
                         session->GetSessionId(), e.what());
            session->Disconnect(DisconnectReason::INTERNAL_ERROR, "Status response error");
            return false;
        }

        // Validate response size (prevent extremely large responses)
        if (response_str.length() > 32767) { // Minecraft string length limit
            spdlog::error("Session {}: Status response too large: {} bytes", 
                         session->GetSessionId(), response_str.length());
            session->Disconnect(DisconnectReason::INTERNAL_ERROR, "Status response too large");
            return false;
        }

        // Send Status Response packet
        Buffer packet;
        packet.write_varint(0x00); // Status Response packet ID
        packet.write_string(response_str);

        session->Send(packet);
        session->UpdateActivity();
        
        spdlog::debug("Session {}: Status response sent ({} bytes)", 
                     session->GetSessionId(), response_str.length());
        return true;

    } catch (const std::exception& e) {
        spdlog::error("Session {}: Exception during status request: {}", 
                      session->GetSessionId(), e.what());
        session->Disconnect(DisconnectReason::PROTOCOL_ERROR, "Status request processing error");
        return false;
    }
}

bool StatusHandler::HandlePingRequest(std::shared_ptr<Session> session, PacketView& view) {
    if (!session) {
        spdlog::error("StatusHandler::HandlePingRequest: null session");
        return false;
    }

    try {
        // Validate packet size
        if (view.readable_bytes() < 8) {
            spdlog::warn("Session {}: Ping Request packet too small ({} bytes)", 
                        session->GetSessionId(), view.readable_bytes());
            session->Disconnect(DisconnectReason::PROTOCOL_ERROR, "Invalid ping packet");
            return false;
        }

        int64_t payload = view.read_int64();

        spdlog::debug("Session {}: Ping request with payload: {}", 
                     session->GetSessionId(), payload);

        // Send Ping Response packet with the same payload
        Buffer packet;
        packet.write_varint(0x01); // Ping Response packet ID
        packet.write_int64(payload);

        session->Send(packet);
        session->UpdateActivity();

        // After ping response, disconnect the client (status check is complete)
        spdlog::info("Session {}: Ping response sent, disconnecting client", 
                     session->GetSessionId());
        session->Disconnect(DisconnectReason::CLIENT_DISCONNECT, "Status check complete");
        
        return true;

    } catch (const std::exception& e) {
        spdlog::error("Session {}: Exception during ping request: {}", 
                      session->GetSessionId(), e.what());
        session->Disconnect(DisconnectReason::PROTOCOL_ERROR, "Ping request processing error");
        return false;
    }
}

} // namespace protocol
