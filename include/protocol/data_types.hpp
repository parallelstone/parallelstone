#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace minecraft::protocol {

// Forward declarations
class BitSet;

class ByteBuffer {
private:
  std::vector<uint8_t> data;
  size_t position = 0;

public:
  ByteBuffer() = default;
  explicit ByteBuffer(const std::vector<uint8_t> &data) : data(data) {}
  explicit ByteBuffer(std::vector<uint8_t> &&data) : data(std::move(data)) {}

  size_t size() const { return data.size(); }
  size_t remaining() const { return data.size() - position; }
  bool hasRemaining() const { return position < data.size(); }

  void reset() { position = 0; }
  void clear() {
    data.clear();
    position = 0;
  }

  const std::vector<uint8_t> &getData() const { return data; }
  std::vector<uint8_t> &getData() { return data; }

  // Read basic types
  uint8_t readByte();
  int8_t readSignedByte();
  uint16_t readShort();
  int16_t readSignedShort();
  uint32_t readInt();
  int32_t readSignedInt();
  uint64_t readLong();
  int64_t readSignedLong();
  float readFloat();
  double readDouble();
  bool readBoolean();

  // Write basic types
  void writeByte(uint8_t value);
  void writeSignedByte(int8_t value);
  void writeShort(uint16_t value);
  void writeSignedShort(int16_t value);
  void writeInt(uint32_t value);
  void writeSignedInt(int32_t value);
  void writeLong(uint64_t value);
  void writeSignedLong(int64_t value);
  void writeFloat(float value);
  void writeDouble(double value);
  void writeBoolean(bool value);

  // Variable length integers
  int32_t readVarInt();
  int64_t readVarLong();
  void writeVarInt(int32_t value);
  void writeVarLong(int64_t value);

  // Strings
  std::string readString();
  void writeString(const std::string &value);

  // Byte arrays
  std::vector<uint8_t> readByteArray(size_t length);
  void writeByteArray(const std::vector<uint8_t> &data);

  // Raw data read/write
  void readBytes(uint8_t *dest, size_t length);
  void writeBytes(const uint8_t *src, size_t length);

  // BitSet serialization
  BitSet readBitSet();
  void writeBitSet(const BitSet &bitSet);
};

// UUID type definition
struct UUID {
  uint64_t mostSignificantBits;
  uint64_t leastSignificantBits;

  UUID() : mostSignificantBits(0), leastSignificantBits(0) {}
  UUID(uint64_t msb, uint64_t lsb)
      : mostSignificantBits(msb), leastSignificantBits(lsb) {}

  bool operator==(const UUID &other) const {
    return mostSignificantBits == other.mostSignificantBits &&
           leastSignificantBits == other.leastSignificantBits;
  }

  std::string toString() const;
  static UUID fromString(const std::string &str);
  static UUID random();
};

// Position type definition (block coordinates)
struct Position {
  int32_t x, y, z;

  Position() : x(0), y(0), z(0) {}
  Position(int32_t x, int32_t y, int32_t z) : x(x), y(y), z(z) {}

  int64_t encode() const {
    return ((static_cast<int64_t>(x) & 0x3FFFFFF) << 38) |
           ((static_cast<int64_t>(z) & 0x3FFFFFF) << 12) |
           (static_cast<int64_t>(y) & 0xFFF);
  }

  static Position decode(int64_t encoded) {
    int32_t x = static_cast<int32_t>(encoded >> 38);
    int32_t z = static_cast<int32_t>((encoded >> 12) & 0x3FFFFFF);
    int32_t y = static_cast<int32_t>(encoded & 0xFFF);

    // Sign extension
    if (x >= (1 << 25))
      x -= (1 << 26);
    if (z >= (1 << 25))
      z -= (1 << 26);
    if (y >= (1 << 11))
      y -= (1 << 12);

    return Position(x, y, z);
  }
};

// Angle type definition (angle represented as 0-255)
struct Angle {
  uint8_t value;

  Angle() : value(0) {}
  explicit Angle(uint8_t val) : value(val) {}
  explicit Angle(float degrees)
      : value(static_cast<uint8_t>((degrees / 360.0f) * 256.0f)) {}

  float toDegrees() const { return (value / 256.0f) * 360.0f; }
};

// Chat component (JSON-based)
struct ChatComponent {
  std::string text;
  std::optional<std::string> color;
  std::optional<bool> bold;
  std::optional<bool> italic;
  std::optional<bool> underlined;
  std::optional<bool> strikethrough;
  std::optional<bool> obfuscated;

  std::string toJson() const;
  static ChatComponent fromJson(const std::string &json);
  static ChatComponent fromPlainText(const std::string &text);
};

// Identifier type definition (namespace:path)
struct Identifier {
  std::string nameSpace;
  std::string path;

  Identifier() : nameSpace("minecraft"), path("") {}
  Identifier(const std::string &full);
  Identifier(const std::string &ns, const std::string &p)
      : nameSpace(ns), path(p) {}

  std::string toString() const { return nameSpace + ":" + path; }

  bool operator==(const Identifier &other) const {
    return nameSpace == other.nameSpace && path == other.path;
  }
};

// Slot data (inventory item)
struct Slot {
  bool present;
  int32_t itemId;
  uint8_t itemCount;
  std::vector<uint8_t> nbtData; // NBT data is simply handled as byte array

  Slot() : present(false), itemId(0), itemCount(0) {}
  Slot(int32_t id, uint8_t count, const std::vector<uint8_t> &nbt = {})
      : present(true), itemId(id), itemCount(count), nbtData(nbt) {}
};

// BitSet type definition
class BitSet {
private:
  std::vector<uint64_t> data;
  size_t bitCount;

public:
  explicit BitSet(size_t size = 0);

  void set(size_t index, bool value = true);
  bool get(size_t index) const;
  void clear();
  size_t size() const { return bitCount; }

  const std::vector<uint64_t> &getData() const { return data; }
  std::vector<uint64_t> &getData() { return data; }
};

} // namespace minecraft::protocol