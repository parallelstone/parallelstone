#pragma once

#include "../packet.hpp"
#include "../protocol_state.hpp"
#include "../data_types.hpp"
#include <chrono>
#include <sstream>

namespace minecraft::protocol::packets::status {

// Serverbound Status Request Packet (0x00)
// 클라이언트가 서버 상태 정보를 요청하는 패킷
class StatusRequestPacket : public PacketBase<ProtocolState::STATUS, PacketDirection::SERVERBOUND> {
public:
    StatusRequestPacket() = default;

    int32_t getPacketId() const override {
        return PacketId::Status::Serverbound::STATUS_REQUEST;
    }

    void serialize(ByteBuffer& buffer) const override {
        // 데이터 없음
    }

    void deserialize(ByteBuffer& buffer) override {
        // 데이터 없음
    }

    std::string toString() const override {
        return "StatusRequestPacket{}";
    }
};

// 서버 상태 정보를 담는 구조체
struct ServerStatusInfo {
    struct Version {
        std::string name;     // "1.20.4"
        int32_t protocol;     // 765
    } version;
    
    struct Players {
        int32_t max;          // 최대 플레이어 수
        int32_t online;       // 현재 온라인 플레이어 수
        
        struct Sample {
            std::string name; // 플레이어 이름
            std::string id;   // 플레이어 UUID
        };
        std::vector<Sample> sample; // 플레이어 샘플 (서버 목록에 표시용)
    } players;
    
    ChatComponent description;        // 서버 설명 (MOTD)
    std::string favicon;              // 서버 아이콘 (base64)
    bool enforcesSecureChat = true;   // 보안 채팅 강제 여부
    bool previewsChat = false;        // 채팅 미리보기 여부
    
    // JSON으로 직렬화
    std::string toJson() const {
        std::ostringstream oss;
        oss << "{";
        
        // Version
        oss << "\"version\":{";
        oss << "\"name\":\"" << version.name << "\",";
        oss << "\"protocol\":" << version.protocol;
        oss << "},";
        
        // Players
        oss << "\"players\":{";
        oss << "\"max\":" << players.max << ",";
        oss << "\"online\":" << players.online;
        if (!players.sample.empty()) {
            oss << ",\"sample\":[";
            for (size_t i = 0; i < players.sample.size(); ++i) {
                if (i > 0) oss << ",";
                oss << "{\"name\":\"" << players.sample[i].name << "\",";
                oss << "\"id\":\"" << players.sample[i].id << "\"}";
            }
            oss << "]";
        }
        oss << "},";
        
        // Description
        oss << "\"description\":" << description.toJson() << ",";
        
        // Optional fields
        if (!favicon.empty()) {
            oss << "\"favicon\":\"" << favicon << "\",";
        }
        
        oss << "\"enforcesSecureChat\":" << (enforcesSecureChat ? "true" : "false") << ",";
        oss << "\"previewsChat\":" << (previewsChat ? "true" : "false");
        
        oss << "}";
        return oss.str();
    }
    
    // JSON에서 파싱 (간단한 구현)
    static ServerStatusInfo fromJson(const std::string& json);
};

// Clientbound Status Response Packet (0x00)
// 서버가 클라이언트에게 상태 정보를 응답하는 패킷
class StatusResponsePacket : public PacketBase<ProtocolState::STATUS, PacketDirection::CLIENTBOUND> {
private:
    std::string jsonResponse;
    ServerStatusInfo statusInfo;

public:
    StatusResponsePacket() = default;
    
    explicit StatusResponsePacket(const ServerStatusInfo& info) 
        : statusInfo(info) {
        jsonResponse = info.toJson();
    }
    
    explicit StatusResponsePacket(const std::string& json) 
        : jsonResponse(json) {
        statusInfo = ServerStatusInfo::fromJson(json);
    }

    int32_t getPacketId() const override {
        return PacketId::Status::Clientbound::STATUS_RESPONSE;
    }

    void serialize(ByteBuffer& buffer) const override {
        buffer.writeString(jsonResponse);
    }

    void deserialize(ByteBuffer& buffer) override {
        jsonResponse = buffer.readString();
        statusInfo = ServerStatusInfo::fromJson(jsonResponse);
    }

    std::string toString() const override {
        return "StatusResponsePacket{json=" + jsonResponse + "}";
    }

    const std::string& getJsonResponse() const { return jsonResponse; }
    const ServerStatusInfo& getStatusInfo() const { return statusInfo; }
    
    void setStatusInfo(const ServerStatusInfo& info) {
        statusInfo = info;
        jsonResponse = info.toJson();
    }
};

// Serverbound Ping Request Packet (0x01)
// 클라이언트가 핑 측정을 위해 보내는 패킷
class PingRequestPacket : public PacketBase<ProtocolState::STATUS, PacketDirection::SERVERBOUND> {
private:
    int64_t payload;

public:
    PingRequestPacket() : payload(0) {}
    explicit PingRequestPacket(int64_t payload) : payload(payload) {}

    int32_t getPacketId() const override {
        return PacketId::Status::Serverbound::PING_REQUEST;
    }

    void serialize(ByteBuffer& buffer) const override {
        buffer.writeLong(static_cast<uint64_t>(payload));
    }

    void deserialize(ByteBuffer& buffer) override {
        payload = static_cast<int64_t>(buffer.readLong());
    }

    std::string toString() const override {
        return "PingRequestPacket{payload=" + std::to_string(payload) + "}";
    }

    int64_t getPayload() const { return payload; }
    void setPayload(int64_t p) { payload = p; }
    
    // 타임스탬프를 페이로드로 사용하는 유틸리티
    static PingRequestPacket createWithTimestamp() {
        auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
        return PingRequestPacket(now);
    }
};

// Clientbound Pong Response Packet (0x01)
// 서버가 핑 요청에 응답하는 패킷
class PongResponsePacket : public PacketBase<ProtocolState::STATUS, PacketDirection::CLIENTBOUND> {
private:
    int64_t payload;

public:
    PongResponsePacket() : payload(0) {}
    explicit PongResponsePacket(int64_t payload) : payload(payload) {}

    int32_t getPacketId() const override {
        return PacketId::Status::Clientbound::PONG_RESPONSE;
    }

    void serialize(ByteBuffer& buffer) const override {
        buffer.writeLong(static_cast<uint64_t>(payload));
    }

    void deserialize(ByteBuffer& buffer) override {
        payload = static_cast<int64_t>(buffer.readLong());
    }

    std::string toString() const override {
        return "PongResponsePacket{payload=" + std::to_string(payload) + "}";
    }

    int64_t getPayload() const { return payload; }
    void setPayload(int64_t p) { payload = p; }
    
    // 핑 계산 유틸리티 (밀리초)
    int64_t calculatePing() const {
        auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
        return now - payload;
    }
};

// Status 상태 패킷 팩토리
class StatusPacketFactory : public IPacketFactory {
public:
    std::unique_ptr<IPacket> createPacket(int32_t packetId, ProtocolState state, PacketDirection direction) override {
        if (state != ProtocolState::STATUS) {
            return nullptr;
        }
        
        if (direction == PacketDirection::SERVERBOUND) {
            switch (packetId) {
                case PacketId::Status::Serverbound::STATUS_REQUEST:
                    return std::make_unique<StatusRequestPacket>();
                case PacketId::Status::Serverbound::PING_REQUEST:
                    return std::make_unique<PingRequestPacket>();
                default:
                    return nullptr;
            }
        } else if (direction == PacketDirection::CLIENTBOUND) {
            switch (packetId) {
                case PacketId::Status::Clientbound::STATUS_RESPONSE:
                    return std::make_unique<StatusResponsePacket>();
                case PacketId::Status::Clientbound::PONG_RESPONSE:
                    return std::make_unique<PongResponsePacket>();
                default:
                    return nullptr;
            }
        }
        
        return nullptr;
    }
};

// Status 관련 유틸리티
namespace utils {
    // 기본 서버 상태 정보 생성
    inline ServerStatusInfo createDefaultServerStatus(
        const std::string& motd = "A Minecraft Server",
        int32_t maxPlayers = 20,
        int32_t onlinePlayers = 0,
        const std::string& version = "1.20.4",
        int32_t protocol = 765
    ) {
        ServerStatusInfo info;
        
        info.version.name = version;
        info.version.protocol = protocol;
        
        info.players.max = maxPlayers;
        info.players.online = onlinePlayers;
        
        info.description = ChatComponent::fromPlainText(motd);
        
        return info;
    }
    
    // 플레이어 샘플 추가
    inline void addPlayerSample(ServerStatusInfo& info, const std::string& name, const std::string& uuid) {
        ServerStatusInfo::Players::Sample sample;
        sample.name = name;
        sample.id = uuid;
        info.players.sample.push_back(sample);
    }
    
    // 서버 아이콘 설정 (PNG를 base64로 인코딩된 문자열)
    inline void setServerIcon(ServerStatusInfo& info, const std::vector<uint8_t>& pngData) {
        // 실제로는 base64 인코딩을 구현해야 함
        // 여기서는 placeholder
        info.favicon = "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAEAAAABA...";
    }
    
    // 핑 측정 시작
    inline int64_t startPingMeasurement() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
    }
    
    // 핑 계산
    inline int64_t calculatePing(int64_t startTime) {
        auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
        return now - startTime;
    }
}

} // namespace minecraft::protocol::packets::status