#pragma once

#include "packet.hpp"
#include "packets/handshaking.hpp"
#include "packets/login.hpp"
#include "packets/play.hpp"
#include "packets/status.hpp"
#include "protocol_state.hpp"
#include <functional>
#include <memory>
#include <unordered_map>

namespace minecraft::protocol {

// Packet identifier structure
struct PacketKey {
  ProtocolState state;
  PacketDirection direction;
  int32_t packetId;

  PacketKey(ProtocolState s, PacketDirection d, int32_t id)
      : state(s), direction(d), packetId(id) {}

  bool operator==(const PacketKey &other) const {
    return state == other.state && direction == other.direction &&
           packetId == other.packetId;
  }
};

// PacketKey hash function
struct PacketKeyHash {
  std::size_t operator()(const PacketKey &key) const {
    std::size_t h1 = std::hash<int>{}(static_cast<int>(key.state));
    std::size_t h2 = std::hash<int>{}(static_cast<int>(key.direction));
    std::size_t h3 = std::hash<int32_t>{}(key.packetId);
    return h1 ^ (h2 << 1) ^ (h3 << 2);
  }
};

// Packet creation function type
using PacketCreateFunction = std::function<std::unique_ptr<IPacket>()>;

// Packet registry class
class PacketRegistry {
private:
  std::unordered_map<PacketKey, PacketCreateFunction, PacketKeyHash>
      packetCreators;

  // Singleton pattern
  PacketRegistry() { registerAllPackets(); }

  void registerAllPackets();

public:
  // Return singleton instance
  static PacketRegistry &getInstance() {
    static PacketRegistry instance;
    return instance;
  }

  // Delete copy constructor and assignment operator
  PacketRegistry(const PacketRegistry &) = delete;
  PacketRegistry &operator=(const PacketRegistry &) = delete;

  // Register packet
  void registerPacket(ProtocolState state, PacketDirection direction,
                      int32_t packetId, PacketCreateFunction creator);

  // Create packet
  std::unique_ptr<IPacket> createPacket(ProtocolState state,
                                        PacketDirection direction,
                                        int32_t packetId);

  // Check packet existence
  bool hasPacket(ProtocolState state, PacketDirection direction,
                 int32_t packetId) const;

  // Return registered packet count
  size_t getRegisteredPacketCount() const { return packetCreators.size(); }

  // Return packet ID list for specific state
  std::vector<int32_t> getPacketIds(ProtocolState state,
                                    PacketDirection direction) const;

  // Return packet information string
  std::string getPacketInfo(ProtocolState state, PacketDirection direction,
                            int32_t packetId) const;
};

// Convenience functions
namespace registry {
// Create packet (global function)
inline std::unique_ptr<IPacket>
createPacket(ProtocolState state, PacketDirection direction, int32_t packetId) {
  return PacketRegistry::getInstance().createPacket(state, direction, packetId);
}

// Check packet existence
inline bool hasPacket(ProtocolState state, PacketDirection direction,
                      int32_t packetId) {
  return PacketRegistry::getInstance().hasPacket(state, direction, packetId);
}

// Return packet information
inline std::string getPacketInfo(ProtocolState state, PacketDirection direction,
                                 int32_t packetId) {
  return PacketRegistry::getInstance().getPacketInfo(state, direction,
                                                     packetId);
}
} // namespace registry

// Packet parser class
class PacketParser {
private:
  PacketRegistry &registry;

public:
  PacketParser() : registry(PacketRegistry::getInstance()) {}

  // Parse raw packet data to create packet object
  std::unique_ptr<IPacket> parsePacket(const RawPacket &rawPacket,
                                       ProtocolState state);

  // Serialize packet object to raw data
  RawPacket serializePacket(const IPacket &packet);

  // Validate packet
  bool validatePacket(const IPacket &packet, ProtocolState expectedState) const;

  // Calculate packet size
  size_t calculatePacketSize(const IPacket &packet) const;
};

// Packet manager class (high-level interface)
class PacketManager {
private:
  PacketParser parser;
  PacketStream stream;
  IPacketLogger *logger;
  PacketStatistics statistics;

public:
  PacketManager() : logger(nullptr) {}
  explicit PacketManager(IPacketLogger *packetLogger) : logger(packetLogger) {}

  // Set logger
  void setLogger(IPacketLogger *packetLogger) { logger = packetLogger; }

  // Compression settings
  void setCompression(int32_t threshold) { stream.setCompression(threshold); }
  void disableCompression() { stream.disableCompression(); }

  // Encryption settings
  void enableEncryption(const std::vector<uint8_t> &sharedSecret) {
    stream.enableEncryption(sharedSecret);
  }
  void disableEncryption() { stream.disableEncryption(); }

  // Prepare packet for sending (serialization)
  std::vector<uint8_t> preparePacketForSending(const IPacket &packet);

  // Process received data
  void processReceivedData(const std::vector<uint8_t> &data);

  // Get next packet
  std::unique_ptr<IPacket> getNextPacket(ProtocolState currentState);

  // Return statistics information
  const PacketStatistics &getStatistics() const { return statistics; }
  void resetStatistics() { statistics = PacketStatistics{}; }

  // Buffer status
  size_t getBufferSize() const { return stream.getBufferSize(); }
  void clearBuffer() { stream.clearBuffer(); }

  // Packet logging
  void logPacket(const IPacket &packet, PacketDirection direction,
                 const std::string &endpoint);
  void logError(const std::string &error, const std::string &context);
};

// Template-based packet creation utility
template <typename PacketType>
inline std::unique_ptr<IPacket> createPacketInstance() {
  return std::make_unique<PacketType>();
}

// Packet registration helper using macros
#define REGISTER_PACKET(registry, state, direction, id, PacketClass)           \
  (registry)->registerPacket(state, direction, id,                             \
                             createPacketInstance<PacketClass>)

// Implementation of function to register all packets
inline void PacketRegistry::registerAllPackets() {
  // Register Handshaking packets
  REGISTER_PACKET(this, ProtocolState::HANDSHAKING,
                  PacketDirection::SERVERBOUND,
                  PacketId::Handshaking::Serverbound::HANDSHAKE,
                  packets::handshaking::HandshakePacket);

  // Register Status packets
  REGISTER_PACKET(this, ProtocolState::STATUS, PacketDirection::SERVERBOUND,
                  PacketId::Status::Serverbound::STATUS_REQUEST,
                  packets::status::StatusRequestPacket);
  REGISTER_PACKET(this, ProtocolState::STATUS, PacketDirection::SERVERBOUND,
                  PacketId::Status::Serverbound::PING_REQUEST,
                  packets::status::PingRequestPacket);
  REGISTER_PACKET(this, ProtocolState::STATUS, PacketDirection::CLIENTBOUND,
                  PacketId::Status::Clientbound::STATUS_RESPONSE,
                  packets::status::StatusResponsePacket);
  REGISTER_PACKET(this, ProtocolState::STATUS, PacketDirection::CLIENTBOUND,
                  PacketId::Status::Clientbound::PONG_RESPONSE,
                  packets::status::PongResponsePacket);

  // Register Login packets
  REGISTER_PACKET(this, ProtocolState::LOGIN, PacketDirection::SERVERBOUND,
                  PacketId::Login::Serverbound::LOGIN_START,
                  packets::login::LoginStartPacket);
  REGISTER_PACKET(this, ProtocolState::LOGIN, PacketDirection::SERVERBOUND,
                  PacketId::Login::Serverbound::ENCRYPTION_RESPONSE,
                  packets::login::EncryptionResponsePacket);
  REGISTER_PACKET(this, ProtocolState::LOGIN, PacketDirection::CLIENTBOUND,
                  PacketId::Login::Clientbound::DISCONNECT,
                  packets::login::LoginDisconnectPacket);
  REGISTER_PACKET(this, ProtocolState::LOGIN, PacketDirection::CLIENTBOUND,
                  PacketId::Login::Clientbound::ENCRYPTION_REQUEST,
                  packets::login::EncryptionRequestPacket);
  REGISTER_PACKET(this, ProtocolState::LOGIN, PacketDirection::CLIENTBOUND,
                  PacketId::Login::Clientbound::LOGIN_SUCCESS,
                  packets::login::LoginSuccessPacket);
  REGISTER_PACKET(this, ProtocolState::LOGIN, PacketDirection::CLIENTBOUND,
                  PacketId::Login::Clientbound::SET_COMPRESSION,
                  packets::login::SetCompressionPacket);

  // Register Play packets (main packets only)
  REGISTER_PACKET(this, ProtocolState::PLAY, PacketDirection::CLIENTBOUND,
                  PacketId::Play::Clientbound::LOGIN,
                  packets::play::LoginPlayPacket);
  REGISTER_PACKET(this, ProtocolState::PLAY, PacketDirection::CLIENTBOUND,
                  PacketId::Play::Clientbound::BLOCK_CHANGE,
                  packets::play::SetBlockPacket);
  REGISTER_PACKET(this, ProtocolState::PLAY, PacketDirection::CLIENTBOUND,
                  PacketId::Play::Clientbound::KEEP_ALIVE,
                  packets::play::KeepAlivePacket);
  REGISTER_PACKET(this, ProtocolState::PLAY, PacketDirection::SERVERBOUND,
                  PacketId::Play::Serverbound::SET_PLAYER_POSITION,
                  packets::play::SetPlayerPositionPacket);
  REGISTER_PACKET(this, ProtocolState::PLAY, PacketDirection::SERVERBOUND,
                  PacketId::Play::Serverbound::SET_PLAYER_POSITION_AND_ROTATION,
                  packets::play::SetPlayerPositionAndRotationPacket);
  REGISTER_PACKET(this, ProtocolState::PLAY, PacketDirection::SERVERBOUND,
                  PacketId::Play::Serverbound::CHAT_MESSAGE,
                  packets::play::PlayerChatMessagePacket);
  REGISTER_PACKET(this, ProtocolState::PLAY, PacketDirection::SERVERBOUND,
                  PacketId::Play::Serverbound::KEEP_ALIVE,
                  packets::play::KeepAliveResponsePacket);
}

} // namespace minecraft::protocol