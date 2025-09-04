#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <stdexcept>
#include <utility>
#include <vector>
#include <memory>
#include <cassert>

namespace parallelstone {
namespace network {

/**
 * @brief A read-only, non-owning view over a packet's data.
 *
 * This class provides a safe and efficient way to parse packet data without
 * copying it. It's designed to be created from a segment of a larger
 * receive buffer and passed to packet handlers.
 *
 * LIFETIME SAFETY:
 * - PacketView does NOT own the data it points to
 * - The underlying buffer must remain valid for the entire lifetime of
 * PacketView
 * - Typically used in a single function call scope (handler methods)
 * - Should not be stored or passed to async operations
 */
class PacketView {
 private:
  const std::uint8_t* data_ptr_;
  const std::size_t size_;
  mutable std::size_t read_pos_;

  /**
   * @brief Checks if enough bytes are available for reading
   * @param count Number of bytes required
   * @throws std::runtime_error if not enough bytes available
   * @details Enhanced bounds checking with overflow protection
   */
  void check_read_bounds(std::size_t count) const;

  /**
   * @brief Validate that read position is within bounds (debug helper)
   * @details Additional safety check for debug builds
   */
  void validate_state() const noexcept;

 public:
  /**
   * @brief Constructor
   * @param data Pointer to the start of the packet data (must remain valid!)
   * @param size The size of the packet data
   */
  PacketView(const std::uint8_t* data, std::size_t size) noexcept
      : data_ptr_(data), size_(size), read_pos_(0) {
    assert(data != nullptr || size == 0);
  }

  /**
   * @brief Constructor from buffer with offset and length
   * @param buffer Source buffer
   * @param offset Offset into the buffer
   * @param length Length of the view
   */
  PacketView(const std::vector<std::uint8_t>& buffer, std::size_t offset = 0,
             std::size_t length = SIZE_MAX)
      : data_ptr_(buffer.data() + offset),
        size_(length == SIZE_MAX ? buffer.size() - offset : length),
        read_pos_(0) {
    assert(offset <= buffer.size());
    assert(offset + size_ <= buffer.size());
  }

  // Non-copyable to prevent accidental copies
  PacketView(const PacketView&) = delete;
  PacketView& operator=(const PacketView&) = delete;

  // Movable for efficiency
  PacketView(PacketView&& other) noexcept
      : data_ptr_(other.data_ptr_),
        size_(other.size_),
        read_pos_(other.read_pos_) {
    const_cast<const std::uint8_t*&>(other.data_ptr_) = nullptr;
    const_cast<std::size_t&>(other.size_) = 0;
    other.read_pos_ = 0;
  }

  PacketView& operator=(PacketView&& other) noexcept {
    if (this != &other) {
      const_cast<const std::uint8_t*&>(data_ptr_) = other.data_ptr_;
      const_cast<std::size_t&>(size_) = other.size_;
      read_pos_ = other.read_pos_;

      const_cast<const std::uint8_t*&>(other.data_ptr_) = nullptr;
      const_cast<std::size_t&>(other.size_) = 0;
      other.read_pos_ = 0;
    }
    return *this;
  }

  ~PacketView() = default;

  /**
   * @brief Check if the view is valid (has non-null data)
   */
  bool is_valid() const noexcept { return data_ptr_ != nullptr; }

  /**
   * @brief Get the number of bytes available for reading
   */
  std::size_t readable_bytes() const noexcept { return size_ - read_pos_; }

  /**
   * @brief Get pointer to current read position
   */
  const std::uint8_t* data() const noexcept { return data_ptr_ + read_pos_; }

  /**
   * @brief Get total size of the view
   */
  std::size_t size() const noexcept { return size_; }

  /**
   * @brief Get current read position
   */
  std::size_t read_position() const noexcept { return read_pos_; }

  /**
   * @brief Reset read position to beginning
   */
  void reset() noexcept { read_pos_ = 0; }

  // Reading methods - all throw std::runtime_error on bounds violation
  void skip_bytes(std::size_t count);
  void read_bytes(void* data, std::size_t size);

  std::uint8_t read_byte();
  bool read_bool();
  std::int8_t read_int8();
  std::uint16_t read_uint16();
  std::int16_t read_int16();
  std::uint32_t read_uint32();
  std::int32_t read_int32();
  std::uint64_t read_uint64();
  std::int64_t read_int64();
  float read_float();
  double read_double();
  std::int32_t read_varint();
  std::int64_t read_varlong();
  std::string read_string();
  std::pair<std::uint64_t, std::uint64_t> read_uuid();

  /**
   * @brief Peek at a byte without advancing read position
   */
  std::uint8_t peek_byte() const {
    check_read_bounds(1);
    return data_ptr_[read_pos_];
  }

  /**
   * @brief Create a sub-view of the remaining data
   * @param length Length of the sub-view (or remaining bytes if SIZE_MAX)
   */
  PacketView subview(std::size_t length = SIZE_MAX) const {
    std::size_t remaining = readable_bytes();
    std::size_t actual_length =
        (length == SIZE_MAX) ? remaining : std::min(length, remaining);
    return PacketView(data_ptr_ + read_pos_, actual_length);
  }
};

/**
 * @brief RAII wrapper for safe PacketView usage with owned data
 *
 * This class ensures that the underlying data remains valid
 * for the lifetime of the PacketView.
 */
class SafePacketView {
 private:
  std::vector<std::uint8_t> owned_data_;
  PacketView view_;

 public:
  explicit SafePacketView(std::vector<std::uint8_t> data)
      : owned_data_(std::move(data)),
        view_(owned_data_.data(), owned_data_.size()) {}

  // Non-copyable but movable
  SafePacketView(const SafePacketView&) = delete;
  SafePacketView& operator=(const SafePacketView&) = delete;
  SafePacketView(SafePacketView&&) = default;
  SafePacketView& operator=(SafePacketView&&) = default;

  PacketView& view() noexcept { return view_; }
  const PacketView& view() const noexcept { return view_; }

  // Implicit conversion for convenience
  operator PacketView&() noexcept { return view_; }
  operator const PacketView&() const noexcept { return view_; }
};

}  // namespace network
}  // namespace parallelstone