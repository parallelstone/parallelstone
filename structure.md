# Minecraft Java Edition 서버 프로토콜 및 데이터 구조 가이드

## 목차
1. [프로토콜 개요](#프로토콜-개요)
2. [프로토콜 상태](#프로토콜-상태)
3. [패킷 구조](#패킷-구조)
4. [데이터 타입](#데이터-타입)
5. [각 상태별 패킷 상세](#각-상태별-패킷-상세)
6. [핵심 데이터 구조](#핵심-데이터-구조)
7. [레지스트리 데이터](#레지스트리-데이터)
8. [구현 참고사항](#구현-참고사항)

---

## 프로토콜 개요

Minecraft Java Edition의 네트워크 프로토콜은 TCP 연결을 기반으로 하며, 클라이언트와 서버 간의 모든 통신을 담당합니다.

### 기본 특징
- **TCP 기반**: 안정적인 연결 보장
- **상태 기반**: 5개의 주요 프로토콜 상태
- **패킷 기반**: 모든 데이터는 패킷 단위로 전송
- **압축 지원**: 일정 크기 이상의 패킷 압축
- **암호화**: RSA + AES 암호화 지원

### 연결 플로우
```
클라이언트 → 서버: Handshaking 상태
           ↓
       Status/Login 상태 선택
           ↓
    Configuration 상태 (1.20.2+)
           ↓
        Play 상태
```

---

## 프로토콜 상태

### 1. Handshaking (핸드셰이킹)
- **목적**: 연결 초기화 및 다음 상태 결정
- **패킷 수**: 1개 (Handshake)
- **특징**: 클라이언트에서 서버로만 전송

### 2. Status (상태 확인)
- **목적**: 서버 상태 조회 (서버 목록에서 확인)
- **주요 기능**: 
  - 서버 정보 조회 (플레이어 수, MOTD, 버전 등)
  - 핑 측정
- **패킷**: Status Request/Response, Ping Request/Response

### 3. Login (로그인)
- **목적**: 플레이어 인증 및 로그인
- **주요 기능**:
  - 온라인 모드 인증 (Mojang 서버와 통신)
  - 플레이어 UUID 확인
  - 암호화 설정
- **패킷**: Login Start, Encryption Request/Response, Login Success 등

### 4. Configuration (설정) - 1.20.2+
- **목적**: 게임 시작 전 클라이언트 설정
- **주요 기능**:
  - 리소스 팩 전송
  - 레지스트리 데이터 동기화
  - 플러그인 채널 설정
- **패킷**: Clientbound/Serverbound Known Packs, Registry Data 등

### 5. Play (게임 플레이)
- **목적**: 실제 게임 플레이 중 모든 통신
- **주요 기능**:
  - 플레이어 움직임
  - 블록 변경
  - 엔티티 상호작용
  - 채팅
  - 인벤토리 관리

---

## 패킷 구조

### 기본 패킷 형식
```
[길이] [압축 여부] [패킷 ID] [데이터]
```

#### 비압축 패킷
```
| VarInt | VarInt  | 데이터    |
| Length | Packet  | Packet   |
|        | ID      | Data     |
```

#### 압축 패킷 (압축 임계값 이상)
```
| VarInt | VarInt      | VarInt  | 압축된    |
| Packet | Data Length | Packet  | Packet   |
| Length |             | ID      | Data     |
```

### 패킷 처리 순서
1. **길이 읽기**: VarInt로 인코딩된 패킷 길이
2. **압축 확인**: Data Length가 0이면 비압축, 0이 아니면 압축
3. **압축 해제**: 필요시 zlib 압축 해제
4. **패킷 ID 추출**: VarInt로 인코딩된 패킷 식별자
5. **데이터 파싱**: 패킷별 정의된 구조에 따라 데이터 해석

---

## 데이터 타입

### 기본 정수형
| 타입 | 크기 | 범위 | 설명 |
|------|------|------|------|
| Boolean | 1 byte | true/false | 0x00 = false, 0x01 = true |
| Byte | 1 byte | -128 ~ 127 | 부호 있는 8비트 정수 |
| Unsigned Byte | 1 byte | 0 ~ 255 | 부호 없는 8비트 정수 |
| Short | 2 bytes | -32,768 ~ 32,767 | 빅 엔디안 부호 있는 16비트 |
| Unsigned Short | 2 bytes | 0 ~ 65,535 | 빅 엔디안 부호 없는 16비트 |
| Int | 4 bytes | -2³¹ ~ 2³¹-1 | 빅 엔디안 부호 있는 32비트 |
| Long | 8 bytes | -2⁶³ ~ 2⁶³-1 | 빅 엔디안 부호 있는 64비트 |

### 가변 길이 정수형
#### VarInt
- **용도**: 패킷 ID, 길이, 작은 정수값
- **인코딩**: 7비트씩 분할, MSB는 연속 여부 표시
- **범위**: -2³¹ ~ 2³¹-1
- **최대 크기**: 5 bytes

**VarInt 인코딩 예시**:
```
300 (0x012C) 인코딩:
1. 300 = 0b100101100
2. 7비트씩 분할: 0000010 | 0101100
3. 리틀 엔디안 + 연속 비트: 11101100 00000010
4. 결과: 0xEC 0x02
```

#### VarLong
- **용도**: 큰 정수값 (위치, 타임스탬프 등)
- **인코딩**: VarInt와 동일하지만 64비트
- **최대 크기**: 10 bytes

### 부동소수점
| 타입 | 크기 | 형식 |
|------|------|------|
| Float | 4 bytes | IEEE 754 단정밀도 |
| Double | 8 bytes | IEEE 754 배정밀도 |

### 문자열 및 바이너리
#### String
```
| VarInt | UTF-8 bytes |
| Length | String data |
```
- **최대 길이**: 32,767 characters
- **인코딩**: UTF-8

#### Chat
- JSON 형식의 문자열
- 텍스트 컴포넌트로 구성
- 색상, 스타일, 클릭 이벤트 등 지원

#### Identifier
```
namespace:path
```
- **namespace**: 리소스 네임스페이스 (기본값: "minecraft")
- **path**: 리소스 경로
- **예시**: "minecraft:stone", "mymod:custom_block"

### 특수 데이터 타입

#### UUID
- **크기**: 16 bytes
- **형식**: 128비트 범용 고유 식별자
- **바이트 순서**: 빅 엔디안

#### Position
- **크기**: 8 bytes (Long)
- **인코딩**: ((x & 0x3FFFFFF) << 38) | ((z & 0x3FFFFFF) << 12) | (y & 0xFFF)
- **범위**: 
  - X, Z: -33,554,432 ~ 33,554,431
  - Y: 0 ~ 4,095

#### Angle
- **크기**: 1 byte
- **범위**: 0 ~ 255
- **단위**: 360도를 256단계로 분할

#### NBT (Named Binary Tag)
- 구조화된 바이너리 데이터 형식
- 다양한 태그 타입 지원:
  - TAG_End (0)
  - TAG_Byte (1)
  - TAG_Short (2)
  - TAG_Int (3)
  - TAG_Long (4)
  - TAG_Float (5)
  - TAG_Double (6)
  - TAG_Byte_Array (7)
  - TAG_String (8)
  - TAG_List (9)
  - TAG_Compound (10)
  - TAG_Int_Array (11)
  - TAG_Long_Array (12)

#### BitSet
- 가변 길이 비트 집합
- Long 배열로 저장
- 용도: 청크 마스크, 업데이트 마스크 등

---

## 각 상태별 패킷 상세

### Handshaking 상태

#### Serverbound Handshake (0x00)
| 필드 | 타입 | 설명 |
|------|------|------|
| Protocol Version | VarInt | 클라이언트 프로토콜 버전 |
| Server Address | String | 접속할 서버 주소 |
| Server Port | Unsigned Short | 서버 포트 |
| Next State | VarInt | 다음 상태 (1=Status, 2=Login) |

### Status 상태

#### Clientbound Status Response (0x00)
| 필드 | 타입 | 설명 |
|------|------|------|
| JSON Response | String | 서버 상태 정보 (JSON) |

**JSON 응답 형식**:
```json
{
  "version": {
    "name": "1.20.4",
    "protocol": 765
  },
  "players": {
    "max": 100,
    "online": 5,
    "sample": [
      {
        "name": "thinkofdeath",
        "id": "4566e69f-c907-48ee-8d71-d7ba5aa00d20"
      }
    ]
  },
  "description": {
    "text": "Hello world"
  },
  "favicon": "data:image/png;base64,<data>",
  "enforcesSecureChat": true,
  "previewsChat": false
}
```

#### Serverbound Ping Request (0x01)
| 필드 | 타입 | 설명 |
|------|------|------|
| Payload | Long | 임의의 값 (핑 측정용) |

### Login 상태

#### Serverbound Login Start (0x00)
| 필드 | 타입 | 설명 |
|------|------|------|
| Name | String | 플레이어 이름 |
| Player UUID | UUID | 플레이어 UUID (오프라인 모드에서는 Optional) |

#### Clientbound Encryption Request (0x01)
| 필드 | 타입 | 설명 |
|------|------|------|
| Server ID | String | 서버 식별자 (보통 빈 문자열) |
| Public Key Length | VarInt | 공개키 길이 |
| Public Key | Byte Array | RSA 공개키 |
| Verify Token Length | VarInt | 검증 토큰 길이 |
| Verify Token | Byte Array | 임의의 검증 토큰 |

#### Serverbound Encryption Response (0x01)
| 필드 | 타입 | 설명 |
|------|------|------|
| Shared Secret Length | VarInt | 공유 비밀 길이 |
| Shared Secret | Byte Array | RSA로 암호화된 AES 키 |
| Verify Token Length | VarInt | 검증 토큰 길이 |
| Verify Token | Byte Array | RSA로 암호화된 검증 토큰 |

#### Clientbound Login Success (0x02)
| 필드 | 타입 | 설명 |
|------|------|------|
| UUID | UUID | 플레이어 UUID |
| Username | String | 플레이어 이름 |
| Number of Properties | VarInt | 속성 개수 |
| Properties | Array | 플레이어 속성 (스킨, 케이프 등) |

### Configuration 상태 (1.20.2+)

#### Clientbound Registry Data (0x05)
| 필드 | 타입 | 설명 |
|------|------|------|
| Registry Codec | NBT | 레지스트리 데이터 |

#### Clientbound Finish Configuration (0x02)
- 데이터 없음
- Configuration 상태 완료를 알림

#### Serverbound Finish Configuration (0x02)
- 데이터 없음
- 클라이언트가 Configuration 완료를 확인

### Play 상태 (주요 패킷들)

#### Clientbound Login (Play) (0x28)
| 필드 | 타입 | 설명 |
|------|------|------|
| Entity ID | Int | 플레이어 엔티티 ID |
| Is hardcore | Boolean | 하드코어 모드 여부 |
| Game mode | Unsigned Byte | 게임 모드 |
| Previous Game mode | Byte | 이전 게임 모드 (-1 = 없음) |
| Dimension Count | VarInt | 차원 개수 |
| Dimension Names | Array of Identifier | 차원 이름들 |
| Registry Codec | NBT | 레지스트리 데이터 |
| Dimension Type | Identifier | 차원 타입 |
| Dimension Name | Identifier | 현재 차원 이름 |
| Hashed seed | Long | 해시된 시드 |
| Max Players | VarInt | 최대 플레이어 수 |
| View Distance | VarInt | 시야 거리 |
| Simulation Distance | VarInt | 시뮬레이션 거리 |
| Reduced Debug Info | Boolean | 디버그 정보 축소 여부 |
| Enable respawn screen | Boolean | 리스폰 화면 활성화 |
| Do limited crafting | Boolean | 제한된 제작 여부 |
| Dimension Type | Identifier | 차원 타입 |
| Death Location | Optional | 사망 위치 |

#### Clientbound Chunk Data and Update Light (0x25)
| 필드 | 타입 | 설명 |
|------|------|------|
| Chunk X | Int | 청크 X 좌표 |
| Chunk Z | Int | 청크 Z 좌표 |
| Heightmaps | NBT | 높이맵 데이터 |
| Size | VarInt | 데이터 크기 |
| Data | Byte Array | 청크 데이터 |
| Number of block entities | VarInt | 블록 엔티티 개수 |
| Block entities | Array | 블록 엔티티 데이터 |
| Sky Light Mask | BitSet | 하늘 빛 마스크 |
| Block Light Mask | BitSet | 블록 빛 마스크 |
| Empty Sky Light Mask | BitSet | 빈 하늘 빛 마스크 |
| Empty Block Light Mask | BitSet | 빈 블록 빛 마스크 |
| Sky Light arrays | Array | 하늘 빛 데이터 |
| Block Light arrays | Array | 블록 빛 데이터 |

#### Serverbound Set Player Position (0x15)
| 필드 | 타입 | 설명 |
|------|------|------|
| X | Double | X 좌표 |
| Feet Y | Double | 발 Y 좌표 |
| Z | Double | Z 좌표 |
| On Ground | Boolean | 땅에 있는지 여부 |

#### Serverbound Set Player Position and Rotation (0x16)
| 필드 | 타입 | 설명 |
|------|------|------|
| X | Double | X 좌표 |
| Feet Y | Double | 발 Y 좌표 |
| Z | Double | Z 좌표 |
| Yaw | Float | 좌우 회전 (도) |
| Pitch | Float | 상하 회전 (도) |
| On Ground | Boolean | 땅에 있는지 여부 |

#### Clientbound Set Block (0x09)
| 필드 | 타입 | 설명 |
|------|------|------|
| Location | Position | 블록 위치 |
| Block ID | VarInt | 블록 상태 ID |

#### Serverbound Player Chat Message (0x05)
| 필드 | 타입 | 설명 |
|------|------|------|
| Message | String | 채팅 메시지 |
| Timestamp | Long | 타임스탬프 |
| Salt | Long | 솔트 (서명용) |
| Signature | Optional Byte Array | 메시지 서명 |
| Message Count | VarInt | 메시지 카운트 |
| Acknowledged | Fixed BitSet | 확인된 메시지들 |

---

## 핵심 데이터 구조

### 청크 (Chunk) 구조

#### 청크 기본 정보
- **크기**: 16×16 블록
- **높이**: 월드 최소~최대 높이 (보통 -64~319)
- **섹션**: 16블록 높이 단위로 분할

#### 청크 섹션 데이터
```
청크 섹션 (Chunk Section):
- Block count: Short (비어있지 않은 블록 수)
- Block states: PalettedContainer
- Biomes: PalettedContainer
```

#### PalettedContainer 구조
```
| 필드 | 타입 | 설명 |
|------|------|------|
| Bits per entry | Unsigned Byte | 엔트리당 비트 수 |
| Palette | 상황에 따라 | 팔레트 데이터 |
| Data Array Length | VarInt | 데이터 배열 길이 |
| Data Array | Array of Long | 압축된 블록 데이터 |
```

### 엔티티 (Entity) 구조

#### 기본 엔티티 데이터
| 필드 | 타입 | 설명 |
|------|------|------|
| Entity ID | VarInt | 엔티티 고유 ID |
| Entity UUID | UUID | 엔티티 UUID |
| Type | VarInt | 엔티티 타입 ID |
| X | Double | X 좌표 |
| Y | Double | Y 좌표 |
| Z | Double | Z 좌표 |
| Pitch | Angle | 상하 회전 |
| Yaw | Angle | 좌우 회전 |
| Head Yaw | Angle | 머리 회전 |
| Data | VarInt | 엔티티별 데이터 |
| Velocity X | Short | X 속도 |
| Velocity Y | Short | Y 속도 |
| Velocity Z | Short | Z 속도 |

#### 엔티티 메타데이터
엔티티의 추가 속성을 저장하는 키-값 구조:

```
| Index | Type | Value | 설명 |
|-------|------|-------|------|
| 0 | Byte | Entity flags | 엔티티 플래그 |
| 1 | VarInt | Air ticks | 공기 틱 |
| 2 | Optional Chat | Custom name | 커스텀 이름 |
| 3 | Boolean | Custom name visible | 이름 표시 여부 |
| 4 | Boolean | Silent | 소리 없음 여부 |
| 5 | Boolean | No gravity | 중력 없음 여부 |
| 6 | Pose | Pose | 자세 |
| 7 | VarInt | Frozen ticks | 얼음 틱 |
```

### 인벤토리 (Inventory) 구조

#### 슬롯 (Slot) 데이터
```
| 필드 | 타입 | 설명 |
|------|------|------|
| Present | Boolean | 아이템 존재 여부 |
| Item ID | VarInt | 아이템 ID (Present가 true일 때) |
| Item Count | Byte | 아이템 개수 |
| NBT | NBT | 아이템 NBT 데이터 |
```

#### 컨테이너 타입
- **Generic 9x1**: 상자, 디스펜서 등
- **Generic 9x2**: 큰 상자 위쪽
- **Generic 9x3**: 큰 상자, 샬커 박스 등
- **Player Inventory**: 플레이어 인벤토리
- **Crafting**: 제작대
- **Furnace**: 화로
- **Anvil**: 모루
- **Brewing Stand**: 양조기

### 블록 상태 (Block State)

#### 블록 상태 ID
- 각 블록의 모든 가능한 상태에 고유 ID 할당
- 예시: `minecraft:oak_log[axis=x]` → ID: 134

#### 속성 (Properties)
블록별로 다양한 속성 보유:
- **방향**: facing, axis
- **상태**: open, powered, lit
- **연결**: north, south, east, west, up, down
- **나이**: age (작물 성장 단계)
- **물**: waterlogged

### 월드 생성 관련

#### 바이옴 (Biome)
```json
{
  "precipitation": "rain",
  "temperature": 0.8,
  "downfall": 0.4,
  "effects": {
    "fog_color": 12638463,
    "water_color": 4159204,
    "water_fog_color": 329011,
    "sky_color": 7907327,
    "grass_color_modifier": "none"
  },
  "spawners": {
    "monster": [...],
    "creature": [...],
    "ambient": [...],
    "water_creature": [...],
    "water_ambient": [...],
    "misc": [...]
  },
  "spawn_costs": {},
  "carvers": {
    "air": [...],
    "liquid": [...]
  },
  "features": [...]
}
```

#### 차원 타입 (Dimension Type)
```json
{
  "fixed_time": 6000,
  "has_skylight": true,
  "has_ceiling": false,
  "ultrawarm": false,
  "natural": true,
  "coordinate_scale": 1.0,
  "bed_works": true,
  "respawn_anchor_works": false,
  "min_y": -64,
  "height": 384,
  "logical_height": 384,
  "infiniburn": "#minecraft:infiniburn_overworld",
  "effects": "minecraft:overworld",
  "ambient_light": 0.0
}
```

---

## 레지스트리 데이터

### 주요 레지스트리 목록
1. **minecraft:block**: 블록 정의
2. **minecraft:item**: 아이템 정의  
3. **minecraft:biome**: 바이옴 정의
4. **minecraft:dimension_type**: 차원 타입
5. **minecraft:entity_type**: 엔티티 타입
6. **minecraft:game_event**: 게임 이벤트
7. **minecraft:sound_event**: 사운드 이벤트
8. **minecraft:enchantment**: 인챈트
9. **minecraft:potion**: 포션 효과
10. **minecraft:particle_type**: 파티클 타입

### 레지스트리 데이터 구조
```json
{
  "type": "minecraft:dimension_type",
  "value": [
    {
      "name": "minecraft:overworld",
      "id": 0,
      "element": {
        "piglin_safe": false,
        "natural": true,
        "ambient_light": 0.0,
        "fixed_time": null,
        "infiniburn": "#minecraft:infiniburn_overworld",
        "respawn_anchor_works": false,
        "has_skylight": true,
        "bed_works": true,
        "effects": "minecraft:overworld",
        "has_raids": true,
        "min_y": -64,
        "height": 384,
        "logical_height": 384,
        "coordinate_scale": 1.0,
        "ultrawarm": false,
        "has_ceiling": false
      }
    }
  ]
}
```

### 레지스트리 동기화 과정
1. **Configuration 상태**에서 서버가 클라이언트에게 전송
2. **Registry Data 패킷**으로 모든 레지스트리 데이터 전송
3. 클라이언트는 이 데이터를 로컬에 저장하여 ID 매핑에 사용

---

## 구현 참고사항

### 1. 패킷 처리 아키텍처

#### 비동기 패킷 처리
```cpp
class PacketHandler {
    void handlePacket(int packetId, ByteBuffer& data) {
        switch(currentState) {
            case HANDSHAKING:
                handleHandshaking(packetId, data);
                break;
            case STATUS:
                handleStatus(packetId, data);
                break;
            case LOGIN:
                handleLogin(packetId, data);
                break;
            case CONFIGURATION:
                handleConfiguration(packetId, data);
                break;
            case PLAY:
                handlePlay(packetId, data);
                break;
        }
    }
};
```

#### 패킷 큐 관리
- **송신 큐**: 클라이언트로 보낼 패킷들을 버퍼링
- **수신 큐**: 클라이언트로부터 받은 패킷들을 처리 대기
- **우선순위**: 중요한 패킷 (위치 업데이트, 청크 로딩)에 높은 우선순위

### 2. 상태 전환 관리

#### 상태 전환 다이어그램
```
HANDSHAKING → STATUS (Next State = 1)
           → LOGIN (Next State = 2)

LOGIN → CONFIGURATION (1.20.2+)
      → PLAY (1.20.1 이하)

CONFIGURATION → PLAY
```

#### 상태별 유효한 패킷 검증
```cpp
bool isValidPacket(ProtocolState state, int packetId) {
    static const std::map<ProtocolState, std::set<int>> validPackets = {
        {HANDSHAKING, {0x00}},
        {STATUS, {0x00, 0x01}},
        {LOGIN, {0x00, 0x01, 0x02, 0x03}},
        // ... 추가 상태들
    };
    
    auto it = validPackets.find(state);
    return it != validPackets.end() && it->second.count(packetId) > 0;
}
```

### 3. 압축 처리

#### 압축 임계값 설정
- **기본값**: 256 bytes
- **설정 방법**: Set Compression 패킷으로 클라이언트에게 알림
- **압축 알고리즘**: zlib (RFC 1950)

#### 압축 구현 예시
```cpp
std::vector<uint8_t> compressPacket(const std::vector<uint8_t>& data) {
    if (data.size() < compressionThreshold) {
        return data; // 압축하지 않음
    }
    
    // zlib로 압축
    z_stream stream = {};
    deflateInit(&stream, Z_DEFAULT_COMPRESSION);
    
    std::vector<uint8_t> compressed(data.size());
    stream.next_in = const_cast<uint8_t*>(data.data());
    stream.avail_in = data.size();
    stream.next_out = compressed.data();
    stream.avail_out = compressed.size();
    
    deflate(&stream, Z_FINISH);
    deflateEnd(&stream);
    
    compressed.resize(stream.total_out);
    return compressed;
}
```

### 4. 암호화 구현

#### RSA 키 생성 (서버 시작 시)
```cpp
void generateRSAKeyPair() {
    RSA* rsa = RSA_new();
    BIGNUM* e = BN_new();
    BN_set_word(e, RSA_F4);
    
    RSA_generate_key_ex(rsa, 1024, e, nullptr);
    
    // 공개키와 개인키 저장
    publicKey = extractPublicKey(rsa);
    privateKey = extractPrivateKey(rsa);
}
```

#### AES 암호화 설정
```cpp
void setupAESEncryption(const std::vector<uint8_t>& sharedSecret) {
    // AES-128-CFB8 모드 사용
    aesKey = sharedSecret;
    
    // IV는 AES 키와 동일
    aesIV = sharedSecret;
    
    // 암호화/복호화 컨텍스트 초기화
    EVP_CIPHER_CTX_init(&encryptCtx);
    EVP_CIPHER_CTX_init(&decryptCtx);
    
    EVP_EncryptInit_ex(&encryptCtx, EVP_aes_128_cfb8(), nullptr, 
                       aesKey.data(), aesIV.data());
    EVP_DecryptInit_ex(&decryptCtx, EVP_aes_128_cfb8(), nullptr, 
                       aesKey.data(), aesIV.data());
}
```

### 5. 청크 관리

#### 청크 로딩 전략
- **시야 거리**: 클라이언트 설정에 따라 조정
- **미리 로딩**: 플레이어 이동 방향 예측하여 청크 미리 준비
- **언로딩**: 플레이어가 멀어진 청크는 메모리에서 해제

#### 청크 데이터 압축
```cpp
std::vector<uint8_t> serializeChunk(const Chunk& chunk) {
    std::vector<uint8_t> data;
    
    // 높이맵 직렬화
    NBT heightmaps = chunk.getHeightmaps();
    data.append(serializeNBT(heightmaps));
    
    // 각 섹션 직렬화
    for (const auto& section : chunk.getSections()) {
        data.append(serializeSection(section));
    }
    
    // 블록 엔티티 직렬화
    for (const auto& blockEntity : chunk.getBlockEntities()) {
        data.append(serializeBlockEntity(blockEntity));
    }
    
    return data;
}
```

### 6. 엔티티 관리

#### 엔티티 추적 (Entity Tracking)
- **추적 범위**: 엔티티 타입별로 다른 추적 거리
- **업데이트 주기**: 20 TPS (50ms마다)
- **델타 압축**: 변경된 필드만 전송

#### 엔티티 동기화
```cpp
void updateEntityForPlayer(Player* player, Entity* entity) {
    double distance = player->getLocation().distance(entity->getLocation());
    
    if (distance > entity->getTrackingRange()) {
        // 추적 범위를 벗어남 - 엔티티 제거
        player->sendPacket(RemoveEntitiesPacket{entity->getId()});
        return;
    }
    
    if (!player->isTracking(entity)) {
        // 새로운 엔티티 - 스폰 패킷 전송
        player->sendPacket(SpawnEntityPacket{entity});
        player->addTrackedEntity(entity);
    } else {
        // 기존 엔티티 - 델타 업데이트
        auto changes = entity->getChanges();
        if (!changes.empty()) {
            player->sendPacket(EntityDataPacket{entity->getId(), changes});
        }
    }
}
```

### 7. 성능 최적화

#### 패킷 배치 처리
```cpp
class PacketBatcher {
    std::vector<Packet> batch;
    
public:
    void addPacket(Packet packet) {
        batch.push_back(packet);
        
        if (batch.size() >= MAX_BATCH_SIZE) {
            flush();
        }
    }
    
    void flush() {
        if (!batch.empty()) {
            sendBatchedPackets(batch);
            batch.clear();
        }
    }
};
```

#### 메모리 풀 사용
```cpp
class PacketPool {
    std::queue<std::unique_ptr<Packet>> pool;
    std::mutex poolMutex;
    
public:
    std::unique_ptr<Packet> acquire() {
        std::lock_guard<std::mutex> lock(poolMutex);
        if (pool.empty()) {
            return std::make_unique<Packet>();
        }
        
        auto packet = std::move(pool.front());
        pool.pop();
        packet->reset();
        return packet;
    }
    
    void release(std::unique_ptr<Packet> packet) {
        std::lock_guard<std::mutex> lock(poolMutex);
        pool.push(std::move(packet));
    }
};
```

### 8. 오류 처리

#### 연결 오류 처리
```cpp
void handleConnectionError(Player* player, const std::exception& e) {
    logger.error("Player {} connection error: {}", 
                 player->getName(), e.what());
    
    // 정리 작업
    removePlayerFromWorld(player);
    closeConnection(player);
    
    // 다른 플레이어들에게 알림
    broadcastPlayerLeave(player);
}
```

#### 패킷 검증
```cpp
bool validatePacket(const Packet& packet, const Player& player) {
    // 패킷 크기 검증
    if (packet.size() > MAX_PACKET_SIZE) {
        return false;
    }
    
    // 상태별 패킷 유효성 검증
    if (!isValidPacketForState(packet.getId(), player.getState())) {
        return false;
    }
    
    // 플레이어별 권한 검증
    if (!player.hasPermission(packet.getRequiredPermission())) {
        return false;
    }
    
    return true;
}
```

### 9. 확장성 고려사항

#### 플러그인 시스템
```cpp
class PluginManager {
    std::vector<std::unique_ptr<Plugin>> plugins;
    
public:
    void loadPlugin(const std::string& path) {
        auto plugin = loadLibrary(path);
        plugins.push_back(std::move(plugin));
    }
    
    void onPacketReceived(Player* player, Packet* packet) {
        for (auto& plugin : plugins) {
            if (plugin->onPacketReceived(player, packet) == CANCEL) {
                return; // 패킷 처리 취소
            }
        }
    }
};
```

#### 이벤트 시스템
```cpp
template<typename EventType>
class EventBus {
    std::vector<std::function<void(const EventType&)>> handlers;
    
public:
    void subscribe(std::function<void(const EventType&)> handler) {
        handlers.push_back(handler);
    }
    
    void publish(const EventType& event) {
        for (const auto& handler : handlers) {
            handler(event);
        }
    }
};
```

이 가이드는 Minecraft Java Edition 서버 구현을 위한 핵심적인 프로토콜과 데이터 구조를 포괄적으로 다루고 있습니다. 실제 구현 시에는 각 부분을 더 자세히 연구하고, 최신 버전의 변경사항을 확인하는 것이 중요합니다.