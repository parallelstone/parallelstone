#include "network/buffer.hpp"
#include <gtest/gtest.h>
#include <vector>

using namespace network;

class BufferTest : public ::testing::Test {
protected:
    void SetUp() override {
        buffer.clear();
    }
    
    Buffer buffer;
};

TEST_F(BufferTest, VarIntOperations) {
    std::vector<std::int32_t> test_values = {
        0, 127, 128, 16383, 16384, 2097151, 2097152, -1, 
        static_cast<std::int32_t>(-2147483648LL)
    };
    
    for (auto value : test_values) {
        buffer.clear();
        buffer.write_varint(value);
        buffer.reset_read_position();
        
        std::int32_t read_value = buffer.read_varint();
        EXPECT_EQ(read_value, value) << "VarInt test failed for value: " << value;
    }
}

TEST_F(BufferTest, StringOperations) {
    std::vector<std::string> test_strings = {
        "",
        "hello",
        "minecraft",
        "localhost",
        "play.hypixel.net"
    };
    
    for (const auto& str : test_strings) {
        buffer.clear();
        buffer.write_string(str);
        buffer.reset_read_position();
        
        std::string read_str = buffer.read_string();
        EXPECT_EQ(read_str, str) << "String test failed for: '" << str << "'";
    }
}

TEST_F(BufferTest, UInt16Operations) {
    std::vector<std::uint16_t> test_ports = {0, 25565, 19132, 65535};
    
    for (auto port : test_ports) {
        buffer.clear();
        buffer.write_uint16(port);
        buffer.reset_read_position();
        
        std::uint16_t read_port = buffer.read_uint16();
        EXPECT_EQ(read_port, port) << "UInt16 test failed for port: " << port;
    }
}

TEST_F(BufferTest, MixedOperations) {
    buffer.clear();
    buffer.write_varint(772);
    buffer.write_string("localhost");
    buffer.write_uint16(25565);
    buffer.write_varint(1);
    
    buffer.reset_read_position();
    std::int32_t protocol = buffer.read_varint();
    std::string address = buffer.read_string();
    std::uint16_t port = buffer.read_uint16();
    std::int32_t next_state = buffer.read_varint();
    
    EXPECT_EQ(protocol, 772);
    EXPECT_EQ(address, "localhost");
    EXPECT_EQ(port, 25565);
    EXPECT_EQ(next_state, 1);
}