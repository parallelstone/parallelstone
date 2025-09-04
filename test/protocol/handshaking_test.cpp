#include "protocol/handlers/handshaking.hpp"
#include "protocol/dispatcher.hpp"
#include "protocol/buffer.hpp"
#include "server/session.hpp"
#include "server/session_manager.hpp"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>

using namespace protocol;
using namespace parallelstone::server;

class MockSession : public Session {
public:
    MockSession() = default;

    MOCK_METHOD(void, SetNextState, (SessionState state), (override));
    MOCK_METHOD(void, Disconnect, (DisconnectReason reason, const std::string& message), (override));
    MOCK_METHOD(bool, SendRawData, (const void* data, size_t size), (override));
    MOCK_METHOD(const std::string&, GetRemoteAddress, (), (const, override));
    MOCK_METHOD(uint16_t, GetRemotePort, (), (const, override));
    MOCK_METHOD(const std::string&, GetSessionId, (), (const, override));
};

class HandshakingTest : public ::testing::Test {
protected:
    void SetUp() override {
        session = std::make_shared<MockSession>();
        handshaking_handler = &GetHandshakingHandler();
        dispatcher = &GetPacketDispatcher();
    }
    
    std::shared_ptr<MockSession> session;
    HandshakingHandler* handshaking_handler;
    PacketDispatcher* dispatcher;
};

TEST_F(HandshakingTest, ValidHandshakeForStatus) {
    Buffer handshake_buffer;
    handshake_buffer.write_varint(772);  // Protocol version (1.21.7)
    handshake_buffer.write_string("localhost");  // Server address
    handshake_buffer.write_uint16(25565);  // Server port
    handshake_buffer.write_varint(1);  // Next state: STATUS
    
    EXPECT_CALL(*session, SetNextState(SessionState::STATUS)).Times(1);
    
    bool result = handshaking_handler->HandleHandshake(session, handshake_buffer);
    EXPECT_TRUE(result) << "Handshake for status should succeed";
}

TEST_F(HandshakingTest, ValidHandshakeForLogin) {
    Buffer login_buffer;
    login_buffer.write_varint(772);  // Protocol version
    login_buffer.write_string("play.example.com");  // Server address
    login_buffer.write_uint16(25565);  // Server port
    login_buffer.write_varint(2);  // Next state: LOGIN
    
    EXPECT_CALL(*session, SetNextState(SessionState::LOGIN)).Times(1);
    
    bool result = handshaking_handler->HandleHandshake(session, login_buffer);
    EXPECT_TRUE(result) << "Handshake for login should succeed";
}

TEST_F(HandshakingTest, ProtocolVersionMismatch) {
    Buffer mismatch_buffer;
    mismatch_buffer.write_varint(999);  // Wrong protocol version
    mismatch_buffer.write_string("localhost");
    mismatch_buffer.write_uint16(25565);
    mismatch_buffer.write_varint(1);
    
    EXPECT_CALL(*session, Disconnect(testing::_, testing::_)).Times(1);
    
    bool result = handshaking_handler->HandleHandshake(session, mismatch_buffer);
    EXPECT_FALSE(result) << "Handshake with wrong protocol version should fail";
}

TEST_F(HandshakingTest, LegacyServerListPing) {
    Buffer legacy_buffer;
    legacy_buffer.write_byte(0x01);  // Some legacy data
    
    EXPECT_CALL(*session, SendRawData(testing::_, testing::_)).Times(1);
    
    bool result = handshaking_handler->HandleLegacyServerListPing(session, legacy_buffer);
    EXPECT_TRUE(result) << "Legacy server list ping should succeed";
}

TEST_F(HandshakingTest, DispatcherIntegration) {
    Buffer dispatcher_buffer;
    dispatcher_buffer.write_varint(772);
    dispatcher_buffer.write_string("test.server.com");
    dispatcher_buffer.write_uint16(25565);
    dispatcher_buffer.write_varint(1);
    
    EXPECT_CALL(*session, SetNextState(SessionState::STATUS)).Times(1);
    
    bool result = dispatcher->DispatchPacket(SessionState::HANDSHAKING, 0x00, session, dispatcher_buffer);
    EXPECT_TRUE(result) << "Dispatcher integration should work";
}