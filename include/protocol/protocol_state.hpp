#pragma once

#include <cstdint>
#include <string>

namespace minecraft::protocol {

// 프로토콜 상태 열거형
enum class ProtocolState : int32_t {
    HANDSHAKING = 0,    // 핸드셰이킹 상태
    STATUS = 1,         // 서버 상태 조회
    LOGIN = 2,          // 로그인 과정
    CONFIGURATION = 3,  // 설정 단계 (1.20.2+)
    PLAY = 4           // 게임 플레이
};

// 프로토콜 방향 (패킷이 전송되는 방향)
enum class PacketDirection {
    SERVERBOUND,    // 클라이언트 → 서버
    CLIENTBOUND     // 서버 → 클라이언트
};

// 게임 모드
enum class GameMode : uint8_t {
    SURVIVAL = 0,
    CREATIVE = 1,
    ADVENTURE = 2,
    SPECTATOR = 3
};

// 난이도
enum class Difficulty : uint8_t {
    PEACEFUL = 0,
    EASY = 1,
    NORMAL = 2,
    HARD = 3
};

// 엔티티 자세
enum class Pose : int32_t {
    STANDING = 0,
    FALL_FLYING = 1,
    SLEEPING = 2,
    SWIMMING = 3,
    SPIN_ATTACK = 4,
    SNEAKING = 5,
    LONG_JUMPING = 6,
    DYING = 7,
    CROAKING = 8,
    USING_TONGUE = 9,
    SITTING = 10,
    ROARING = 11,
    SNIFFING = 12,
    EMERGING = 13,
    DIGGING = 14
};

// 패킷 ID 정의
namespace PacketId {
    // Handshaking State
    namespace Handshaking {
        namespace Serverbound {
            constexpr int32_t HANDSHAKE = 0x00;
        }
    }

    // Status State
    namespace Status {
        namespace Serverbound {
            constexpr int32_t STATUS_REQUEST = 0x00;
            constexpr int32_t PING_REQUEST = 0x01;
        }
        namespace Clientbound {
            constexpr int32_t STATUS_RESPONSE = 0x00;
            constexpr int32_t PONG_RESPONSE = 0x01;
        }
    }

    // Login State
    namespace Login {
        namespace Serverbound {
            constexpr int32_t LOGIN_START = 0x00;
            constexpr int32_t ENCRYPTION_RESPONSE = 0x01;
            constexpr int32_t LOGIN_PLUGIN_RESPONSE = 0x02;
        }
        namespace Clientbound {
            constexpr int32_t DISCONNECT = 0x00;
            constexpr int32_t ENCRYPTION_REQUEST = 0x01;
            constexpr int32_t LOGIN_SUCCESS = 0x02;
            constexpr int32_t SET_COMPRESSION = 0x03;
            constexpr int32_t LOGIN_PLUGIN_REQUEST = 0x04;
        }
    }

    // Configuration State
    namespace Configuration {
        namespace Serverbound {
            constexpr int32_t CLIENT_INFORMATION = 0x00;
            constexpr int32_t PLUGIN_MESSAGE = 0x01;
            constexpr int32_t FINISH_CONFIGURATION = 0x02;
            constexpr int32_t KEEP_ALIVE = 0x03;
            constexpr int32_t PONG = 0x04;
            constexpr int32_t RESOURCE_PACK_RESPONSE = 0x05;
        }
        namespace Clientbound {
            constexpr int32_t PLUGIN_MESSAGE = 0x00;
            constexpr int32_t DISCONNECT = 0x01;
            constexpr int32_t FINISH_CONFIGURATION = 0x02;
            constexpr int32_t KEEP_ALIVE = 0x03;
            constexpr int32_t PING = 0x04;
            constexpr int32_t REGISTRY_DATA = 0x05;
            constexpr int32_t REMOVE_RESOURCE_PACK = 0x06;
            constexpr int32_t ADD_RESOURCE_PACK = 0x07;
            constexpr int32_t FEATURE_FLAGS = 0x08;
            constexpr int32_t UPDATE_TAGS = 0x09;
        }
    }

    // Play State (주요 패킷들만)
    namespace Play {
        namespace Serverbound {
            constexpr int32_t CONFIRM_TELEPORTATION = 0x00;
            constexpr int32_t QUERY_BLOCK_NBT = 0x01;
            constexpr int32_t CHANGE_DIFFICULTY = 0x02;
            constexpr int32_t CHAT_ACK = 0x03;
            constexpr int32_t CHAT_COMMAND = 0x04;
            constexpr int32_t CHAT_MESSAGE = 0x05;
            constexpr int32_t CLIENT_COMMAND = 0x06;
            constexpr int32_t CLIENT_INFORMATION = 0x07;
            constexpr int32_t COMMAND_SUGGESTIONS_REQUEST = 0x08;
            constexpr int32_t CLICK_CONTAINER_BUTTON = 0x09;
            constexpr int32_t CLICK_CONTAINER = 0x0A;
            constexpr int32_t CLOSE_CONTAINER = 0x0B;
            constexpr int32_t PLUGIN_MESSAGE = 0x0C;
            constexpr int32_t EDIT_BOOK = 0x0D;
            constexpr int32_t QUERY_ENTITY_NBT = 0x0E;
            constexpr int32_t INTERACT = 0x0F;
            constexpr int32_t JIGSAW_GENERATE = 0x10;
            constexpr int32_t KEEP_ALIVE = 0x11;
            constexpr int32_t LOCK_DIFFICULTY = 0x12;
            constexpr int32_t SET_PLAYER_POSITION = 0x13;
            constexpr int32_t SET_PLAYER_POSITION_AND_ROTATION = 0x14;
            constexpr int32_t SET_PLAYER_ROTATION = 0x15;
            constexpr int32_t SET_PLAYER_ON_GROUND = 0x16;
            constexpr int32_t MOVE_VEHICLE = 0x17;
            constexpr int32_t PADDLE_BOAT = 0x18;
            constexpr int32_t PICK_ITEM = 0x19;
            constexpr int32_t PLACE_RECIPE = 0x1A;
            constexpr int32_t PLAYER_ABILITIES = 0x1B;
            constexpr int32_t PLAYER_ACTION = 0x1C;
            constexpr int32_t PLAYER_COMMAND = 0x1D;
            constexpr int32_t PLAYER_INPUT = 0x1E;
            constexpr int32_t PONG = 0x1F;
            constexpr int32_t CHANGE_RECIPE_BOOK_SETTINGS = 0x20;
            constexpr int32_t SET_SEEN_RECIPE = 0x21;
            constexpr int32_t RENAME_ITEM = 0x22;
            constexpr int32_t RESOURCE_PACK_RESPONSE = 0x23;
            constexpr int32_t SEEN_ADVANCEMENTS = 0x24;
            constexpr int32_t SELECT_TRADE = 0x25;
            constexpr int32_t SET_BEACON_EFFECT = 0x26;
            constexpr int32_t SET_HELD_ITEM = 0x27;
            constexpr int32_t PROGRAM_COMMAND_BLOCK = 0x28;
            constexpr int32_t PROGRAM_COMMAND_BLOCK_MINECART = 0x29;
            constexpr int32_t SET_CREATIVE_MODE_SLOT = 0x2A;
            constexpr int32_t PROGRAM_JIGSAW_BLOCK = 0x2B;
            constexpr int32_t PROGRAM_STRUCTURE_BLOCK = 0x2C;
            constexpr int32_t UPDATE_SIGN = 0x2D;
            constexpr int32_t SWING_ARM = 0x2E;
            constexpr int32_t TELEPORT_TO_ENTITY = 0x2F;
            constexpr int32_t USE_ITEM_ON = 0x30;
            constexpr int32_t USE_ITEM = 0x31;
        }
        
        namespace Clientbound {
            constexpr int32_t BUNDLE_DELIMITER = 0x00;
            constexpr int32_t SPAWN_ENTITY = 0x01;
            constexpr int32_t SPAWN_EXPERIENCE_ORB = 0x02;
            constexpr int32_t ENTITY_ANIMATION = 0x03;
            constexpr int32_t AWARD_STATISTICS = 0x04;
            constexpr int32_t ACKNOWLEDGE_BLOCK_CHANGE = 0x05;
            constexpr int32_t SET_BLOCK_DESTROY_STAGE = 0x06;
            constexpr int32_t BLOCK_ENTITY_DATA = 0x07;
            constexpr int32_t BLOCK_ACTION = 0x08;
            constexpr int32_t BLOCK_CHANGE = 0x09;
            constexpr int32_t BOSS_BAR = 0x0A;
            constexpr int32_t CHANGE_DIFFICULTY = 0x0B;
            constexpr int32_t CHUNK_BATCH_FINISHED = 0x0C;
            constexpr int32_t CHUNK_BATCH_START = 0x0D;
            constexpr int32_t CHUNK_BIOMES = 0x0E;
            constexpr int32_t CLEAR_TITLES = 0x0F;
            constexpr int32_t COMMAND_SUGGESTIONS_RESPONSE = 0x10;
            constexpr int32_t COMMANDS = 0x11;
            constexpr int32_t CLOSE_CONTAINER = 0x12;
            constexpr int32_t SET_CONTAINER_CONTENT = 0x13;
            constexpr int32_t SET_CONTAINER_PROPERTY = 0x14;
            constexpr int32_t SET_CONTAINER_SLOT = 0x15;
            constexpr int32_t SET_COOLDOWN = 0x16;
            constexpr int32_t CHAT_SUGGESTIONS = 0x17;
            constexpr int32_t PLUGIN_MESSAGE = 0x18;
            constexpr int32_t DAMAGE_EVENT = 0x19;
            constexpr int32_t DELETE_MESSAGE = 0x1A;
            constexpr int32_t DISCONNECT = 0x1B;
            constexpr int32_t DISGUISED_CHAT = 0x1C;
            constexpr int32_t ENTITY_EVENT = 0x1D;
            constexpr int32_t EXPLOSION = 0x1E;
            constexpr int32_t UNLOAD_CHUNK = 0x1F;
            constexpr int32_t GAME_EVENT = 0x20;
            constexpr int32_t OPEN_HORSE_SCREEN = 0x21;
            constexpr int32_t HURT_ANIMATION = 0x22;
            constexpr int32_t INITIALIZE_WORLD_BORDER = 0x23;
            constexpr int32_t KEEP_ALIVE = 0x24;
            constexpr int32_t CHUNK_DATA_AND_UPDATE_LIGHT = 0x25;
            constexpr int32_t WORLD_EVENT = 0x26;
            constexpr int32_t PARTICLE = 0x27;
            constexpr int32_t UPDATE_LIGHT = 0x28;
            constexpr int32_t LOGIN = 0x29;
            constexpr int32_t MAP_DATA = 0x2A;
            constexpr int32_t MERCHANT_OFFERS = 0x2B;
            constexpr int32_t UPDATE_ENTITY_POSITION = 0x2C;
            constexpr int32_t UPDATE_ENTITY_POSITION_AND_ROTATION = 0x2D;
            constexpr int32_t UPDATE_ENTITY_ROTATION = 0x2E;
            constexpr int32_t MOVE_VEHICLE = 0x2F;
            constexpr int32_t OPEN_BOOK = 0x30;
            constexpr int32_t OPEN_SCREEN = 0x31;
            constexpr int32_t OPEN_SIGN_EDITOR = 0x32;
            constexpr int32_t PING = 0x33;
            constexpr int32_t PLACE_GHOST_RECIPE = 0x34;
            constexpr int32_t PLAYER_ABILITIES = 0x35;
            constexpr int32_t PLAYER_CHAT_MESSAGE = 0x36;
            constexpr int32_t END_COMBAT = 0x37;
            constexpr int32_t ENTER_COMBAT = 0x38;
            constexpr int32_t COMBAT_DEATH = 0x39;
            constexpr int32_t PLAYER_INFO_REMOVE = 0x3A;
            constexpr int32_t PLAYER_INFO_UPDATE = 0x3B;
            constexpr int32_t LOOK_AT = 0x3C;
            constexpr int32_t SYNCHRONIZE_PLAYER_POSITION = 0x3D;
            constexpr int32_t UPDATE_RECIPE_BOOK = 0x3E;
            constexpr int32_t REMOVE_ENTITIES = 0x3F;
            constexpr int32_t REMOVE_ENTITY_EFFECT = 0x40;
            constexpr int32_t RESOURCE_PACK_SEND = 0x41;
            constexpr int32_t RESPAWN = 0x42;
            constexpr int32_t SET_HEAD_ROTATION = 0x43;
            constexpr int32_t UPDATE_SECTION_BLOCKS = 0x44;
            constexpr int32_t SELECT_ADVANCEMENTS_TAB = 0x45;
            constexpr int32_t SERVER_DATA = 0x46;
            constexpr int32_t SET_ACTION_BAR_TEXT = 0x47;
            constexpr int32_t SET_BORDER_CENTER = 0x48;
            constexpr int32_t SET_BORDER_LERP_SIZE = 0x49;
            constexpr int32_t SET_BORDER_SIZE = 0x4A;
            constexpr int32_t SET_BORDER_WARNING_DELAY = 0x4B;
            constexpr int32_t SET_BORDER_WARNING_DISTANCE = 0x4C;
            constexpr int32_t SET_CAMERA = 0x4D;
            constexpr int32_t SET_HELD_ITEM = 0x4E;
            constexpr int32_t SET_CENTER_CHUNK = 0x4F;
            constexpr int32_t SET_RENDER_DISTANCE = 0x50;
            constexpr int32_t SET_DEFAULT_SPAWN_POSITION = 0x51;
            constexpr int32_t DISPLAY_OBJECTIVE = 0x52;
            constexpr int32_t SET_ENTITY_METADATA = 0x53;
            constexpr int32_t LINK_ENTITIES = 0x54;
            constexpr int32_t SET_ENTITY_VELOCITY = 0x55;
            constexpr int32_t SET_EQUIPMENT = 0x56;
            constexpr int32_t SET_EXPERIENCE = 0x57;
            constexpr int32_t SET_HEALTH = 0x58;
            constexpr int32_t UPDATE_OBJECTIVES = 0x59;
            constexpr int32_t SET_PASSENGERS = 0x5A;
            constexpr int32_t UPDATE_TEAMS = 0x5B;
            constexpr int32_t UPDATE_SCORE = 0x5C;
            constexpr int32_t SET_SIMULATION_DISTANCE = 0x5D;
            constexpr int32_t SET_SUBTITLE_TEXT = 0x5E;
            constexpr int32_t UPDATE_TIME = 0x5F;
            constexpr int32_t SET_TITLE_TEXT = 0x60;
            constexpr int32_t SET_TITLE_ANIMATION_TIMES = 0x61;
            constexpr int32_t ENTITY_SOUND_EFFECT = 0x62;
            constexpr int32_t SOUND_EFFECT = 0x63;
            constexpr int32_t START_CONFIGURATION = 0x64;
            constexpr int32_t STOP_SOUND = 0x65;
            constexpr int32_t SYSTEM_CHAT_MESSAGE = 0x66;
            constexpr int32_t SET_TAB_LIST_HEADER_AND_FOOTER = 0x67;
            constexpr int32_t TAG_QUERY_RESPONSE = 0x68;
            constexpr int32_t PICKUP_ITEM = 0x69;
            constexpr int32_t TELEPORT_ENTITY = 0x6A;
            constexpr int32_t UPDATE_ADVANCEMENTS = 0x6B;
            constexpr int32_t UPDATE_ATTRIBUTES = 0x6C;
            constexpr int32_t ENTITY_EFFECT = 0x6D;
            constexpr int32_t UPDATE_RECIPES = 0x6E;
            constexpr int32_t UPDATE_TAGS = 0x6F;
        }
    }
}

// 유틸리티 함수들
inline std::string protocolStateToString(ProtocolState state) {
    switch (state) {
        case ProtocolState::HANDSHAKING: return "HANDSHAKING";
        case ProtocolState::STATUS: return "STATUS";
        case ProtocolState::LOGIN: return "LOGIN";
        case ProtocolState::CONFIGURATION: return "CONFIGURATION";
        case ProtocolState::PLAY: return "PLAY";
        default: return "UNKNOWN";
    }
}

inline std::string packetDirectionToString(PacketDirection direction) {
    switch (direction) {
        case PacketDirection::SERVERBOUND: return "SERVERBOUND";
        case PacketDirection::CLIENTBOUND: return "CLIENTBOUND";
        default: return "UNKNOWN";
    }
}

} // namespace minecraft::protocol