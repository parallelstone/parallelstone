#include "network/packet_view.hpp"
#include <stdexcept>
#include <bit>
#include <type_traits>

// Remove platform-specific headers - no longer needed for byte order operations
// All endianness handling is now done with C++20 std::endian and templates

namespace parallelstone {
namespace network {

void PacketView::check_read_bounds(std::size_t count) const {
    // Enhanced bounds checking with overflow protection
    if (count > size_) {
        throw std::runtime_error("PacketView: requested read size exceeds total buffer size");
    }
    
    if (read_pos_ > size_) {
        throw std::runtime_error("PacketView: internal state corrupted - read position beyond buffer");
    }
    
    // Check for arithmetic overflow in addition
    if (count > SIZE_MAX - read_pos_) {
        throw std::runtime_error("PacketView: arithmetic overflow in bounds check");
    }
    
    if (read_pos_ + count > size_) {
        throw std::runtime_error("PacketView: read out of bounds (requested: " + 
                                std::to_string(count) + " bytes, available: " + 
                                std::to_string(size_ - read_pos_) + " bytes)");
    }
    
    // Additional debug validation
    validate_state();
}

void PacketView::validate_state() const noexcept {
    // Debug-only validation (no-op in release builds unless explicitly enabled)
    #ifndef NDEBUG
        assert(data_ptr_ != nullptr || size_ == 0);
        assert(read_pos_ <= size_);
    #endif
}

void PacketView::skip_bytes(std::size_t count) {
    check_read_bounds(count);
    read_pos_ += count;
}

void PacketView::read_bytes(void* data, std::size_t size) {
    check_read_bounds(size);
    
    // Safe byte-by-byte copy without memcpy or std::copy_n
    // This ensures no alignment issues and explicit memory access control
    auto* dest = static_cast<std::uint8_t*>(data);
    for (std::size_t i = 0; i < size; ++i) {
        dest[i] = data_ptr_[read_pos_ + i];
    }
    
    read_pos_ += size;
}

std::uint8_t PacketView::read_byte() {
    check_read_bounds(1);
    return data_ptr_[read_pos_++];
}

bool PacketView::read_bool() {
    return read_byte() != 0;
}

std::int8_t PacketView::read_int8() {
    return static_cast<std::int8_t>(read_byte());
}

namespace {
    /**
     * @brief Safely read integer from big-endian byte stream
     * @tparam T Integral type to read (uint16_t, uint32_t, uint64_t, etc.)
     * @param ptr Pointer to byte data (must have at least sizeof(T) bytes)
     * @return Value in native byte order
     * @details This function reads bytes sequentially without memcpy, alignment issues, or UB.
     *          It's constexpr-compatible and works on any platform regardless of endianness.
     */
    template<typename T>
    constexpr T read_big_endian_integer(const std::uint8_t* ptr) noexcept {
        static_assert(std::is_integral_v<T>, "T must be an integral type");
        static_assert(std::is_unsigned_v<T>, "T should be unsigned for bit operations");
        static_assert(sizeof(T) <= 8, "Maximum 64-bit integers supported");
        
        T value = 0;
        
        // Read bytes in big-endian order (most significant byte first)
        // This loop will be optimized by the compiler for known sizes
        for (std::size_t i = 0; i < sizeof(T); ++i) {
            value = (value << 8) | static_cast<T>(ptr[i]);
        }
        
        return value;
    }
    
    /**
     * @brief Validate that a type is safe for bit operations
     * @tparam T Type to validate
     */
    template<typename T>
    constexpr bool is_safe_for_bit_cast() noexcept {
        return std::is_trivially_copyable_v<T> && 
               std::is_standard_layout_v<T> &&
               !std::is_pointer_v<T>;
    }
}

std::uint16_t PacketView::read_uint16() {
    check_read_bounds(2);
    std::uint16_t value = read_big_endian_integer<std::uint16_t>(data_ptr_ + read_pos_);
    read_pos_ += 2;
    return value;
}

std::int16_t PacketView::read_int16() {
    return static_cast<std::int16_t>(read_uint16());
}

std::uint32_t PacketView::read_uint32() {
    check_read_bounds(4);
    std::uint32_t value = read_big_endian_integer<std::uint32_t>(data_ptr_ + read_pos_);
    read_pos_ += 4;
    return value;
}

std::int32_t PacketView::read_int32() {
    return static_cast<std::int32_t>(read_uint32());
}

std::uint64_t PacketView::read_uint64() {
    check_read_bounds(8);
    std::uint64_t value = read_big_endian_integer<std::uint64_t>(data_ptr_ + read_pos_);
    read_pos_ += 8;
    return value;
}

std::int64_t PacketView::read_int64() {
    return static_cast<std::int64_t>(read_uint64());
}

float PacketView::read_float() {
    // Compile-time safety validation
    static_assert(sizeof(float) == 4, "float must be 32-bit for Minecraft protocol compatibility");
    static_assert(sizeof(std::uint32_t) == 4, "uint32_t must be 32-bit");
    static_assert(is_safe_for_bit_cast<float>(), "float must be safe for bit_cast operations");
    
    // read_uint32() already handles endianness conversion from big-endian to native
    std::uint32_t int_value = read_uint32();
    
    // C++20 std::bit_cast provides safe, well-defined type punning
    // No undefined behavior, no alignment issues, no platform dependencies
    return std::bit_cast<float>(int_value);
}

double PacketView::read_double() {
    // Compile-time safety validation
    static_assert(sizeof(double) == 8, "double must be 64-bit for Minecraft protocol compatibility");
    static_assert(sizeof(std::uint64_t) == 8, "uint64_t must be 64-bit");
    static_assert(is_safe_for_bit_cast<double>(), "double must be safe for bit_cast operations");
    
    // read_uint64() already handles endianness conversion from big-endian to native
    std::uint64_t int_value = read_uint64();
    
    // C++20 std::bit_cast provides safe, well-defined type punning
    // No undefined behavior, no alignment issues, no platform dependencies
    return std::bit_cast<double>(int_value);
}

std::int32_t PacketView::read_varint() {
    std::int32_t value = 0;
    std::size_t position = 0;
    std::uint8_t current_byte;

    do {
        if (position >= 32) {
            throw std::runtime_error("VarInt is too big");
        }
        
        current_byte = read_byte();
        value |= static_cast<std::int32_t>(current_byte & 0x7F) << position;
        position += 7;
    } while ((current_byte & 0x80) != 0);

    return value;
}

std::int64_t PacketView::read_varlong() {
    std::int64_t value = 0;
    std::size_t position = 0;
    std::uint8_t current_byte;

    do {
        if (position >= 64) {
            throw std::runtime_error("VarLong is too big");
        }
        
        current_byte = read_byte();
        value |= static_cast<std::int64_t>(current_byte & 0x7F) << position;
        position += 7;
    } while ((current_byte & 0x80) != 0);

    return value;
}

std::string PacketView::read_string() {
    std::int32_t length = read_varint();
    if (length < 0) {
        throw std::runtime_error("Invalid string length: negative value");
    }
    
    // Check for reasonable string length (Minecraft limit is 32767)
    constexpr std::int32_t MAX_STRING_LENGTH = 32767;
    if (length > MAX_STRING_LENGTH) {
        throw std::runtime_error("String length exceeds maximum allowed size");
    }
    
    check_read_bounds(static_cast<std::size_t>(length));
    
    // Create string directly from bytes without intermediate copy
    std::string value;
    value.reserve(static_cast<std::size_t>(length));
    
    for (std::int32_t i = 0; i < length; ++i) {
        value.push_back(static_cast<char>(data_ptr_[read_pos_ + i]));
    }
    
    read_pos_ += static_cast<std::size_t>(length);
    return value;
}

std::pair<std::uint64_t, std::uint64_t> PacketView::read_uuid() {
    std::uint64_t most = read_uint64();
    std::uint64_t least = read_uint64();
    return {most, least};
}

} // namespace network
} // namespace parallelstone
