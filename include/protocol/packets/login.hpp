#pragma once

#include "../data_types.hpp"
#include "../packet.hpp"
#include "../protocol_state.hpp"
#include <optional>
#include <vector>

namespace minecraft::protocol::packets::login {

// Player property information
struct PlayerProperty {
  std::string name;                     // Property name (e.g., "textures")
  std::string value;                    // Property value (base64 encoded JSON)
  std::optional<std::string> signature; // Signature (optional)

  PlayerProperty() = default;
  PlayerProperty(const std::string &n, const std::string &v,
                 const std::optional<std::string> &s = std::nullopt)
      : name(n), value(v), signature(s) {}
};

// Serverbound Login Start Packet (0x00)
// Packet sent when client starts login
class LoginStartPacket
    : public PacketBase<ProtocolState::LOGIN, PacketDirection::SERVERBOUND> {
private:
  std::string name;               // Player name
  std::optional<UUID> playerUuid; // Player UUID (used in online mode)

public:
  LoginStartPacket() = default;
  LoginStartPacket(const std::string &playerName,
                   const std::optional<UUID> &uuid = std::nullopt)
      : name(playerName), playerUuid(uuid) {}

  int32_t getPacketId() const override {
    return PacketId::Login::Serverbound::LOGIN_START;
  }

  void serialize(ByteBuffer &buffer) const override {
    buffer.writeString(name);
    buffer.writeBoolean(playerUuid.has_value());
    if (playerUuid.has_value()) {
      buffer.writeLong(playerUuid->mostSignificantBits);
      buffer.writeLong(playerUuid->leastSignificantBits);
    }
  }

  void deserialize(ByteBuffer &buffer) override {
    name = buffer.readString();
    bool hasUuid = buffer.readBoolean();
    if (hasUuid) {
      uint64_t msb = buffer.readLong();
      uint64_t lsb = buffer.readLong();
      playerUuid = UUID(msb, lsb);
    } else {
      playerUuid = std::nullopt;
    }
  }

  std::string toString() const override {
    std::string uuidStr =
        playerUuid.has_value() ? playerUuid->toString() : "none";
    return "LoginStartPacket{name=" + name + ", uuid=" + uuidStr + "}";
  }

  const std::string &getName() const { return name; }
  const std::optional<UUID> &getPlayerUuid() const { return playerUuid; }

  void setName(const std::string &playerName) { name = playerName; }
  void setPlayerUuid(const std::optional<UUID> &uuid) { playerUuid = uuid; }

  bool isValidPlayerName() const {
    return !name.empty() && name.length() <= 16 &&
           name.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQR"
                                  "STUVWXYZ0123456789_") == std::string::npos;
  }
};

// Clientbound Encryption Request Packet (0x01)
// Packet sent by server to client for encryption
class EncryptionRequestPacket
    : public PacketBase<ProtocolState::LOGIN, PacketDirection::CLIENTBOUND> {
private:
  std::string serverId;             // Server ID (usually empty string)
  std::vector<uint8_t> publicKey;   // RSA public key
  std::vector<uint8_t> verifyToken; // Verify token (4-byte random data)

public:
  EncryptionRequestPacket() = default;
  EncryptionRequestPacket(const std::string &id,
                          const std::vector<uint8_t> &key,
                          const std::vector<uint8_t> &token)
      : serverId(id), publicKey(key), verifyToken(token) {}

  int32_t getPacketId() const override {
    return PacketId::Login::Clientbound::ENCRYPTION_REQUEST;
  }

  void serialize(ByteBuffer &buffer) const override {
    buffer.writeString(serverId);
    buffer.writeVarInt(static_cast<int32_t>(publicKey.size()));
    buffer.writeByteArray(publicKey);
    buffer.writeVarInt(static_cast<int32_t>(verifyToken.size()));
    buffer.writeByteArray(verifyToken);
  }

  void deserialize(ByteBuffer &buffer) override {
    serverId = buffer.readString();
    int32_t keyLength = buffer.readVarInt();
    publicKey = buffer.readByteArray(keyLength);
    int32_t tokenLength = buffer.readVarInt();
    verifyToken = buffer.readByteArray(tokenLength);
  }

  std::string toString() const override {
    return "EncryptionRequestPacket{serverId=" + serverId +
           ", publicKeyLength=" + std::to_string(publicKey.size()) +
           ", verifyTokenLength=" + std::to_string(verifyToken.size()) + "}";
  }

  const std::string &getServerId() const { return serverId; }
  const std::vector<uint8_t> &getPublicKey() const { return publicKey; }
  const std::vector<uint8_t> &getVerifyToken() const { return verifyToken; }

  void setServerId(const std::string &id) { serverId = id; }
  void setPublicKey(const std::vector<uint8_t> &key) { publicKey = key; }
  void setVerifyToken(const std::vector<uint8_t> &token) {
    verifyToken = token;
  }
};

// Serverbound Encryption Response Packet (0x01)
// Packet sent by client in response to encryption request
class EncryptionResponsePacket
    : public PacketBase<ProtocolState::LOGIN, PacketDirection::SERVERBOUND> {
private:
  std::vector<uint8_t> sharedSecret; // AES shared secret encrypted with RSA
  std::vector<uint8_t> verifyToken;  // Verify token encrypted with RSA

public:
  EncryptionResponsePacket() = default;
  EncryptionResponsePacket(const std::vector<uint8_t> &secret,
                           const std::vector<uint8_t> &token)
      : sharedSecret(secret), verifyToken(token) {}

  int32_t getPacketId() const override {
    return PacketId::Login::Serverbound::ENCRYPTION_RESPONSE;
  }

  void serialize(ByteBuffer &buffer) const override {
    buffer.writeVarInt(static_cast<int32_t>(sharedSecret.size()));
    buffer.writeByteArray(sharedSecret);
    buffer.writeVarInt(static_cast<int32_t>(verifyToken.size()));
    buffer.writeByteArray(verifyToken);
  }

  void deserialize(ByteBuffer &buffer) override {
    int32_t secretLength = buffer.readVarInt();
    sharedSecret = buffer.readByteArray(secretLength);
    int32_t tokenLength = buffer.readVarInt();
    verifyToken = buffer.readByteArray(tokenLength);
  }

  std::string toString() const override {
    return "EncryptionResponsePacket{sharedSecretLength=" +
           std::to_string(sharedSecret.size()) +
           ", verifyTokenLength=" + std::to_string(verifyToken.size()) + "}";
  }

  const std::vector<uint8_t> &getSharedSecret() const { return sharedSecret; }
  const std::vector<uint8_t> &getVerifyToken() const { return verifyToken; }

  void setSharedSecret(const std::vector<uint8_t> &secret) {
    sharedSecret = secret;
  }
  void setVerifyToken(const std::vector<uint8_t> &token) {
    verifyToken = token;
  }
};

// Clientbound Login Success Packet (0x02)
// Packet sent by server to notify login success
class LoginSuccessPacket
    : public PacketBase<ProtocolState::LOGIN, PacketDirection::CLIENTBOUND> {
private:
  UUID uuid;            // Player UUID
  std::string username; // Player name
  std::vector<PlayerProperty>
      properties; // Player properties (skin, cape, etc.)

public:
  LoginSuccessPacket() = default;
  LoginSuccessPacket(const UUID &playerUuid, const std::string &playerName,
                     const std::vector<PlayerProperty> &props = {})
      : uuid(playerUuid), username(playerName), properties(props) {}

  int32_t getPacketId() const override {
    return PacketId::Login::Clientbound::LOGIN_SUCCESS;
  }

  void serialize(ByteBuffer &buffer) const override {
    buffer.writeLong(uuid.mostSignificantBits);
    buffer.writeLong(uuid.leastSignificantBits);
    buffer.writeString(username);
    buffer.writeVarInt(static_cast<int32_t>(properties.size()));

    for (const auto &prop : properties) {
      buffer.writeString(prop.name);
      buffer.writeString(prop.value);
      buffer.writeBoolean(prop.signature.has_value());
      if (prop.signature.has_value()) {
        buffer.writeString(prop.signature.value());
      }
    }
  }

  void deserialize(ByteBuffer &buffer) override {
    uuid.mostSignificantBits = buffer.readLong();
    uuid.leastSignificantBits = buffer.readLong();
    username = buffer.readString();
    int32_t propCount = buffer.readVarInt();

    properties.clear();
    properties.reserve(propCount);

    for (int32_t i = 0; i < propCount; ++i) {
      PlayerProperty prop;
      prop.name = buffer.readString();
      prop.value = buffer.readString();
      bool hasSignature = buffer.readBoolean();
      if (hasSignature) {
        prop.signature = buffer.readString();
      }
      properties.push_back(prop);
    }
  }

  std::string toString() const override {
    return "LoginSuccessPacket{uuid=" + uuid.toString() +
           ", username=" + username +
           ", properties=" + std::to_string(properties.size()) + "}";
  }

  const UUID &getUuid() const { return uuid; }
  const std::string &getUsername() const { return username; }
  const std::vector<PlayerProperty> &getProperties() const {
    return properties;
  }

  void setUuid(const UUID &playerUuid) { uuid = playerUuid; }
  void setUsername(const std::string &name) { username = name; }
  void setProperties(const std::vector<PlayerProperty> &props) {
    properties = props;
  }
  void addProperty(const PlayerProperty &prop) { properties.push_back(prop); }
};

// Clientbound Set Compression Packet (0x03)
// Packet sent by server to notify compression settings
class SetCompressionPacket
    : public PacketBase<ProtocolState::LOGIN, PacketDirection::CLIENTBOUND> {
private:
  int32_t threshold; // Compression threshold (-1 disables compression)

public:
  SetCompressionPacket() : threshold(-1) {}
  explicit SetCompressionPacket(int32_t compressionThreshold)
      : threshold(compressionThreshold) {}

  int32_t getPacketId() const override {
    return PacketId::Login::Clientbound::SET_COMPRESSION;
  }

  void serialize(ByteBuffer &buffer) const override {
    buffer.writeVarInt(threshold);
  }

  void deserialize(ByteBuffer &buffer) override {
    threshold = buffer.readVarInt();
  }

  std::string toString() const override {
    return "SetCompressionPacket{threshold=" + std::to_string(threshold) + "}";
  }

  int32_t getThreshold() const { return threshold; }
  void setThreshold(int32_t compressionThreshold) {
    threshold = compressionThreshold;
  }

  bool isCompressionEnabled() const { return threshold >= 0; }
};

// Clientbound Disconnect Packet (0x00)
// Packet sent by server when disconnecting during login
class LoginDisconnectPacket
    : public PacketBase<ProtocolState::LOGIN, PacketDirection::CLIENTBOUND> {
private:
  ChatComponent reason; // Disconnect reason

public:
  LoginDisconnectPacket() = default;
  explicit LoginDisconnectPacket(const ChatComponent &disconnectReason)
      : reason(disconnectReason) {}
  explicit LoginDisconnectPacket(const std::string &reasonText)
      : reason(ChatComponent::fromPlainText(reasonText)) {}

  int32_t getPacketId() const override {
    return PacketId::Login::Clientbound::DISCONNECT;
  }

  void serialize(ByteBuffer &buffer) const override {
    buffer.writeString(reason.toJson());
  }

  void deserialize(ByteBuffer &buffer) override {
    std::string json = buffer.readString();
    reason = ChatComponent::fromJson(json);
  }

  std::string toString() const override {
    return "LoginDisconnectPacket{reason=" + reason.toJson() + "}";
  }

  const ChatComponent &getReason() const { return reason; }
  void setReason(const ChatComponent &disconnectReason) {
    reason = disconnectReason;
  }
  void setReason(const std::string &reasonText) {
    reason = ChatComponent::fromPlainText(reasonText);
  }
};

// Login state packet factory
class LoginPacketFactory : public IPacketFactory {
public:
  std::unique_ptr<IPacket> createPacket(int32_t packetId, ProtocolState state,
                                        PacketDirection direction) override {
    if (state != ProtocolState::LOGIN) {
      return nullptr;
    }

    if (direction == PacketDirection::SERVERBOUND) {
      switch (packetId) {
        // Add packet creation logic here if needed
      }
    }

    return nullptr;
  }
};

// Login related utilities
namespace utils {

// Create default texture property
inline PlayerProperty createTexturesProperty(
    const std::string &textureData,
    const std::optional<std::string> &signature = std::nullopt) {
  return PlayerProperty("textures", textureData, signature);
}

} // namespace utils

} // namespace minecraft::protocol::packets::login