#include "network/buffer.hpp"
#include <cstring>
#include <algorithm>
#include <stdexcept>

namespace parallelstone {
namespace network {

// ============================================================================
// CONSTRUCTOR IMPLEMENTATIONS
// ============================================================================

Buffer::Buffer(std::size_t initial_capacity) 
    : data_(initial_capacity), read_pos_(0), write_pos_(0) {}

Buffer::Buffer(const std::uint8_t* data, std::size_t size) 
    : data_(data, data + size), read_pos_(0), write_pos_(size) {}

Buffer::Buffer(const std::vector<std::uint8_t>& data) 
    : data_(data), read_pos_(0), write_pos_(data.size()) {}

// ============================================================================
// PRIVATE HELPER METHODS
// ============================================================================

void Buffer::ensure_capacity(std::size_t size) {
    if (write_pos_ + size > data_.size()) {
        data_.resize(write_pos_ + size);
    }
}

void Buffer::check_read_bounds(std::size_t size) const {
    if (read_pos_ + size > write_pos_) {
        throw std::runtime_error("Buffer underrun: not enough data to read");
    }
}

// ============================================================================
// BUFFER MANAGEMENT METHODS
// ============================================================================

void Buffer::set_read_position(std::size_t pos) {
    if (pos > write_pos_) {
        throw std::runtime_error("Read position beyond write position");
    }
    read_pos_ = pos;
}

void Buffer::reserve(std::size_t capacity) {
    if (capacity > data_.size()) {
        data_.resize(capacity);
    }
}

// ============================================================================
// WRITING METHODS
// ============================================================================

void Buffer::write_bytes(const void* data, std::size_t size) {
    ensure_capacity(size);
    std::memcpy(data_.data() + write_pos_, data, size);
    write_pos_ += size;
}

void Buffer::write_byte(std::uint8_t value) {
    write_bytes(&value, 1);
}

void Buffer::write_bool(bool value) {
    write_byte(value ? 1 : 0);
}

void Buffer::write_int8(std::int8_t value) {
    write_bytes(&value, 1);
}

void Buffer::write_uint16(std::uint16_t value) {
    // Convert to big-endian
    std::uint8_t bytes[2] = {
        static_cast<std::uint8_t>(value >> 8),
        static_cast<std::uint8_t>(value & 0xFF)
    };
    write_bytes(bytes, 2);
}

void Buffer::write_int16(std::int16_t value) {
    write_uint16(static_cast<std::uint16_t>(value));
}

void Buffer::write_uint32(std::uint32_t value) {
    // Convert to big-endian
    std::uint8_t bytes[4] = {
        static_cast<std::uint8_t>(value >> 24),
        static_cast<std::uint8_t>(value >> 16),
        static_cast<std::uint8_t>(value >> 8),
        static_cast<std::uint8_t>(value & 0xFF)
    };
    write_bytes(bytes, 4);
}

void Buffer::write_int32(std::int32_t value) {
    write_uint32(static_cast<std::uint32_t>(value));
}

void Buffer::write_uint64(std::uint64_t value) {
    // Convert to big-endian
    std::uint8_t bytes[8];
    for (int i = 7; i >= 0; --i) {
        bytes[7-i] = static_cast<std::uint8_t>(value >> (i * 8));
    }
    write_bytes(bytes, 8);
}

void Buffer::write_int64(std::int64_t value) {
    write_uint64(static_cast<std::uint64_t>(value));
}

void Buffer::write_float(float value) {
    static_assert(sizeof(float) == 4, "float must be 32-bit");
    // Use bit_cast equivalent for float to uint32
    std::uint32_t int_value;
    std::memcpy(&int_value, &value, sizeof(float));
    write_uint32(int_value);
}

void Buffer::write_double(double value) {
    static_assert(sizeof(double) == 8, "double must be 64-bit");
    // Use bit_cast equivalent for double to uint64
    std::uint64_t int_value;
    std::memcpy(&int_value, &value, sizeof(double));
    write_uint64(int_value);
}

void Buffer::write_varint(std::int32_t value) {
    std::uint32_t unsigned_value = static_cast<std::uint32_t>(value);
    while (unsigned_value >= 0x80) {
        write_byte(static_cast<std::uint8_t>(unsigned_value | 0x80));
        unsigned_value >>= 7;
    }
    write_byte(static_cast<std::uint8_t>(unsigned_value));
}

void Buffer::write_varlong(std::int64_t value) {
    std::uint64_t unsigned_value = static_cast<std::uint64_t>(value);
    while (unsigned_value >= 0x80) {
        write_byte(static_cast<std::uint8_t>(unsigned_value | 0x80));
        unsigned_value >>= 7;
    }
    write_byte(static_cast<std::uint8_t>(unsigned_value));
}

void Buffer::write_string(const std::string& value) {
    write_varint(static_cast<std::int32_t>(value.length()));
    write_bytes(value.data(), value.length());
}

void Buffer::write_string(const char* value) {
    write_string(std::string(value));
}

void Buffer::write_uuid(std::uint64_t most_significant, std::uint64_t least_significant) {
    write_uint64(most_significant);
    write_uint64(least_significant);
}

// ============================================================================
// READING METHODS
// ============================================================================

void Buffer::read_bytes(void* data, std::size_t size) {
    check_read_bounds(size);
    std::memcpy(data, data_.data() + read_pos_, size);
    read_pos_ += size;
}

std::uint8_t Buffer::read_byte() {
    check_read_bounds(1);
    return data_[read_pos_++];
}

bool Buffer::read_bool() {
    return read_byte() != 0;
}

std::int8_t Buffer::read_int8() {
    return static_cast<std::int8_t>(read_byte());
}

std::uint16_t Buffer::read_uint16() {
    check_read_bounds(2);
    std::uint16_t value = (static_cast<std::uint16_t>(data_[read_pos_]) << 8) |
                          static_cast<std::uint16_t>(data_[read_pos_ + 1]);
    read_pos_ += 2;
    return value;
}

std::int16_t Buffer::read_int16() {
    return static_cast<std::int16_t>(read_uint16());
}

std::uint32_t Buffer::read_uint32() {
    check_read_bounds(4);
    std::uint32_t value = (static_cast<std::uint32_t>(data_[read_pos_]) << 24) |
                          (static_cast<std::uint32_t>(data_[read_pos_ + 1]) << 16) |
                          (static_cast<std::uint32_t>(data_[read_pos_ + 2]) << 8) |
                          static_cast<std::uint32_t>(data_[read_pos_ + 3]);
    read_pos_ += 4;
    return value;
}

std::int32_t Buffer::read_int32() {
    return static_cast<std::int32_t>(read_uint32());
}

std::uint64_t Buffer::read_uint64() {
    check_read_bounds(8);
    std::uint64_t value = 0;
    for (int i = 0; i < 8; ++i) {
        value = (value << 8) | static_cast<std::uint64_t>(data_[read_pos_ + i]);
    }
    read_pos_ += 8;
    return value;
}

std::int64_t Buffer::read_int64() {
    return static_cast<std::int64_t>(read_uint64());
}

float Buffer::read_float() {
    std::uint32_t int_value = read_uint32();
    float float_value;
    std::memcpy(&float_value, &int_value, sizeof(float));
    return float_value;
}

double Buffer::read_double() {
    std::uint64_t int_value = read_uint64();
    double double_value;
    std::memcpy(&double_value, &int_value, sizeof(double));
    return double_value;
}

std::int32_t Buffer::read_varint() {
    std::int32_t result = 0;
    int shift = 0;
    std::uint8_t byte;
    
    do {
        if (shift >= 32) {
            throw std::runtime_error("VarInt is too long");
        }
        
        byte = read_byte();
        result |= static_cast<std::int32_t>(byte & 0x7F) << shift;
        shift += 7;
    } while (byte & 0x80);
    
    return result;
}

std::int64_t Buffer::read_varlong() {
    std::int64_t result = 0;
    int shift = 0;
    std::uint8_t byte;
    
    do {
        if (shift >= 64) {
            throw std::runtime_error("VarLong is too long");
        }
        
        byte = read_byte();
        result |= static_cast<std::int64_t>(byte & 0x7F) << shift;
        shift += 7;
    } while (byte & 0x80);
    
    return result;
}

std::string Buffer::read_string() {
    std::int32_t length = read_varint();
    
    if (length < 0) {
        throw std::runtime_error("String length cannot be negative");
    }
    
    if (length == 0) {
        return std::string();
    }
    
    std::string result(length, '\0');
    read_bytes(result.data(), length);
    return result;
}

std::pair<std::uint64_t, std::uint64_t> Buffer::read_uuid() {
    std::uint64_t most = read_uint64();
    std::uint64_t least = read_uint64();
    return std::make_pair(most, least);
}

void Buffer::skip_bytes(std::size_t count) {
    check_read_bounds(count);
    read_pos_ += count;
}

std::uint8_t Buffer::peek_byte() const {
    check_read_bounds(1);
    return data_[read_pos_];
}

// ============================================================================
// PACKET-SPECIFIC METHODS FOR SESSION OPTIMIZATION
// ============================================================================

bool Buffer::has_complete_packet() const noexcept {
    if (readable_bytes() == 0) {
        return false;
    }
    
    // Parse the VarInt length and calculate its size in one pass
    std::size_t temp_read_pos = read_pos_;
    std::int32_t packet_length = 0;
    int shift = 0;
    std::size_t varint_size = 0;
    std::uint8_t byte;
    
    do {
        if (shift >= 32 || temp_read_pos >= write_pos_) {
            return false; // VarInt too long or not enough data
        }
        
        byte = data_[temp_read_pos++];
        varint_size++;
        packet_length |= static_cast<std::int32_t>(byte & 0x7F) << shift;
        shift += 7;
    } while (byte & 0x80);
    
    // Check if we have enough data for the complete packet
    // varint_size = bytes used for length prefix
    // packet_length = bytes needed for packet data
    return readable_bytes() >= (varint_size + static_cast<std::size_t>(packet_length));
}

std::optional<std::int32_t> Buffer::peek_packet_length() const noexcept {
    if (readable_bytes() == 0) {
        return std::nullopt;
    }
    
    std::size_t temp_read_pos = read_pos_;
    std::int32_t result = 0;
    int shift = 0;
    std::uint8_t byte;
    
    try {
        do {
            if (shift >= 32) {
                return std::nullopt; // VarInt too long
            }
            
            if (temp_read_pos >= write_pos_) {
                return std::nullopt; // Not enough data
            }
            
            byte = data_[temp_read_pos++];
            result |= static_cast<std::int32_t>(byte & 0x7F) << shift;
            shift += 7;
        } while (byte & 0x80);
        
        return result;
    } catch (...) {
        return std::nullopt;
    }
}

void Buffer::skip_packet_length() {
    // Read and discard the VarInt length
    std::uint8_t byte;
    do {
        byte = read_byte();
    } while (byte & 0x80);
}

void Buffer::advance_read_position(std::size_t count) {
    if (read_pos_ + count > write_pos_) {
        throw std::runtime_error("Cannot advance read position beyond write position");
    }
    read_pos_ += count;
}

void Buffer::compact() {
    if (read_pos_ == 0) {
        return; // Already compacted
    }
    
    std::size_t remaining = readable_bytes();
    if (remaining > 0) {
        std::memmove(data_.data(), data_.data() + read_pos_, remaining);
    }
    
    read_pos_ = 0;
    write_pos_ = remaining;
}

} // namespace network
} // namespace parallelstone