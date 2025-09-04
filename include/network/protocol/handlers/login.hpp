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
using SessionInfo = parallelstone::server::SessionInfo;

/**
 * @brief Handles packets for the Login state.
 */
class LoginHandler {
public:
    LoginHandler() = default;
    ~LoginHandler() = default;

    LoginHandler(const LoginHandler&) = delete;
    LoginHandler& operator=(const LoginHandler&) = delete;

    bool HandleLoginStart(std::shared_ptr<Session> session, PacketView& view);
    bool HandleEncryptionResponse(std::shared_ptr<Session> session, PacketView& view);
    bool HandleLoginPluginResponse(std::shared_ptr<Session> session, PacketView& view);
};

LoginHandler& GetLoginHandler();

} // namespace protocol
