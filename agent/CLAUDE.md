# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

ParellelStone is a C++20 Minecraft server implementation using CMake as the build system. The project implements the Minecraft Java Edition protocol with full support for networking, packet handling, and game state management.

## Dependencies

- **Boost.Asio**: For network programming and async I/O
- **Boost.System**: Required by Boost.Asio
- **Boost.Thread**: For thread management

## Cross-Platform Build

### Supported Platforms & Architectures
- **Windows**: x86_64 only
- **Linux**: x86_64 and arm64
- **macOS**: arm64 only

### Quick Build (Recommended)
Use platform-specific build scripts:

```bash
# Linux (auto-detects x86_64 or arm64)
./scripts/build_linux.sh

# macOS (arm64)
./scripts/build_macos.sh

# Windows (x86_64)
scripts\build_windows.bat
```

### Manual Build Commands

#### Linux (x86_64 or arm64)
```bash
# Install dependencies
sudo apt-get update
sudo apt-get install -y cmake build-essential

# Auto-detect architecture and use vcpkg
# For x86_64: vcpkg install --triplet=x64-linux
# For arm64: vcpkg install --triplet=arm64-linux

# Build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake
make -j$(nproc)
```

#### macOS (arm64)
```bash
# Use vcpkg for dependencies
vcpkg install --triplet=arm64-osx

# Build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH=../vcpkg_installed/arm64-osx
make -j$(sysctl -n hw.ncpu)
```

#### Windows (x86_64)
```bash
# Install vcpkg and dependencies
vcpkg install --triplet=x64-windows

# Build
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake \
    -DVCPKG_TARGET_TRIPLET=x64-windows
cmake --build . --config Release --parallel
```

## Development Commands

```bash
# Debug build
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Release build (default)
cmake -DCMAKE_BUILD_TYPE=Release ..

# Build with tests
cmake -DBUILD_TESTS=ON ..
```

## Project Structure

```
ParellelStone/
├── CMakeLists.txt          # CMake configuration
├── vcpkg.json             # Package dependencies
├── structure.md           # Minecraft protocol documentation
├── src/                   # Source files
│   └── main.cpp          # Server entry point
├── include/               # Header files
│   ├── platform.hpp      # Cross-platform utilities
│   └── protocol/          # Minecraft protocol implementation
│       ├── data_types.hpp       # Basic data types (VarInt, UUID, etc.)
│       ├── protocol_state.hpp   # Protocol states and packet IDs
│       ├── packet.hpp           # Base packet classes and interfaces
│       ├── packet_registry.hpp  # Packet factory and registry
│       └── packets/             # Protocol-specific packets
│           ├── handshaking.hpp  # Handshaking state packets
│           ├── status.hpp       # Status state packets
│           ├── login.hpp        # Login state packets
│           └── play.hpp         # Play state packets
├── scripts/               # Build scripts
│   ├── build_linux.sh    # Linux build script
│   ├── build_macos.sh    # macOS build script
│   └── build_windows.bat # Windows build script
├── .github/workflows/     # CI/CD workflows
├── tests/                 # Test files
├── docs/                  # Documentation
└── build/                 # Build directory (generated)
```

## Architecture

- **Language**: C++20
- **Build System**: CMake 3.20+
- **Networking**: Boost.Asio for async I/O
- **Threading**: C++ standard library + Boost.Thread
- **Platform**: Cross-platform (Linux/macOS/Windows)
- **Protocol**: Minecraft Java Edition 1.20.4 (Protocol 765)

### Minecraft Protocol Implementation

#### Core Components
- **Data Types** (`data_types.hpp`): VarInt, VarLong, UUID, Position, Angle, ChatComponent, Identifier
- **Protocol States** (`protocol_state.hpp`): Handshaking, Status, Login, Configuration, Play
- **Packet System** (`packet.hpp`): Base packet interfaces, compression, encryption, streaming
- **Packet Registry** (`packet_registry.hpp`): Factory pattern for packet creation and management

#### Protocol States
1. **Handshaking**: Initial connection and state selection
2. **Status**: Server information queries and ping measurement
3. **Login**: Player authentication and encryption setup
4. **Configuration**: Client setup and registry synchronization (1.20.2+)
5. **Play**: Active gameplay packet handling

#### Key Features
- **Type-safe packet handling** with C++20 templates
- **Automatic packet serialization/deserialization**
- **Built-in compression support** (zlib)
- **AES encryption** for secure communication
- **Extensible packet registry** for easy protocol updates
- **Cross-platform byte order handling**

#### Implemented Packets
- **Handshaking**: HandshakePacket
- **Status**: StatusRequest/Response, Ping/Pong with server info
- **Login**: LoginStart, EncryptionRequest/Response, LoginSuccess, SetCompression
- **Play**: LoginPlay, SetPlayerPosition, SetBlock, ChatMessage, KeepAlive

### Cross-Platform Features

- Platform detection macros in `include/platform.hpp`
- Conditional compilation for platform-specific code
- Unified build system across all platforms
- Automated CI/CD testing on all platforms

## Development Guidelines

### Protocol Development
- Follow Minecraft protocol specifications from `structure.md`
- Implement packet validation and error handling
- Use the packet registry for all new packet types
- Maintain backward compatibility when possible

### Code Style
- Use C++20 features (concepts, modules, ranges when appropriate)
- Follow RAII principles for resource management
- Implement proper error handling with exceptions or error codes
- Use smart pointers for memory management

### Testing
- Write unit tests for all packet serialization/deserialization
- Test cross-platform compatibility
- Verify protocol compliance with official Minecraft client

## License

This project is licensed under the Apache License 2.0 as specified in the LICENSE file.
