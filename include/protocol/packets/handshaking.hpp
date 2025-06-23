#pragma once

#include "../data_types.hpp"
#include "../packet.hpp"
#include "../protocol_state.hpp"

namespace minecraft::protocol::packets::handshaking {

// Serverbound Handshake Packet (0x00)
// Packet sent when client initiates connection to server
class HandshakePacket : public PacketBase<ProtocolState::HANDSHAKING,
                                          PacketDirection::SERVERBOUND> {
private:
  int32_t protocolVersion;   // Client protocol version
  std::string serverAddress; // Server address
  uint16_t serverPort;       // Server port
  int32_t nextState;         // Next state (1=Status, 2=Login)

public:
  HandshakePacket() : protocolVersion(0), serverPort(0), nextState(0) {}

  HandshakePacket(int32_t protocol, const std::string &address, uint16_t port,
                  int32_t next)
      : protocolVersion(protocol), serverAddress(address), serverPort(port),
        nextState(next) {}

  // IPacket interface implementation
  int32_t getPacketId() const override {
    return PacketId::Handshaking::Serverbound::HANDSHAKE;
  }

  void serialize(ByteBuffer &buffer) const override {
    buffer.writeVarInt(protocolVersion);
    buffer.writeString(serverAddress);
    buffer.writeShort(serverPort);
    buffer.writeVarInt(nextState);
  }

  void deserialize(ByteBuffer &buffer) override {
    protocolVersion = buffer.readVarInt();
    serverAddress = buffer.readString();
    serverPort = buffer.readShort();
    nextState = buffer.readVarInt();
  }

  std::string toString() const override {
    return "HandshakePacket{protocol=" + std::to_string(protocolVersion) +
           ", address=" + serverAddress +
           ", port=" + std::to_string(serverPort) +
           ", nextState=" + std::to_string(nextState) + "}";
  }

  // Getter methods
  int32_t getProtocolVersion() const { return protocolVersion; }
  const std::string &getServerAddress() const { return serverAddress; }
  uint16_t getServerPort() const { return serverPort; }
  int32_t getNextState() const { return nextState; }

  // Setter methods
  void setProtocolVersion(int32_t version) { protocolVersion = version; }
  void setServerAddress(const std::string &address) { serverAddress = address; }
  void setServerPort(uint16_t port) { serverPort = port; }
  void setNextState(int32_t state) { nextState = state; }

  // Utility methods
  bool isRequestingStatus() const { return nextState == 1; }
  bool isRequestingLogin() const { return nextState == 2; }

  ProtocolState getRequestedNextState() const {
    switch (nextState) {
    case 1:
      return ProtocolState::STATUS;
    case 2:
      return ProtocolState::LOGIN;
    default:
      return ProtocolState::HANDSHAKING; // Invalid state
    }
  }

  // Protocol version validation
  bool isProtocolVersionSupported() const {
    // Protocol version for 1.20.4 is 765
    // Here we assume supporting range 760-770 as example
    return protocolVersion >= 760 && protocolVersion <= 770;
  }

  // Server address validation
  bool isServerAddressValid() const {
    // Basic validation: not empty and not too long
    return !serverAddress.empty() && serverAddress.length() <= 255;
  }

  // Port validation
  bool isServerPortValid() const {
    // Port range validation (Minecraft servers typically use port 25565)
    return serverPort > 0 && serverPort <= 65535;
  }

  // Overall packet validity validation
  bool isValid() const {
    return isProtocolVersionSupported() && isServerAddressValid() &&
           isServerPortValid() && (nextState == 1 || nextState == 2);
  }
};

// Since only Handshake packet exists in Handshaking state,
// packet factory class is also defined here
class HandshakingPacketFactory : public IPacketFactory {
public:
  std::unique_ptr<IPacket> createPacket(int32_t packetId, ProtocolState state,
                                        PacketDirection direction) override {
    if (state != ProtocolState::HANDSHAKING) {
      return nullptr;
    }

    if (direction == PacketDirection::SERVERBOUND &&
        packetId == PacketId::Handshaking::Serverbound::HANDSHAKE) {
      return std::make_unique<HandshakePacket>();
    }

    return nullptr;
  }
};

// Handshaking state related utility functions
namespace utils {
// Convert protocol version to Minecraft version string
inline std::string protocolVersionToMinecraftVersion(int32_t protocolVersion) {
  // Actually more version mappings are needed
  switch (protocolVersion) {
  case 765:
    return "1.20.4";
  case 764:
    return "1.20.3";
  case 763:
    return "1.20.2";
  case 762:
    return "1.20.1";
  case 761:
    return "1.20";
  default:
    return "Unknown (" + std::to_string(protocolVersion) + ")";
  }
}

// Convert Minecraft version string to protocol version
inline int32_t minecraftVersionToProtocolVersion(const std::string &version) {
  if (version == "1.20.4")
    return 765;
  if (version == "1.20.3")
    return 764;
  if (version == "1.20.2")
    return 763;
  if (version == "1.20.1")
    return 762;
  if (version == "1.20")
    return 761;
  return -1; // Unsupported version
}

// Convert next state to string
inline std::string nextStateToString(int32_t nextState) {
  switch (nextState) {
  case 1:
    return "STATUS";
  case 2:
    return "LOGIN";
  default:
    return "UNKNOWN";
  }
}
} // namespace utils

} // namespace minecraft::protocol::packets::handshaking