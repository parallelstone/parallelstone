# Development Workflow Rules for ParellelStone Minecraft Server

## Check the `agent/current.md` file for the current development environment setup.

## Rule 1: Code Documentation Standards

### 1.1 Header Documentation Requirements
- **MUST** include comprehensive header comments for all public classes and functions
- **MUST** use Doxygen-style comments (`/**` and `*/`) for API documentation
- **MUST** document all parameters, return values, and exceptions
- **MUST** provide usage examples for complex protocols and packet handlers

```cpp
/**
 * @brief Handles Minecraft protocol packet serialization and deserialization
 * @details This class manages the complete packet lifecycle including compression,
 *          encryption, and validation according to Minecraft Java Edition protocol
 * @author Development Team
 * @version 1.0.0
 * @since Protocol Version 765 (Minecraft 1.20.4)
 */
class PacketHandler {
    /**
     * @brief Processes incoming packet data and routes to appropriate handlers
     * @param packetId The unique identifier for the packet type
     * @param data Raw packet data buffer
     * @param state Current protocol state (HANDSHAKING, STATUS, LOGIN, etc.)
     * @throws PacketValidationException if packet structure is invalid
     * @throws StateTransitionException if packet is not valid for current state
     * @return true if packet was processed successfully, false otherwise
     */
    bool processPacket(int32_t packetId, const ByteBuffer& data, ProtocolState state);
};
```

### 1.2 Protocol Documentation Standards
- **MUST** maintain accurate protocol specification documentation in `structure.md`
- **MUST** update packet registry documentation when adding new packet types
- **MUST** document all data type serialization formats and byte order requirements
- **MUST** include packet flow diagrams for complex protocol interactions

### 1.3 Architecture Documentation
- **MUST** document design decisions and architectural patterns used
- **MUST** maintain cross-platform compatibility notes and platform-specific implementations
- **MUST** document thread safety guarantees and concurrent access patterns
- **MUST** provide performance benchmarks and optimization notes

## Rule 2: Feature Implementation Guidelines

### 2.1 Protocol Implementation Standards
- **MUST** implement all packet types according to official Minecraft protocol specification
- **MUST** use type-safe packet handling with C++20 concepts and templates
- **MUST** implement proper packet validation before processing
- **MUST** support both compressed and uncompressed packet formats

```cpp
// Example of type-safe packet implementation
template<typename PacketType>
concept MinecraftPacket = requires(PacketType packet) {
    { packet.getPacketId() } -> std::same_as<int32_t>;
    { packet.serialize() } -> std::same_as<std::vector<uint8_t>>;
    { packet.deserialize(std::declval<ByteBuffer&>()) } -> std::same_as<void>;
};
```

### 2.2 Cross-Platform Implementation
- **MUST** ensure all features work on Windows (x86_64), Linux (x86_64/arm64), and macOS (arm64)
- **MUST** use platform-agnostic data types and byte order handling
- **MUST** implement conditional compilation for platform-specific code using macros
- **MUST** test network functionality on all supported platforms

### 2.3 Memory Management and Performance
- **MUST** use RAII principles for all resource management
- **MUST** implement object pooling for frequently allocated packet objects
- **MUST** use smart pointers (std::unique_ptr, std::shared_ptr) for dynamic memory
- **MUST** implement efficient serialization with minimal memory allocations

### 2.4 Error Handling and Validation
- **MUST** implement comprehensive input validation for all network data
- **MUST** use C++ exceptions for error handling with appropriate exception types
- **MUST** implement graceful degradation for network errors and connection issues
- **MUST** validate packet sizes and prevent buffer overflow attacks

## Rule 3: Debugging and Troubleshooting Protocols

### 3.1 Logging Infrastructure
- **MUST** implement structured logging with multiple log levels (DEBUG, INFO, WARN, ERROR)
- **MUST** include packet-level logging for protocol debugging
- **MUST** implement performance logging for network throughput and latency metrics
- **MUST** support both file-based and console logging with configurable formats

```cpp
// Example logging implementation
class PacketLogger {
public:
    void logPacket(const IPacket& packet, PacketDirection direction, 
                   const std::string& endpoint) {
        LOG_DEBUG("Packet {} {} - ID: 0x{:02X}, Size: {} bytes, Endpoint: {}", 
                  direction == PacketDirection::INBOUND ? "RECV" : "SEND",
                  packet.getPacketName(), packet.getPacketId(), 
                  packet.getSize(), endpoint);
    }
};
```

### 3.2 Debug Build Configuration
- **MUST** enable comprehensive debug symbols in Debug builds
- **MUST** implement debug-only packet validation and sanity checks
- **MUST** include memory leak detection and address sanitizer support
- **MUST** provide debug visualization tools for packet flow analysis

### 3.3 Network Debugging Tools
- **MUST** implement packet capture and replay functionality for debugging
- **MUST** provide network traffic analysis tools for protocol compliance
- **MUST** implement connection state visualization and monitoring
- **MUST** support protocol fuzzing and stress testing capabilities

### 3.4 Error Reporting and Diagnostics
- **MUST** implement automatic crash reporting with stack traces
- **MUST** provide detailed error context including protocol state and packet history
- **MUST** implement health check endpoints for server monitoring
- **MUST** support remote debugging capabilities for production issues

## Rule 4: Test Code Development Standards

### 4.1 Unit Testing Requirements
- **MUST** achieve minimum 80% code coverage for all protocol handling code
- **MUST** implement comprehensive packet serialization/deserialization tests
- **MUST** test all data type encoding/decoding functions with edge cases
- **MUST** verify protocol state transitions and validation logic

```cpp
// Example unit test structure
class PacketSerializationTest : public ::testing::Test {
protected:
    void SetUp() override {
        stream = std::make_unique<PacketStream>();
        // Initialize test data
    }
    
    void TearDown() override {
        stream.reset();
    }
    
    std::unique_ptr<PacketStream> stream;
};

TEST_F(PacketSerializationTest, HandshakePacketSerialization) {
    HandshakePacket packet(765, "localhost", 25565, ProtocolState::LOGIN);
    
    auto serialized = stream->serializePacket(packet);
    auto deserialized = stream->deserializePacket(serialized);
    
    ASSERT_EQ(packet.getProtocolVersion(), deserialized->getProtocolVersion());
    ASSERT_EQ(packet.getServerAddress(), deserialized->getServerAddress());
    ASSERT_EQ(packet.getServerPort(), deserialized->getServerPort());
    ASSERT_EQ(packet.getNextState(), deserialized->getNextState());
}
```

### 4.2 Integration Testing
- **MUST** implement end-to-end protocol flow testing with real Minecraft clients
- **MUST** test server behavior under various network conditions (latency, packet loss)
- **MUST** verify compatibility with different Minecraft client versions
- **MUST** implement load testing for concurrent client connections

### 4.3 Cross-Platform Testing
- **MUST** run test suite on all supported platforms (Windows, Linux, macOS)
- **MUST** test platform-specific networking code and byte order handling
- **MUST** verify build system compatibility across all platforms
- **MUST** implement automated CI/CD testing for all platform combinations

### 4.4 Performance and Stress Testing
- **MUST** implement benchmarks for packet processing throughput
- **MUST** test memory usage under high load conditions
- **MUST** verify server stability during extended operation periods
- **MUST** implement regression testing for performance optimizations

### 4.5 Security Testing
- **MUST** test encryption and authentication mechanisms
- **MUST** implement fuzzing tests for malformed packet handling
- **MUST** verify protection against common network attacks (DDoS, buffer overflow)
- **MUST** test secure handling of player authentication and session management

## Compliance and Review Process

### Code Review Requirements
- **MUST** conduct peer review for all protocol implementation changes
- **MUST** verify compliance with Minecraft protocol specification
- **MUST** ensure backward compatibility with existing client connections
- **MUST** validate performance impact of new features

### Continuous Integration
- **MUST** run full test suite on every commit to main branch
- **MUST** perform static code analysis and security scanning
- **MUST** verify cross-platform build compatibility
- **MUST** generate and publish code coverage reports

### Documentation Updates
- **MUST** update all relevant documentation when implementing new features
- **MUST** maintain changelog with detailed release notes
- **MUST** provide migration guides for breaking changes
- **MUST** update API documentation and examples

---

*These rules ensure consistent, high-quality development practices for the ParellelStone Minecraft server project while maintaining compatibility with the official Minecraft protocol and supporting cross-platform deployment.*