#include <gtest/gtest.h>
#include "network/buffer.hpp"
#include <vector>

using namespace parallelstone::network;

class BufferOptimizationTest : public ::testing::Test {
protected:
    Buffer buffer;
    
    void SetUp() override {
        buffer = Buffer(1024); // 1KB initial capacity
    }
};

TEST_F(BufferOptimizationTest, TestPacketLengthPeekAndSkip) {
    // Write a test packet: length=5, id=0x10, data="test"
    buffer.write_varint(5); // packet length
    buffer.write_varint(0x10); // packet id
    buffer.write_string("test"); // payload
    
    // Reset read position to beginning
    buffer.set_read_position(0);
    
    ASSERT_TRUE(buffer.has_complete_packet()) << "Buffer should have complete packet";
    
    // Peek at packet length without consuming it
    auto length = buffer.peek_packet_length();
    ASSERT_TRUE(length.has_value()) << "Should be able to peek at packet length";
    EXPECT_EQ(length.value(), 5) << "Packet length should be 5";
    
    // Read position should not have changed
    EXPECT_EQ(buffer.read_position(), 0) << "Read position should not change after peek";
    
    // Skip the packet length
    buffer.skip_packet_length();
    
    // Now we should be at the packet ID
    EXPECT_EQ(buffer.read_varint(), 0x10) << "Packet ID should be 0x10";
}

TEST_F(BufferOptimizationTest, TestIncompletePacket) {
    // Write incomplete packet: length=10 but only provide 3 bytes
    buffer.write_varint(10); // declare length of 10
    buffer.write_bytes("abc", 3); // but only write 3 bytes
    
    buffer.set_read_position(0);
    
    EXPECT_FALSE(buffer.has_complete_packet()) << "Buffer should not have complete packet";
    
    auto length = buffer.peek_packet_length();
    ASSERT_TRUE(length.has_value()) << "Should be able to peek at packet length";
    EXPECT_EQ(length.value(), 10) << "Packet length should be 10";
}

TEST_F(BufferOptimizationTest, TestZeroCopyPacketProcessing) {
    // Simulate session packet processing pattern
    
    // Write multiple packets
    buffer.write_varint(4); // packet 1: length=4
    buffer.write_varint(0x01); // id=1
    buffer.write_bytes("hi", 2); // data="hi"
    buffer.write_byte(0x00); // padding
    
    buffer.write_varint(6); // packet 2: length=6  
    buffer.write_varint(0x02); // id=2
    buffer.write_bytes("test", 4); // data="test"
    buffer.write_byte(0x00); // padding
    
    buffer.set_read_position(0);
    
    // Process first packet
    ASSERT_TRUE(buffer.has_complete_packet());
    auto length1 = buffer.peek_packet_length();
    ASSERT_TRUE(length1.has_value());
    EXPECT_EQ(length1.value(), 4);
    
    // Skip length and get packet data pointer (zero-copy)
    buffer.skip_packet_length();
    const uint8_t* packet1_data = buffer.current_read_ptr();
    
    // Advance past first packet
    buffer.advance_read_position(length1.value());
    
    // Process second packet
    ASSERT_TRUE(buffer.has_complete_packet());
    auto length2 = buffer.peek_packet_length();
    ASSERT_TRUE(length2.has_value());
    EXPECT_EQ(length2.value(), 6);
    
    buffer.skip_packet_length();
    const uint8_t* packet2_data = buffer.current_read_ptr();
    
    // Verify we can read the packet data directly
    EXPECT_EQ(packet1_data[0], 0x01) << "First packet ID should be 0x01";
    EXPECT_EQ(packet2_data[0], 0x02) << "Second packet ID should be 0x02";
}

TEST_F(BufferOptimizationTest, TestBufferCompaction) {
    // Fill buffer with test data
    buffer.write_bytes("0123456789", 10);
    buffer.set_read_position(0);
    
    // Read first 4 bytes
    std::vector<uint8_t> read_data(4);
    buffer.read_bytes(read_data.data(), 4);
    
    EXPECT_EQ(buffer.read_position(), 4);
    EXPECT_EQ(buffer.readable_bytes(), 6);
    
    // Compact buffer
    buffer.compact();
    
    EXPECT_EQ(buffer.read_position(), 0) << "Read position should reset to 0 after compact";
    EXPECT_EQ(buffer.readable_bytes(), 6) << "Should still have 6 bytes readable";
    
    // Verify data is correct after compaction
    std::vector<uint8_t> remaining(6);
    buffer.read_bytes(remaining.data(), 6);
    EXPECT_EQ(std::string(remaining.begin(), remaining.end()), "456789") << "Remaining data should be '456789'";
}

TEST_F(BufferOptimizationTest, TestLargeVarIntHandling) {
    // Test with large VarInt (max allowed)
    int32_t large_value = 0x7FFFFFFF; // Max positive int32
    buffer.write_varint(large_value);
    buffer.set_read_position(0);
    
    auto peeked = buffer.peek_packet_length();
    ASSERT_TRUE(peeked.has_value());
    EXPECT_EQ(peeked.value(), large_value);
    
    // Read position should not change
    EXPECT_EQ(buffer.read_position(), 0);
    
    // Now actually read it
    int32_t read_value = buffer.read_varint();
    EXPECT_EQ(read_value, large_value);
}

TEST_F(BufferOptimizationTest, TestErrorConditions) {
    // Test empty buffer
    EXPECT_FALSE(buffer.has_complete_packet());
    EXPECT_FALSE(buffer.peek_packet_length().has_value());
    
    // Test buffer with only partial VarInt
    buffer.write_byte(0x80); // VarInt continuation byte without ending
    buffer.set_read_position(0);
    
    EXPECT_FALSE(buffer.has_complete_packet());
    EXPECT_FALSE(buffer.peek_packet_length().has_value());
}