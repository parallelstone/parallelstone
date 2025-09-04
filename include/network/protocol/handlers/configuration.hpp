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
 * @brief Handles packets for the Configuration state.
 */
class ConfigurationHandler {
public:
    ConfigurationHandler() = default;
    ~ConfigurationHandler() = default;

    ConfigurationHandler(const ConfigurationHandler&) = delete;
    ConfigurationHandler& operator=(const ConfigurationHandler&) = delete;

    bool HandleClientInformation(std::shared_ptr<Session> session, PacketView& view);
    bool HandlePluginMessage(std::shared_ptr<Session> session, PacketView& view);
    bool HandleFinishConfiguration(std::shared_ptr<Session> session, PacketView& view);
    bool HandleKeepAlive(std::shared_ptr<Session> session, PacketView& view);
    bool HandlePong(std::shared_ptr<Session> session, PacketView& view);
    bool HandleResourcePackResponse(std::shared_ptr<Session> session, PacketView& view);
};

ConfigurationHandler& GetConfigurationHandler();

} // namespace protocol