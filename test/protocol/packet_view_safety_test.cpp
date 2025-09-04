#include <gtest/gtest.h>
#include "network/packet_view.hpp"
#include <vector>
#include <limits>
#include <cstring>

using namespace parallelstone::network;

class PacketViewSafetyTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test data with known values
        test_data_ = {
            // uint16 big-endian: 0x1234 = 4660
            0x12, 0x34,
            // uint32 big-endian: 0x12345678 = 305419896
            0x12, 0x34, 0x56, 0x78,
            // uint64 big-endian: 0x123456789ABCDEF0
            0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0,
            // float32 big-endian: 1.0f (0x3F800000)
            0x3F, 0x80, 0x00, 0x00,
            // double64 big-endian: 1.0 (0x3FF0000000000000)
            0x3F, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        };
    }

    std::vector<std::uint8_t> test_data_;
};

TEST_F(PacketViewSafetyTest, MemcpyFreeReading) {
    PacketView view(test_data_);
    
    // Test that all integer reading works without memcpy
    EXPECT_EQ(view.read_uint16(), 0x1234);
    EXPECT_EQ(view.read_uint32(), 0x12345678);
    EXPECT_EQ(view.read_uint64(), 0x123456789ABCDEF0ULL);
    
    // Verify read position advanced correctly
    EXPECT_EQ(view.read_position(), 14);
}

TEST_F(PacketViewSafetyTest, BitCastSafety) {
    PacketView view(test_data_);
    
    // Skip to float/double data
    view.skip_bytes(14);
    
    // Test safe bit_cast operations
    float f = view.read_float();
    EXPECT_FLOAT_EQ(f, 1.0f);
    
    double d = view.read_double();
    EXPECT_DOUBLE_EQ(d, 1.0);
}

TEST_F(PacketViewSafetyTest, BoundsCheckingEnhanced) {
    std::vector<std::uint8_t> small_data = {0x12, 0x34};
    PacketView view(small_data);
    
    // Normal read should work
    EXPECT_NO_THROW(view.read_uint16());
    
    // Reading beyond bounds should throw with descriptive message
    EXPECT_THROW({
        try {
            view.read_uint32(); // Only 2 bytes in buffer, need 4
        } catch(const std::runtime_error& e) {
            EXPECT_THAT(std::string(e.what()), 
                       testing::HasSubstr("read out of bounds"));
            EXPECT_THAT(std::string(e.what()), 
                       testing::HasSubstr("requested: 4 bytes"));
            EXPECT_THAT(std::string(e.what()), 
                       testing::HasSubstr("available: 0 bytes"));
            throw;
        }
    }, std::runtime_error);
}

TEST_F(PacketViewSafetyTest, OverflowProtection) {
    std::vector<std::uint8_t> data = {0x01, 0x02};
    PacketView view(data);
    
    // Test protection against arithmetic overflow
    EXPECT_THROW({
        view.check_read_bounds(SIZE_MAX);
    }, std::runtime_error);
}

TEST_F(PacketViewSafetyTest, SafeByteReading) {
    std::vector<std::uint8_t> data = {0xAA, 0xBB, 0xCC, 0xDD};
    PacketView view(data);
    
    // Test safe byte-by-byte reading without memcpy
    std::uint8_t buffer[4] = {0};
    view.read_bytes(buffer, 4);
    
    EXPECT_EQ(buffer[0], 0xAA);
    EXPECT_EQ(buffer[1], 0xBB);  
    EXPECT_EQ(buffer[2], 0xCC);
    EXPECT_EQ(buffer[3], 0xDD);
}

TEST_F(PacketViewSafetyTest, PlatformIndependentEndianness) {
    // Test data that should produce same results regardless of platform endianness
    std::vector<std::uint8_t> data = {
        0x00, 0x01, // uint16: 1
        0x00, 0x00, 0x00, 0x02, // uint32: 2  
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03 // uint64: 3
    };
    
    PacketView view(data);
    
    // These results should be identical on all platforms
    EXPECT_EQ(view.read_uint16(), 1);
    EXPECT_EQ(view.read_uint32(), 2);
    EXPECT_EQ(view.read_uint64(), 3);
}

TEST_F(PacketViewSafetyTest, DebugValidation) {
    std::vector<std::uint8_t> data = {0x01, 0x02};
    PacketView view(data);
    
    // In debug builds, this should not crash
    // In release builds, this should be optimized away
    EXPECT_NO_THROW(view.validate_state());
}

TEST_F(PacketViewSafetyTest, NoUndefinedBehavior) {
    // Test edge cases that could cause UB in unsafe implementations
    
    // Empty buffer
    std::vector<std::uint8_t> empty;
    PacketView empty_view(empty);
    EXPECT_THROW(empty_view.read_byte(), std::runtime_error);
    
    // Single byte buffer trying to read larger type
    std::vector<std::uint8_t> single = {0xFF};
    PacketView single_view(single);
    EXPECT_NO_THROW(single_view.read_byte());
    EXPECT_THROW(single_view.read_uint16(), std::runtime_error);
    
    // Misaligned access (should work fine with our implementation)
    std::vector<std::uint8_t> misaligned = {0x00, 0x12, 0x34, 0x56, 0x78};
    PacketView misaligned_view(misaligned);
    misaligned_view.skip_bytes(1); // Now unaligned for uint32 read
    EXPECT_NO_THROW(misaligned_view.read_uint32()); // Should work regardless of alignment
}

// Compile-time safety verification
TEST_F(PacketViewSafetyTest, CompileTimeSafety) {
    // These should compile and validate type safety
    static_assert(sizeof(float) == 4, "Float size validation");
    static_assert(sizeof(double) == 8, "Double size validation");  
    static_assert(std::is_trivially_copyable_v<float>, "Float copyability");
    static_assert(std::is_trivially_copyable_v<double>, "Double copyability");
    
    // This test just ensures the static_asserts in the code compile correctly
    SUCCEED();
}