#pragma once

#include "../data_types.hpp"
#include "../packet.hpp"
#include "../protocol_state.hpp"
#include <optional>
#include <vector>

namespace minecraft::protocol::packets::play {

// Player ability flags
struct PlayerAbilities {
  bool invulnerable = false; // Invulnerable state
  bool flying = false;       // Currently flying
  bool allowFlying = false;  // Flying allowed
  bool creativeMode = false; // Creative mode
  float flyingSpeed = 0.05f; // Flying speed
  float walkingSpeed = 0.1f; // Walking speed
};

// Clientbound Login (Play) Packet (0x29)
// Packet sent when player enters the game
class LoginPlayPacket
    : public PacketBase<ProtocolState::PLAY, PacketDirection::CLIENTBOUND> {
private:
  int32_t entityId;                          // Player entity ID
  bool isHardcore;                           // Whether hardcore mode
  std::vector<Identifier> dimensionNames;    // Dimension names
  int32_t maxPlayers;                        // Maximum number of players
  int32_t viewDistance;                      // View distance
  int32_t simulationDistance;                // Simulation distance
  bool reducedDebugInfo;                     // Reduced debug info
  bool enableRespawnScreen;                  // Enable respawn screen
  bool doLimitedCrafting;                    // Limited crafting
  Identifier dimensionType;                  // Dimension type
  Identifier dimensionName;                  // Current dimension name
  int64_t hashedSeed;                        // Hashed seed
  GameMode gameMode;                         // Game mode
  GameMode previousGameMode;                 // Previous game mode
  bool isDebug;                              // Debug world
  bool isFlat;                               // 평지 월드
  std::optional<Position> lastDeathLocation; // 마지막 사망 위치

public:
  LoginPlayPacket()
      : entityId(0), isHardcore(false), maxPlayers(0), viewDistance(0),
        simulationDistance(0), reducedDebugInfo(false),
        enableRespawnScreen(true), doLimitedCrafting(false), hashedSeed(0),
        gameMode(GameMode::SURVIVAL), previousGameMode(GameMode::SURVIVAL),
        isDebug(false), isFlat(false) {}

  int32_t getPacketId() const override {
    return PacketId::Play::Clientbound::LOGIN;
  }

  void serialize(ByteBuffer &buffer) const override {
    buffer.writeSignedInt(entityId);
    buffer.writeBoolean(isHardcore);
    buffer.writeVarInt(static_cast<int32_t>(dimensionNames.size()));
    for (const auto &name : dimensionNames) {
      buffer.writeString(name.toString());
    }
    buffer.writeVarInt(maxPlayers);
    buffer.writeVarInt(viewDistance);
    buffer.writeVarInt(simulationDistance);
    buffer.writeBoolean(reducedDebugInfo);
    buffer.writeBoolean(enableRespawnScreen);
    buffer.writeBoolean(doLimitedCrafting);
    buffer.writeString(dimensionType.toString());
    buffer.writeString(dimensionName.toString());
    buffer.writeSignedLong(hashedSeed);
    buffer.writeByte(static_cast<uint8_t>(gameMode));
    buffer.writeSignedByte(static_cast<int8_t>(previousGameMode));
    buffer.writeBoolean(isDebug);
    buffer.writeBoolean(isFlat);
    buffer.writeBoolean(lastDeathLocation.has_value());
    if (lastDeathLocation.has_value()) {
      buffer.writeString(dimensionName.toString());
      buffer.writeSignedLong(lastDeathLocation->encode());
    }
  }

  void deserialize(ByteBuffer &buffer) override {
    entityId = buffer.readSignedInt();
    isHardcore = buffer.readBoolean();
    int32_t dimCount = buffer.readVarInt();
    dimensionNames.clear();
    dimensionNames.reserve(dimCount);
    for (int32_t i = 0; i < dimCount; ++i) {
      dimensionNames.emplace_back(buffer.readString());
    }
    maxPlayers = buffer.readVarInt();
    viewDistance = buffer.readVarInt();
    simulationDistance = buffer.readVarInt();
    reducedDebugInfo = buffer.readBoolean();
    enableRespawnScreen = buffer.readBoolean();
    doLimitedCrafting = buffer.readBoolean();
    dimensionType = Identifier(buffer.readString());
    dimensionName = Identifier(buffer.readString());
    hashedSeed = buffer.readSignedLong();
    gameMode = static_cast<GameMode>(buffer.readByte());
    previousGameMode = static_cast<GameMode>(buffer.readSignedByte());
    isDebug = buffer.readBoolean();
    isFlat = buffer.readBoolean();
    bool hasDeathLocation = buffer.readBoolean();
    if (hasDeathLocation) {
      buffer.readString(); // dimension name
      lastDeathLocation = Position::decode(buffer.readSignedLong());
    }
  }

  std::string toString() const override {
    return "LoginPlayPacket{entityId=" + std::to_string(entityId) +
           ", gameMode=" + std::to_string(static_cast<int>(gameMode)) +
           ", dimensionName=" + dimensionName.toString() + "}";
  }

  // Getters
  int32_t getEntityId() const { return entityId; }
  bool getIsHardcore() const { return isHardcore; }
  GameMode getGameMode() const { return gameMode; }
  const Identifier &getDimensionName() const { return dimensionName; }
  int32_t getViewDistance() const { return viewDistance; }
  int32_t getSimulationDistance() const { return simulationDistance; }

  // Setters
  void setEntityId(int32_t id) { entityId = id; }
  void setGameMode(GameMode mode) { gameMode = mode; }
  void setDimensionName(const Identifier &name) { dimensionName = name; }
  void setViewDistance(int32_t distance) { viewDistance = distance; }
  void setSimulationDistance(int32_t distance) {
    simulationDistance = distance;
  }
};

// Serverbound Set Player Position Packet (0x13)
// 클라이언트가 플레이어 위치를 서버에 알리는 패킷
class SetPlayerPositionPacket
    : public PacketBase<ProtocolState::PLAY, PacketDirection::SERVERBOUND> {
private:
  double x, y, z; // 플레이어 좌표
  bool onGround;  // 땅에 있는지 여부

public:
  SetPlayerPositionPacket() : x(0.0), y(0.0), z(0.0), onGround(false) {}
  SetPlayerPositionPacket(double px, double py, double pz, bool ground)
      : x(px), y(py), z(pz), onGround(ground) {}

  int32_t getPacketId() const override {
    return PacketId::Play::Serverbound::SET_PLAYER_POSITION;
  }

  void serialize(ByteBuffer &buffer) const override {
    buffer.writeDouble(x);
    buffer.writeDouble(y);
    buffer.writeDouble(z);
    buffer.writeBoolean(onGround);
  }

  void deserialize(ByteBuffer &buffer) override {
    x = buffer.readDouble();
    y = buffer.readDouble();
    z = buffer.readDouble();
    onGround = buffer.readBoolean();
  }

  std::string toString() const override {
    return "SetPlayerPositionPacket{x=" + std::to_string(x) +
           ", y=" + std::to_string(y) + ", z=" + std::to_string(z) +
           ", onGround=" + (onGround ? "true" : "false") + "}";
  }

  double getX() const { return x; }
  double getY() const { return y; }
  double getZ() const { return z; }
  bool isOnGround() const { return onGround; }

  void setPosition(double px, double py, double pz) {
    x = px;
    y = py;
    z = pz;
  }
  void setOnGround(bool ground) { onGround = ground; }
};

// Serverbound Set Player Position and Rotation Packet (0x14)
// 클라이언트가 플레이어 위치와 회전을 서버에 알리는 패킷
class SetPlayerPositionAndRotationPacket
    : public PacketBase<ProtocolState::PLAY, PacketDirection::SERVERBOUND> {
private:
  double x, y, z;   // 플레이어 좌표
  float yaw, pitch; // 회전값 (도 단위)
  bool onGround;    // 땅에 있는지 여부

public:
  SetPlayerPositionAndRotationPacket()
      : x(0.0), y(0.0), z(0.0), yaw(0.0f), pitch(0.0f), onGround(false) {}
  SetPlayerPositionAndRotationPacket(double px, double py, double pz, float y,
                                     float p, bool ground)
      : x(px), y(py), z(pz), yaw(y), pitch(p), onGround(ground) {}

  int32_t getPacketId() const override {
    return PacketId::Play::Serverbound::SET_PLAYER_POSITION_AND_ROTATION;
  }

  void serialize(ByteBuffer &buffer) const override {
    buffer.writeDouble(x);
    buffer.writeDouble(y);
    buffer.writeDouble(z);
    buffer.writeFloat(yaw);
    buffer.writeFloat(pitch);
    buffer.writeBoolean(onGround);
  }

  void deserialize(ByteBuffer &buffer) override {
    x = buffer.readDouble();
    y = buffer.readDouble();
    z = buffer.readDouble();
    yaw = buffer.readFloat();
    pitch = buffer.readFloat();
    onGround = buffer.readBoolean();
  }

  std::string toString() const override {
    return "SetPlayerPositionAndRotationPacket{x=" + std::to_string(x) +
           ", y=" + std::to_string(y) + ", z=" + std::to_string(z) +
           ", yaw=" + std::to_string(yaw) + ", pitch=" + std::to_string(pitch) +
           ", onGround=" + (onGround ? "true" : "false") + "}";
  }

  double getX() const { return x; }
  double getY() const { return y; }
  double getZ() const { return z; }
  float getYaw() const { return yaw; }
  float getPitch() const { return pitch; }
  bool isOnGround() const { return onGround; }

  void setPosition(double px, double py, double pz) {
    x = px;
    y = py;
    z = pz;
  }
  void setRotation(float y, float p) {
    yaw = y;
    pitch = p;
  }
  void setOnGround(bool ground) { onGround = ground; }
};

// Clientbound Set Block Packet (0x09)
// 서버가 클라이언트에게 블록 변경을 알리는 패킷
class SetBlockPacket
    : public PacketBase<ProtocolState::PLAY, PacketDirection::CLIENTBOUND> {
private:
  Position location; // 블록 위치
  int32_t blockId;   // 블록 상태 ID

public:
  SetBlockPacket() : blockId(0) {}
  SetBlockPacket(const Position &pos, int32_t id)
      : location(pos), blockId(id) {}

  int32_t getPacketId() const override {
    return PacketId::Play::Clientbound::BLOCK_CHANGE;
  }

  void serialize(ByteBuffer &buffer) const override {
    buffer.writeSignedLong(location.encode());
    buffer.writeVarInt(blockId);
  }

  void deserialize(ByteBuffer &buffer) override {
    location = Position::decode(buffer.readSignedLong());
    blockId = buffer.readVarInt();
  }

  std::string toString() const override {
    return "SetBlockPacket{location=(" + std::to_string(location.x) + ", " +
           std::to_string(location.y) + ", " + std::to_string(location.z) +
           "), blockId=" + std::to_string(blockId) + "}";
  }

  const Position &getLocation() const { return location; }
  int32_t getBlockId() const { return blockId; }

  void setLocation(const Position &pos) { location = pos; }
  void setBlockId(int32_t id) { blockId = id; }
};

// Serverbound Player Chat Message Packet (0x05)
// 클라이언트가 채팅 메시지를 보내는 패킷
class PlayerChatMessagePacket
    : public PacketBase<ProtocolState::PLAY, PacketDirection::SERVERBOUND> {
private:
  std::string message;                           // 채팅 메시지
  int64_t timestamp;                             // 타임스탬프
  int64_t salt;                                  // 솔트
  std::optional<std::vector<uint8_t>> signature; // 메시지 서명
  int32_t messageCount;                          // 메시지 개수
  BitSet acknowledged;                           // 확인된 메시지들

public:
  PlayerChatMessagePacket() : timestamp(0), salt(0), messageCount(0) {}
  PlayerChatMessagePacket(const std::string &msg, int64_t ts, int64_t s,
                          int32_t count)
      : message(msg), timestamp(ts), salt(s), messageCount(count) {}

  int32_t getPacketId() const override {
    return PacketId::Play::Serverbound::CHAT_MESSAGE;
  }

  void serialize(ByteBuffer &buffer) const override {
    buffer.writeString(message);
    buffer.writeSignedLong(timestamp);
    buffer.writeSignedLong(salt);
    buffer.writeBoolean(signature.has_value());
    if (signature.has_value()) {
      buffer.writeVarInt(static_cast<int32_t>(signature->size()));
      buffer.writeByteArray(*signature);
    }
    buffer.writeVarInt(messageCount);
    buffer.writeBitSet(acknowledged);
  }

  void deserialize(ByteBuffer &buffer) override {
    message = buffer.readString();
    timestamp = buffer.readSignedLong();
    salt = buffer.readSignedLong();
    bool hasSignature = buffer.readBoolean();
    if (hasSignature) {
      int32_t sigLength = buffer.readVarInt();
      signature = buffer.readByteArray(sigLength);
    }
    messageCount = buffer.readVarInt();
    acknowledged = buffer.readBitSet();
  }

  std::string toString() const override {
    return "PlayerChatMessagePacket{message=" + message +
           ", timestamp=" + std::to_string(timestamp) +
           ", messageCount=" + std::to_string(messageCount) + "}";
  }

  const std::string &getMessage() const { return message; }
  int64_t getTimestamp() const { return timestamp; }
  int64_t getSalt() const { return salt; }
  int32_t getMessageCount() const { return messageCount; }

  void setMessage(const std::string &msg) { message = msg; }
  void setTimestamp(int64_t ts) { timestamp = ts; }
  void setSalt(int64_t s) { salt = s; }
  void setMessageCount(int32_t count) { messageCount = count; }
};

// Clientbound Keep Alive Packet (0x24)
// 서버가 클라이언트에게 연결 유지를 확인하는 패킷
class KeepAlivePacket
    : public PacketBase<ProtocolState::PLAY, PacketDirection::CLIENTBOUND> {
private:
  int64_t keepAliveId; // Keep Alive ID

public:
  KeepAlivePacket() : keepAliveId(0) {}
  explicit KeepAlivePacket(int64_t id) : keepAliveId(id) {}

  int32_t getPacketId() const override {
    return PacketId::Play::Clientbound::KEEP_ALIVE;
  }

  void serialize(ByteBuffer &buffer) const override {
    buffer.writeSignedLong(keepAliveId);
  }

  void deserialize(ByteBuffer &buffer) override {
    keepAliveId = buffer.readSignedLong();
  }

  std::string toString() const override {
    return "KeepAlivePacket{id=" + std::to_string(keepAliveId) + "}";
  }

  int64_t getKeepAliveId() const { return keepAliveId; }
  void setKeepAliveId(int64_t id) { keepAliveId = id; }
};

// Serverbound Keep Alive Response Packet (0x11)
// 클라이언트가 Keep Alive에 응답하는 패킷
class KeepAliveResponsePacket
    : public PacketBase<ProtocolState::PLAY, PacketDirection::SERVERBOUND> {
private:
  int64_t keepAliveId; // Keep Alive ID

public:
  KeepAliveResponsePacket() : keepAliveId(0) {}
  explicit KeepAliveResponsePacket(int64_t id) : keepAliveId(id) {}

  int32_t getPacketId() const override {
    return PacketId::Play::Serverbound::KEEP_ALIVE;
  }

  void serialize(ByteBuffer &buffer) const override {
    buffer.writeSignedLong(keepAliveId);
  }

  void deserialize(ByteBuffer &buffer) override {
    keepAliveId = buffer.readSignedLong();
  }

  std::string toString() const override {
    return "KeepAliveResponsePacket{id=" + std::to_string(keepAliveId) + "}";
  }

  int64_t getKeepAliveId() const { return keepAliveId; }
  void setKeepAliveId(int64_t id) { keepAliveId = id; }
};

// Play 상태 패킷 팩토리 (일부 패킷만)
class PlayPacketFactory : public IPacketFactory {
public:
  std::unique_ptr<IPacket> createPacket(int32_t packetId, ProtocolState state,
                                        PacketDirection direction) override {
    if (state != ProtocolState::PLAY) {
      return nullptr;
    }

    if (direction == PacketDirection::SERVERBOUND) {
      switch (packetId) {
      case PacketId::Play::Serverbound::SET_PLAYER_POSITION:
        return std::make_unique<SetPlayerPositionPacket>();
      case PacketId::Play::Serverbound::SET_PLAYER_POSITION_AND_ROTATION:
        return std::make_unique<SetPlayerPositionAndRotationPacket>();
      case PacketId::Play::Serverbound::CHAT_MESSAGE:
        return std::make_unique<PlayerChatMessagePacket>();
      case PacketId::Play::Serverbound::KEEP_ALIVE:
        return std::make_unique<KeepAliveResponsePacket>();
      default:
        return nullptr;
      }
    } else if (direction == PacketDirection::CLIENTBOUND) {
      switch (packetId) {
      case PacketId::Play::Clientbound::LOGIN:
        return std::make_unique<LoginPlayPacket>();
      case PacketId::Play::Clientbound::BLOCK_CHANGE:
        return std::make_unique<SetBlockPacket>();
      case PacketId::Play::Clientbound::KEEP_ALIVE:
        return std::make_unique<KeepAlivePacket>();
      default:
        return nullptr;
      }
    }

    return nullptr;
  }
};

// Play 관련 유틸리티
namespace utils {
// 기본 로그인 패킷 생성
inline std::unique_ptr<LoginPlayPacket> createDefaultLoginPacket(
    int32_t entityId, GameMode gameMode = GameMode::SURVIVAL,
    const std::string &dimensionName = "minecraft:overworld") {
  auto packet = std::make_unique<LoginPlayPacket>();
  packet->setEntityId(entityId);
  packet->setGameMode(gameMode);
  packet->setDimensionName(Identifier(dimensionName));
  packet->setViewDistance(10);
  packet->setSimulationDistance(10);
  return packet;
}

// 블록 상태 ID 계산 (간단한 예시)
inline int32_t calculateBlockStateId(const std::string &blockName) {
  // 실제로는 복잡한 블록 상태 계산이 필요
  if (blockName == "minecraft:air")
    return 0;
  if (blockName == "minecraft:stone")
    return 1;
  if (blockName == "minecraft:grass_block")
    return 2;
  if (blockName == "minecraft:dirt")
    return 3;
  return 0; // 기본값
}

// Keep Alive ID 생성
inline int64_t generateKeepAliveId() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
}

// 플레이어 위치 유효성 검사
inline bool isValidPlayerPosition(double x, double y, double z) {
  // 월드 경계 검사 (예시)
  constexpr double MAX_COORD = 30000000.0;
  constexpr double MIN_Y = -64.0;
  constexpr double MAX_Y = 320.0;

  return std::abs(x) <= MAX_COORD && std::abs(z) <= MAX_COORD && y >= MIN_Y &&
         y <= MAX_Y;
}
} // namespace utils

} // namespace minecraft::protocol::packets::play