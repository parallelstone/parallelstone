# cuberite.md

This file provides analysis of the Cuberite repository structure - a C++ Minecraft server implementation.

## Project Overview

Cuberite is a Minecraft-compatible multiplayer game server written in C++ and designed to be efficient with memory and CPU, featuring a flexible Lua Plugin API. It's compatible with the Java Edition Minecraft client.

## Key Features

- **Languages**: C++ (core) and Lua (plugins)
- **Build System**: CMake 3.13+
- **Supported Platforms**: Windows, Linux, macOS, FreeBSD, Android (including Raspberry Pi)
- **Protocol Support**: Minecraft Java Edition 1.8 - 1.12.2
- **Compiler Requirements**: C++17 capable compiler (Clang 7.0+, GCC 7.4+, or VS 2017+)

## Repository Structure

### Root Directory Files
```
cuberite/
├── CMakeLists.txt          # Main build configuration
├── README.md               # Project overview and installation
├── COMPILING.md           # Detailed compilation instructions
├── CONTRIBUTING.md        # Contribution guidelines
├── GETTING-STARTED.md     # Developer getting started guide
├── TESTING.md             # Testing guidelines
├── LICENSE                # Apache License V2
├── compile.sh             # Automated compilation script
├── easyinstall.sh         # Installation script
└── nightlybuild.sh        # Nightly build automation
```

### Build Configuration
```
CMake/                      # Build system utilities
├── AddDependencies.cmake   # Dependency management
├── Fixups.cmake           # Platform-specific fixes
├── GenerateBindings.cmake  # Lua binding generation
├── GroupSources.cmake     # Source file organization
└── StampBuild.cmake       # Build versioning
```

### Source Code (`src/`)

#### Core Components
```
src/
├── main.cpp               # Server entry point
├── Root.cpp/.h            # Root server management
├── Server.cpp/.h          # Server core functionality
├── World.cpp/.h           # World management
├── Globals.cpp/.h         # Global definitions and utilities
├── Defines.cpp/.h         # Core definitions
├── Chunk.cpp/.h           # Chunk data management
├── ChunkMap.cpp/.h        # Chunk mapping and storage
└── ClientHandle.cpp/.h    # Client connection handling
```

#### Minecraft Protocol Implementation
```
Protocol/
├── Protocol.h                    # Base protocol interface
├── ProtocolRecognizer.cpp/.h     # Protocol version detection
├── Protocol_1_8.cpp/.h          # Minecraft 1.8 protocol
├── Protocol_1_9.cpp/.h          # Minecraft 1.9 protocol
├── Protocol_1_10.cpp/.h         # Minecraft 1.10 protocol
├── Protocol_1_11.cpp/.h         # Minecraft 1.11 protocol
├── Protocol_1_12.cpp/.h         # Minecraft 1.12 protocol
├── Protocol_1_13.cpp/.h         # Minecraft 1.13 protocol
├── Protocol_1_14.cpp/.h         # Minecraft 1.14 protocol
├── Packetizer.cpp/.h            # Packet serialization
├── ChunkDataSerializer.cpp/.h    # Chunk data encoding
├── Authenticator.cpp/.h         # Player authentication
└── MojangAPI.cpp/.h             # Mojang API integration
```

#### Block System
```
Blocks/
├── BlockHandler.cpp/.h      # Base block handler
├── BlockAir.h              # Air block implementation
├── BlockBed.cpp/.h         # Bed block mechanics
├── BlockButton.h           # Button interactions
├── BlockDoor.cpp/.h        # Door mechanics
├── BlockFire.h             # Fire spread logic
├── BlockFluid.h            # Fluid mechanics
├── BlockHandler.cpp/.h     # Block type registry
├── BlockPiston.cpp/.h      # Piston mechanics
├── BlockRedstoneWire.h     # Redstone circuit logic
└── [50+ specialized block handlers]
```

#### Entity System
```
Entities/
├── Entity.cpp/.h               # Base entity class
├── Player.cpp/.h              # Player entity
├── Pawn.cpp/.h                # Moving entity base
├── Pickup.cpp/.h              # Item pickups
├── Minecart.cpp/.h            # Minecart vehicles
├── ProjectileEntity.cpp/.h     # Projectile base
├── ArrowEntity.cpp/.h         # Arrow projectiles
├── TNTEntity.cpp/.h           # TNT explosives
├── ExpOrb.cpp/.h              # Experience orbs
└── [20+ entity implementations]
```

#### Monster AI System
```
Mobs/
├── Monster.cpp/.h              # Base monster class
├── AggressiveMonster.cpp/.h    # Hostile mob base
├── PassiveMonster.cpp/.h       # Peaceful mob base
├── PathFinder.cpp/.h           # AI pathfinding
├── Path.cpp/.h                 # Path representation
├── Zombie.cpp/.h               # Zombie implementation
├── Skeleton.cpp/.h             # Skeleton implementation
├── Creeper.cpp/.h              # Creeper behavior
├── Spider.cpp/.h               # Spider AI
└── [30+ mob implementations]
```

#### Block Entities
```
BlockEntities/
├── BlockEntity.cpp/.h          # Base block entity
├── ChestEntity.cpp/.h          # Chest storage
├── FurnaceEntity.cpp/.h        # Furnace mechanics
├── SignEntity.cpp/.h           # Sign text storage
├── BrewingstandEntity.cpp/.h   # Brewing mechanics
├── BeaconEntity.cpp/.h         # Beacon effects
├── HopperEntity.cpp/.h         # Item transport
└── [15+ block entity types]
```

#### World Generation
```
Generating/
├── ChunkGenerator.cpp/.h       # Base generator interface
├── ComposableGenerator.cpp/.h  # Modular generation system
├── BioGen.cpp/.h              # Biome generation
├── HeiGen.cpp/.h              # Height map generation
├── CompoGen.cpp/.h            # Terrain composition
├── FinishGen.cpp/.h           # Structure finishing
├── Caves.cpp/.h               # Cave generation
├── Ravines.cpp/.h             # Ravine generation
├── Trees.cpp/.h               # Tree generation
├── VillageGen.cpp/.h          # Village generation
└── MineShafts.cpp/.h          # Mine shaft generation
```

#### Simulation Systems
```
Simulator/
├── Simulator.cpp/.h            # Base simulator
├── FluidSimulator.cpp/.h       # Fluid mechanics
├── FireSimulator.cpp/.h        # Fire spread
├── SandSimulator.cpp/.h        # Falling blocks
├── DelayedFluidSimulator.cpp/.h # Advanced fluid physics
└── RedstoneSimulator.h         # Redstone circuits
```

#### Lua Plugin System
```
Bindings/
├── PluginManager.cpp/.h        # Plugin lifecycle management
├── Plugin.cpp/.h               # Base plugin interface
├── PluginLua.cpp/.h            # Lua plugin implementation
├── LuaState.cpp/.h             # Lua VM management
├── ManualBindings.cpp/.h       # C++ to Lua bindings
├── BlockState.cpp/.h           # Block state API
└── [Multiple Lua binding files]
```

#### User Interface System
```
UI/
├── Window.cpp/.h               # Base window class
├── InventoryWindow.cpp/.h      # Player inventory
├── ChestWindow.cpp/.h          # Chest interface
├── FurnaceWindow.cpp/.h        # Furnace interface
├── CraftingWindow.cpp/.h       # Crafting table
├── AnvilWindow.cpp/.h          # Anvil interface
└── [10+ window implementations]
```

#### Networking and OS Support
```
OSSupport/
├── Network.h                   # Network abstraction
├── TCPLinkImpl.cpp/.h         # TCP connections
├── UDPEndpointImpl.cpp/.h     # UDP endpoints
├── File.cpp/.h                # File operations
├── CriticalSection.cpp/.h     # Thread synchronization
├── Event.cpp/.h               # Event handling
└── StackTrace.cpp/.h          # Debugging support

HTTP/
├── HTTPServer.cpp/.h          # Web admin server
├── HTTPMessage.cpp/.h         # HTTP protocol
├── UrlClient.cpp/.h           # HTTP client
└── WebAdmin.cpp/.h            # Web administration
```

#### Cryptography
```
mbedTLS++/
├── AesCfb128Encryptor.cpp/.h   # AES encryption
├── Sha1Checksum.cpp/.h         # SHA1 hashing
├── RsaPrivateKey.cpp/.h        # RSA key handling
├── SslContext.cpp/.h           # SSL/TLS support
└── X509Cert.cpp/.h             # Certificate handling
```

#### Game Mechanics
```
src/
├── Inventory.cpp/.h            # Item management
├── Item.cpp/.h                 # Item definitions
├── ItemGrid.cpp/.h             # Grid-based storage
├── CraftingRecipes.cpp/.h      # Crafting system
├── FurnaceRecipe.cpp/.h        # Smelting recipes
├── BrewingRecipes.cpp/.h       # Potion brewing
├── Enchantments.cpp/.h         # Enchantment system
├── Scoreboard.cpp/.h           # Scoreboard system
└── StatisticsManager.cpp/.h    # Player statistics
```

### Server Configuration (`Server/`)
```
Server/
├── README.txt                  # Server setup guide
├── crafting.txt               # Crafting recipes
├── furnace.txt                # Smelting recipes
├── brewing.txt                # Brewing recipes
├── items.ini                  # Item definitions
├── monsters.ini               # Monster configuration
├── webadmin/                  # Web administration
│   ├── template.lua          # Web template engine
│   └── login_template.html   # Login page
└── Plugins/                   # Default plugins
    ├── InfoDump.lua          # Server information
    └── InfoReg.lua           # Command registration
```

### External Dependencies (`lib/`)
```
lib/
├── SQLiteCpp/                 # SQLite C++ wrapper
├── TCLAP/                     # Command line parsing
├── jsoncpp/                   # JSON parsing
├── libdeflate/                # Compression library
├── libevent/                  # Event handling
├── lua/                       # Lua scripting engine
├── mbedtls/                   # Cryptography library
├── sqlite/                    # Database engine
└── tolua++/                   # C++ to Lua binding generator
```

### Development Tools (`Tools/`)
```
Tools/
└── AnvilStats/                # World analysis tool
    ├── AnvilStats.cpp         # Main analysis code
    ├── BiomeMap.cpp/.h        # Biome visualization
    ├── HeightMap.cpp/.h       # Height map generation
    └── ChunkExtract.cpp/.h    # Chunk data extraction
```

### Testing Framework (`tests/`)
```
tests/
├── BlockTypeRegistry/         # Block system tests
├── BoundingBox/              # Collision detection tests
├── ByteBuffer/               # Data serialization tests
├── ChunkData/                # Chunk management tests
├── CompositeChat/            # Chat system tests
├── Generating/               # World generation tests
├── HTTP/                     # Web server tests
├── Network/                  # Networking tests
└── SchematicFileSerializer/  # Schematic import tests
```

### Documentation (`dev-docs/`)
```
dev-docs/
├── Generator.html            # World generation documentation
├── Plugin API.md             # Plugin development guide
├── ExportingAPI.html         # API export documentation
├── Login sequence.txt        # Authentication flow
├── Object ownership.gv       # Memory management diagram
└── img/                      # Documentation images
```

## Build System Features

### CMake Configuration
- Multi-platform build support (Windows, Linux, macOS, FreeBSD, Android)
- Multiple build types (Debug, Release, RelWithDebInfo)
- Conditional compilation for platform-specific features
- Automated dependency management
- Unity builds for faster compilation
- Precompiled headers support
- Link-time optimization (LTO)

### Build Flags
- `BUILD_TOOLS`: Include development tools
- `SELF_TEST`: Enable unit testing
- `FORCE_32`: Force 32-bit builds
- `NO_NATIVE_OPTIMIZATION`: Cross-compilation support
- `UNITY_BUILDS`: Faster compilation
- `PRECOMPILE_HEADERS`: Compilation optimization
- `WHOLE_PROGRAM_OPTIMISATION`: Link-time optimization

## Architecture Highlights

### Plugin System
- Lua-based plugin architecture
- Hot-reloading capability
- Extensive API bindings
- Event-driven architecture
- Secure sandboxing

### Performance Features
- Efficient chunk management
- Multi-threaded architecture
- Memory-optimized data structures
- Fast protocol implementation
- Optimized world generation

### Protocol Implementation
- Multiple protocol version support
- Packet compression
- Encryption support
- Authentication integration
- Mojang API compatibility

### Cross-Platform Support
- Platform-specific optimizations
- Unified build system
- Conditional compilation
- Mobile platform support (Android)
- Embedded device compatibility

## Comparison with ParellelStone

While both are C++ Minecraft server implementations, they have different approaches:

**Cuberite:**
- Mature, stable codebase with extensive features
- Lua plugin system for extensibility
- Multiple protocol version support (1.8-1.12.2)
- Comprehensive world generation
- Full game mechanics implementation

**ParellelStone:**
- Modern C++20 implementation
- Protocol 765 (1.20.4) support
- More recent protocol features
- Focused architecture with clear separation

Both projects demonstrate different architectural approaches to implementing Minecraft server functionality in C++.