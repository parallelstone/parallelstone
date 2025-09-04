#pragma once

#include <vector>
#include <cstdint>
#include <string>
#include <stdexcept>
#include <utility>
#include <optional>

namespace parallelstone {
namespace network {

/**
 * @brief High-performance binary buffer for network protocol data
 *
 * This class provides efficient reading and writing of binary data with
 * support for Minecraft-specific data types like VarInt, VarLong, and
 * network byte order conversion.
 */
class Buffer {
 private:
  std::vector<std::uint8_t> data_;
  std::size_t read_pos_;
  std::size_t write_pos_;

  /**
   * @brief Ensures the buffer has enough capacity for writing
   * @param size Number of bytes to reserve
   */
  void ensure_capacity(std::size_t size);

  /**
   * @brief Checks if enough bytes are available for reading
   * @param size Number of bytes required
   * @throws std::runtime_error if not enough bytes available
   */
  void check_read_bounds(std::size_t size) const;

 public:
  /**
   * @brief Default constructor with initial capacity
   * @param initial_capacity Initial buffer capacity in bytes
   */
  explicit Buffer(std::size_t initial_capacity = 4096);

  /**
   * @brief Constructor from existing data
   * @param data Pointer to source data
   * @param size Size of source data
   */
  Buffer(const std::uint8_t* data, std::size_t size);

  /**
   * @brief Constructor from vector
   * @param data Source vector
   */
  explicit Buffer(const std::vector<std::uint8_t>& data);

  // Copy constructor and assignment
  Buffer(const Buffer&) = default;
  Buffer& operator=(const Buffer&) = default;

  // Move constructor and assignment
  Buffer(Buffer&&) noexcept = default;
  Buffer& operator=(Buffer&&) noexcept = default;

  // Destructor
  ~Buffer() = default;

  // ============================================================================
  // BUFFER STATE METHODS
  // ============================================================================

  /**
   * @brief Get current read position
   * @return Current read position in bytes
   */
  std::size_t read_position() const noexcept { return read_pos_; }

  /**
   * @brief Get current write position (size of written data)
   * @return Current write position in bytes
   */
  std::size_t write_position() const noexcept { return write_pos_; }

  /**
   * @brief Get total capacity of buffer
   * @return Total capacity in bytes
   */
  std::size_t capacity() const noexcept { return data_.size(); }

  /**
   * @brief Get number of bytes available for reading
   * @return Number of readable bytes
   */
  std::size_t readable_bytes() const noexcept { return write_pos_ - read_pos_; }

  /**
   * @brief Get number of bytes available for writing
   * @return Number of writable bytes
   */
  std::size_t writable_bytes() const noexcept {
    return data_.size() - write_pos_;
  }

  /**
   * @brief Check if buffer has readable data
   * @return true if readable data is available
   */
  bool has_readable_data() const noexcept { return read_pos_ < write_pos_; }

  /**
   * @brief Reset buffer positions to beginning
   */
  void clear() noexcept {
    read_pos_ = 0;
    write_pos_ = 0;
  }

  /**
   * @brief Reset read position to beginning
   */
  void reset_read_position() noexcept { read_pos_ = 0; }

  /**
   * @brief Set read position
   * @param pos New read position
   * @throws std::runtime_error if position is invalid
   */
  void set_read_position(std::size_t pos);

  /**
   * @brief Get pointer to raw data at current read position
   * @return Pointer to readable data
   */
  const std::uint8_t* data() const noexcept { return data_.data() + read_pos_; }

  /**
   * @brief Get pointer to raw data buffer
   * @return Pointer to entire buffer
   */
  const std::uint8_t* raw_data() const noexcept { return data_.data(); }

  /**
   * @brief Get mutable pointer to raw data buffer
   * @return Mutable pointer to entire buffer
   */
  std::uint8_t* raw_data() noexcept { return data_.data(); }

  /**
   * @brief Get mutable pointer to current write position
   * @return Mutable pointer for writing data
   */
  std::uint8_t* write_ptr() noexcept { return data_.data() + write_pos_; }

  /**
   * @brief Advance write position after manual write
   * @param count Number of bytes written
   */
  void advance_write_position(std::size_t count) { write_pos_ += count; }

  /**
   * @brief Reserve capacity for buffer
   * @param capacity New capacity in bytes
   */
  void reserve(std::size_t capacity);

  // ============================================================================
  // WRITING METHODS
  // ============================================================================

  /**
   * @brief Write raw bytes to buffer
   * @param data Pointer to source data
   * @param size Number of bytes to write
   */
  void write_bytes(const void* data, std::size_t size);

  /**
   * @brief Write a single byte
   * @param value Byte value to write
   */
  void write_byte(std::uint8_t value);

  /**
   * @brief Write boolean as byte
   * @param value Boolean value to write
   */
  void write_bool(bool value);

  /**
   * @brief Write signed byte
   * @param value Signed byte to write
   */
  void write_int8(std::int8_t value);

  /**
   * @brief Write unsigned 16-bit integer in big-endian format
   * @param value 16-bit value to write
   */
  void write_uint16(std::uint16_t value);

  /**
   * @brief Write signed 16-bit integer in big-endian format
   * @param value 16-bit value to write
   */
  void write_int16(std::int16_t value);

  /**
   * @brief Write unsigned 32-bit integer in big-endian format
   * @param value 32-bit value to write
   */
  void write_uint32(std::uint32_t value);

  /**
   * @brief Write signed 32-bit integer in big-endian format
   * @param value 32-bit value to write
   */
  void write_int32(std::int32_t value);

  /**
   * @brief Write unsigned 64-bit integer in big-endian format
   * @param value 64-bit value to write
   */
  void write_uint64(std::uint64_t value);

  /**
   * @brief Write signed 64-bit integer in big-endian format
   * @param value 64-bit value to write
   */
  void write_int64(std::int64_t value);

  /**
   * @brief Write 32-bit float in big-endian format
   * @param value Float value to write
   */
  void write_float(float value);

  /**
   * @brief Write 64-bit double in big-endian format
   * @param value Double value to write
   */
  void write_double(double value);

  /**
   * @brief Write VarInt (variable-length integer)
   * @param value 32-bit integer to write as VarInt
   */
  void write_varint(std::int32_t value);

  /**
   * @brief Write VarLong (variable-length long integer)
   * @param value 64-bit integer to write as VarLong
   */
  void write_varlong(std::int64_t value);

  /**
   * @brief Write string with VarInt length prefix
   * @param value String to write
   */
  void write_string(const std::string& value);

  /**
   * @brief Write C-style string with VarInt length prefix
   * @param value C-style string to write
   */
  void write_string(const char* value);

  /**
   * @brief Write UUID (128-bit identifier)
   * @param most_significant Most significant 64 bits
   * @param least_significant Least significant 64 bits
   */
  void write_uuid(std::uint64_t most_significant,
                  std::uint64_t least_significant);

  // ============================================================================
  // READING METHODS
  // ============================================================================

  /**
   * @brief Read raw bytes from buffer
   * @param data Pointer to destination buffer
   * @param size Number of bytes to read
   */
  void read_bytes(void* data, std::size_t size);

  /**
   * @brief Read a single byte
   * @return Byte value
   */
  std::uint8_t read_byte();

  /**
   * @brief Read boolean from byte
   * @return Boolean value
   */
  bool read_bool();

  /**
   * @brief Read signed byte
   * @return Signed byte value
   */
  std::int8_t read_int8();

  /**
   * @brief Read unsigned 16-bit integer from big-endian format
   * @return 16-bit value
   */
  std::uint16_t read_uint16();

  /**
   * @brief Read signed 16-bit integer from big-endian format
   * @return 16-bit value
   */
  std::int16_t read_int16();

  /**
   * @brief Read unsigned 32-bit integer from big-endian format
   * @return 32-bit value
   */
  std::uint32_t read_uint32();

  /**
   * @brief Read signed 32-bit integer from big-endian format
   * @return 32-bit value
   */
  std::int32_t read_int32();

  /**
   * @brief Read unsigned 64-bit integer from big-endian format
   * @return 64-bit value
   */
  std::uint64_t read_uint64();

  /**
   * @brief Read signed 64-bit integer from big-endian format
   * @return 64-bit value
   */
  std::int64_t read_int64();

  /**
   * @brief Read 32-bit float from big-endian format
   * @return Float value
   */
  float read_float();

  /**
   * @brief Read 64-bit double from big-endian format
   * @return Double value
   */
  double read_double();

  /**
   * @brief Read VarInt (variable-length integer)
   * @return 32-bit integer value
   * @throws std::runtime_error if VarInt is too long or malformed
   */
  std::int32_t read_varint();

  /**
   * @brief Read VarLong (variable-length long integer)
   * @return 64-bit integer value
   * @throws std::runtime_error if VarLong is too long or malformed
   */
  std::int64_t read_varlong();

  /**
   * @brief Read string with VarInt length prefix
   * @return String value
   * @throws std::runtime_error if string length is invalid
   */
  std::string read_string();

  /**
   * @brief Read UUID (128-bit identifier)
   * @return Pair of (most_significant, least_significant) bits
   */
  std::pair<std::uint64_t, std::uint64_t> read_uuid();

  /**
   * @brief Skip bytes without reading them
   * @param count Number of bytes to skip
   */
  void skip_bytes(std::size_t count);

  /**
   * @brief Peek at next byte without advancing read position
   * @return Next byte value
   */
  std::uint8_t peek_byte() const;

  // ============================================================================
  // PACKET-SPECIFIC METHODS FOR SESSION OPTIMIZATION
  // ============================================================================

  /**
   * @brief Check if buffer contains a complete packet
   * @return true if a complete packet is available for processing
   * @details Checks if buffer has at least a VarInt length prefix and
   *          enough data for the complete packet
   */
  bool has_complete_packet() const noexcept;

  /**
   * @brief Peek at packet length without advancing read position
   * @return Optional packet length, empty if incomplete VarInt
   * @details Reads the VarInt length prefix without consuming it
   */
  std::optional<int32_t> peek_packet_length() const noexcept;

  /**
   * @brief Check if buffer has specified number of bytes available
   * @param count Required number of bytes
   * @return true if enough bytes are available
   */
  bool has_bytes_available(std::size_t count) const noexcept {
    return readable_bytes() >= count;
  }

  /**
   * @brief Skip the packet length VarInt at current read position
   * @details Advances read position past the length prefix
   */
  void skip_packet_length();

  /**
   * @brief Get pointer to current read position
   * @return Pointer to current readable data
   * @details Used for zero-copy PacketView creation
   */
  const std::uint8_t* current_read_ptr() const noexcept {
    return data_.data() + read_pos_;
  }

  /**
   * @brief Advance read position by specified count
   * @param count Number of bytes to advance
   * @throws std::runtime_error if count exceeds available data
   */
  void advance_read_position(std::size_t count);

  /**
   * @brief Compact buffer by removing processed data
   * @details Moves unread data to beginning of buffer to reclaim space
   */
  void compact();
};

}  // namespace network
}  // namespace parallelstone