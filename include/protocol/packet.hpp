#pragma once

#include "data_types.hpp"
#include "protocol_state.hpp"
#include <memory>
#include <string>
#include <vector>

namespace minecraft::protocol {

// Basic packet interface
class IPacket {
public:
  virtual ~IPacket() = default;

  // Packet identification information
  virtual int32_t getPacketId() const = 0;
  virtual ProtocolState getProtocolState() const = 0;
  virtual PacketDirection getDirection() const = 0;

  // Serialization/Deserialization
  virtual void serialize(ByteBuffer &buffer) const = 0;
  virtual void deserialize(ByteBuffer &buffer) = 0;

  // For debugging
  virtual std::string toString() const = 0;
};

// Packet base class template
template <ProtocolState State, PacketDirection Direction>
class PacketBase : public IPacket {
public:
  ProtocolState getProtocolState() const override { return State; }

  PacketDirection getDirection() const override { return Direction; }
};

// Packet header information
struct PacketHeader {
  int32_t length;     // Total packet length
  int32_t dataLength; // Original data length if compressed (0 if uncompressed)
  int32_t packetId;   // Packet ID

  PacketHeader() : length(0), dataLength(0), packetId(0) {}
  PacketHeader(int32_t len, int32_t dataLen, int32_t id)
      : length(len), dataLength(dataLen), packetId(id) {}

  bool isCompressed() const { return dataLength > 0; }
};

// Raw packet data (before parsing)
class RawPacket {
private:
  PacketHeader header;
  std::vector<uint8_t> data;

public:
  RawPacket() = default;
  RawPacket(const PacketHeader &h, std::vector<uint8_t> d)
      : header(h), data(std::move(d)) {}

  const PacketHeader &getHeader() const { return header; }
  const std::vector<uint8_t> &getData() const { return data; }

  void setHeader(const PacketHeader &h) { header = h; }
  void setData(std::vector<uint8_t> d) { data = std::move(d); }

  size_t size() const { return data.size(); }
  bool empty() const { return data.empty(); }
};

// Packet factory interface
class IPacketFactory {
public:
  virtual ~IPacketFactory() = default;
  virtual std::unique_ptr<IPacket> createPacket(int32_t packetId,
                                                ProtocolState state,
                                                PacketDirection direction) = 0;
};

// Packet compression related
class PacketCompression {
private:
  int32_t threshold;
  bool enabled;

public:
  PacketCompression() : threshold(-1), enabled(false) {}
  explicit PacketCompression(int32_t thresh)
      : threshold(thresh), enabled(thresh >= 0) {}

  bool isEnabled() const { return enabled; }
  int32_t getThreshold() const { return threshold; }

  void setThreshold(int32_t thresh) {
    threshold = thresh;
    enabled = thresh >= 0;
  }

  void disable() {
    threshold = -1;
    enabled = false;
  }

  // Check if compression is needed
  bool shouldCompress(size_t dataSize) const {
    return enabled && static_cast<int32_t>(dataSize) >= threshold;
  }

  // Compress data
  std::vector<uint8_t> compress(const std::vector<uint8_t> &data) const;

  // Decompress data
  std::vector<uint8_t> decompress(const std::vector<uint8_t> &compressedData,
                                  size_t originalSize) const;
};

// Packet encryption related
class PacketEncryption {
private:
  bool enabled;
  std::vector<uint8_t> sharedSecret;
  std::vector<uint8_t> initVector;

public:
  PacketEncryption() : enabled(false) {}

  bool isEnabled() const { return enabled; }

  void enable(const std::vector<uint8_t> &secret) {
    sharedSecret = secret;
    initVector = secret; // Minecraft uses same data for IV
    enabled = true;
  }

  void disable() {
    enabled = false;
    sharedSecret.clear();
    initVector.clear();
  }

  // Encrypt data
  std::vector<uint8_t> encrypt(const std::vector<uint8_t> &data) const;

  // Decrypt data
  std::vector<uint8_t> decrypt(const std::vector<uint8_t> &encryptedData) const;
};

// Packet stream processing class
class PacketStream {
private:
  ByteBuffer buffer;
  PacketCompression compression;
  PacketEncryption encryption;

public:
  PacketStream() = default;

  // Compression settings
  void setCompression(int32_t threshold) {
    compression.setThreshold(threshold);
  }

  void disableCompression() { compression.disable(); }

  // 암호화 설정
  void enableEncryption(const std::vector<uint8_t> &sharedSecret) {
    encryption.enable(sharedSecret);
  }

  void disableEncryption() { encryption.disable(); }

  // 패킷 직렬화 (전송용)
  std::vector<uint8_t> serializePacket(const IPacket &packet) const;

  // 패킷 역직렬화 (수신용)
  std::unique_ptr<RawPacket>
  deserializePacket(const std::vector<uint8_t> &data);

  // 데이터 추가 (스트림 방식)
  void addData(const std::vector<uint8_t> &data);

  // 완전한 패킷이 있는지 확인
  bool hasCompletePacket() const;

  // 다음 패킷 추출
  std::unique_ptr<RawPacket> extractNextPacket();

  // 버퍼 상태
  size_t getBufferSize() const { return buffer.size(); }
  void clearBuffer() { buffer.clear(); }
};

// 패킷 검증 관련
class PacketValidator {
public:
  // 패킷 크기 검증
  static bool validatePacketSize(const RawPacket &packet,
                                 size_t maxSize = 2097151); // 2^21 - 1

  // 패킷 ID 검증
  static bool validatePacketId(int32_t packetId, ProtocolState state,
                               PacketDirection direction);

  // 프로토콜 상태 전환 검증
  static bool validateStateTransition(ProtocolState from, ProtocolState to);

  // 문자열 길이 검증
  static bool validateStringLength(const std::string &str,
                                   size_t maxLength = 32767);

  // 배열 길이 검증
  static bool validateArrayLength(size_t length, size_t maxLength);
};

// 패킷 통계 정보
struct PacketStatistics {
  uint64_t totalPacketsSent = 0;
  uint64_t totalPacketsReceived = 0;
  uint64_t totalBytesSent = 0;
  uint64_t totalBytesReceived = 0;
  uint64_t compressionSavings = 0;
  uint64_t encryptionOverhead = 0;

  void recordSentPacket(size_t bytes) {
    totalPacketsSent++;
    totalBytesSent += bytes;
  }

  void recordReceivedPacket(size_t bytes) {
    totalPacketsReceived++;
    totalBytesReceived += bytes;
  }

  void recordCompressionSaving(size_t original, size_t compressed) {
    if (original > compressed) {
      compressionSavings += (original - compressed);
    }
  }

  std::string toString() const;
};

// 패킷 로깅 인터페이스
class IPacketLogger {
public:
  virtual ~IPacketLogger() = default;
  virtual void logPacket(const IPacket &packet, PacketDirection direction,
                         const std::string &endpoint) = 0;
  virtual void logRawPacket(const RawPacket &packet, PacketDirection direction,
                            const std::string &endpoint) = 0;
  virtual void logError(const std::string &error,
                        const std::string &context) = 0;
};

} // namespace minecraft::protocol