/**
 * @file block_system_demo.cpp
 * @brief Demonstration of ParallelStone's modern block system
 * 
 * This file showcases the complete block system implementation,
 * including block types, states, chunk storage, and handlers.
 * 
 * Based on analysis of Cuberite's block system but modernized with:
 * - C++20 features and strong typing
 * - Efficient sparse chunk storage
 * - Type-safe block state management
 * - Performance-optimized data structures
 */

#include "world/block_type.hpp"
#include "world/block_state.hpp"
#include "world/chunk_section.hpp"
#include "world/block_handler.hpp"
#include <iostream>
#include <chrono>
#include <vector>

using namespace parallelstone::world;
using namespace parallelstone::utils;

void demonstrate_block_types() {
    std::cout << "=== Block Type System Demo ===\n";
    
    // Modern enum-based block types
    BlockType stone = BlockType::STONE;
    BlockType grass = BlockType::GRASS_BLOCK;
    BlockType oak_log = BlockType::OAK_LOG;
    
    // Type-safe operations
    std::cout << "Stone hardness: " << BlockRegistry::get_properties(stone).hardness << "\n";
    std::cout << "Grass is transparent: " << BlockRegistry::get_properties(grass).is_transparent << "\n";
    std::cout << "Oak log is flammable: " << BlockRegistry::get_properties(oak_log).is_flammable << "\n";
    
    // Name lookup
    std::cout << "Stone name: " << BlockRegistry::get_name(stone) << "\n";
    
    // Reverse lookup
    BlockType looked_up = BlockRegistry::from_name("minecraft:grass_block");
    std::cout << "Looked up grass: " << (looked_up == grass ? "Success" : "Failed") << "\n";
    
    // Network protocol integration
    std::uint16_t network_id = BlockRegistry::get_network_id(stone);
    BlockType from_network = BlockRegistry::from_network_id(network_id);
    std::cout << "Network round-trip: " << (from_network == stone ? "Success" : "Failed") << "\n";
    
    std::cout << "\n";
}

void demonstrate_block_states() {
    std::cout << "=== Block State System Demo ===\n";
    
    // Simple block state (no properties)
    BlockState simple_stone = BlockState::default_state(BlockType::STONE);
    std::cout << "Simple stone: " << simple_stone.to_string() << "\n";
    
    // Complex block state with properties
    BlockState oak_door = BlockState::builder(BlockType::OAK_DOOR)
        .with("facing", "north")
        .with("open", false)
        .with("hinge", "left")
        .with("half", "lower")
        .build();
    
    std::cout << "Oak door: " << oak_door.to_string() << "\n";
    
    // Property access
    auto facing = oak_door.get_string("facing");
    auto is_open = oak_door.get_bool("open");
    std::cout << "Door facing: " << (facing ? *facing : "unknown") << "\n";
    std::cout << "Door open: " << (is_open ? (*is_open ? "yes" : "no") : "unknown") << "\n";
    
    // State modification (creates new state)
    BlockState opened_door = oak_door.with_property("open", PropertyValue(true));
    std::cout << "Opened door: " << opened_door.to_string() << "\n";
    
    // Fast equality comparison
    bool same = (oak_door == opened_door);
    std::cout << "States equal: " << (same ? "yes" : "no") << "\n";
    
    // Log with axis property
    BlockState log_x = BlockState::builder(BlockType::OAK_LOG)
        .with("axis", "x")
        .build();
    std::cout << "Oak log (X axis): " << log_x.to_string() << "\n";
    
    // Water with level
    BlockState flowing_water = BlockState::builder(BlockType::WATER)
        .with("level", 3)
        .build();
    std::cout << "Flowing water: " << flowing_water.to_string() << "\n";
    
    std::cout << "\n";
}

void demonstrate_chunk_storage() {
    std::cout << "=== Chunk Storage System Demo ===\n";
    
    // Create a chunk at coordinates (0, 0)
    Chunk chunk(0, 0);
    std::cout << "Created chunk at (" << chunk.chunk_x() << ", " << chunk.chunk_z() << ")\n";
    std::cout << "Chunk is empty: " << (chunk.is_empty() ? "yes" : "no") << "\n";
    
    // Set some blocks
    chunk.set_block(8, 64, 8, BlockState::default_state(BlockType::GRASS_BLOCK));
    chunk.set_block(8, 63, 8, BlockState::default_state(BlockType::DIRT));
    chunk.set_block(8, 62, 8, BlockState::default_state(BlockType::STONE));
    
    // Create a log with orientation
    BlockState vertical_log = BlockState::builder(BlockType::OAK_LOG)
        .with("axis", "y")
        .build();
    
    chunk.set_block(10, 64, 10, vertical_log);
    chunk.set_block(10, 65, 10, vertical_log);
    chunk.set_block(10, 66, 10, vertical_log);
    
    std::cout << "Set blocks in chunk\n";
    std::cout << "Chunk is empty: " << (chunk.is_empty() ? "yes" : "no") << "\n";
    
    // Read blocks back
    BlockState grass = chunk.get_block(8, 64, 8);
    BlockState dirt = chunk.get_block(8, 63, 8);
    BlockState log = chunk.get_block(10, 65, 10);
    
    std::cout << "Block at (8,64,8): " << grass.to_string() << "\n";
    std::cout << "Block at (8,63,8): " << dirt.to_string() << "\n";
    std::cout << "Block at (10,65,10): " << log.to_string() << "\n";
    
    // Check heightmap
    std::int32_t height_at_8_8 = chunk.get_height(8, 8);
    std::int32_t height_at_10_10 = chunk.get_height(10, 10);
    std::cout << "Height at (8,8): " << height_at_8_8 << "\n";
    std::cout << "Height at (10,10): " << height_at_10_10 << "\n";
    
    // Demonstrate section access
    ChunkSection* section = chunk.get_section(Chunk::y_to_section_index(64));
    if (section) {
        std::cout << "Section has " << section->non_air_count() << " non-air blocks\n";
        std::cout << "Section has lighting: " << (section->has_lighting() ? "yes" : "no") << "\n";
    }
    
    std::cout << "\n";
}

void demonstrate_performance() {
    std::cout << "=== Performance Demo ===\n";
    
    Chunk chunk(0, 0);
    
    // Measure block setting performance
    auto start = std::chrono::high_resolution_clock::now();
    
    constexpr int ITERATIONS = 100000;
    
    // Set many blocks
    for (int i = 0; i < ITERATIONS; ++i) {
        std::uint8_t x = i % 16;
        std::uint8_t z = (i / 16) % 16;
        std::int32_t y = 64 + (i / 256) % 16;
        
        BlockType type = (i % 2 == 0) ? BlockType::STONE : BlockType::DIRT;
        chunk.set_block(x, y, z, BlockState::default_state(type));
    }
    
    auto middle = std::chrono::high_resolution_clock::now();
    
    // Read many blocks
    std::uint32_t checksum = 0;
    for (int i = 0; i < ITERATIONS; ++i) {
        std::uint8_t x = i % 16;
        std::uint8_t z = (i / 16) % 16;
        std::int32_t y = 64 + (i / 256) % 16;
        
        BlockState state = chunk.get_block(x, y, z);
        checksum += static_cast<std::uint32_t>(state.type());
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    
    auto set_time = std::chrono::duration_cast<std::chrono::microseconds>(middle - start);
    auto get_time = std::chrono::duration_cast<std::chrono::microseconds>(end - middle);
    
    std::cout << "Set " << ITERATIONS << " blocks in " << set_time.count() << " μs\n";
    std::cout << "Get " << ITERATIONS << " blocks in " << get_time.count() << " μs\n";
    std::cout << "Set rate: " << (ITERATIONS * 1000000.0 / set_time.count()) << " blocks/sec\n";
    std::cout << "Get rate: " << (ITERATIONS * 1000000.0 / get_time.count()) << " blocks/sec\n";
    std::cout << "Checksum: " << checksum << " (prevents optimization)\n";
    
    std::cout << "\n";
}

void demonstrate_state_registry() {
    std::cout << "=== Block State Registry Demo ===\n";
    
    // Initialize the registry with default states
    BlockStateRegistry::initialize_defaults();
    
    // Create some block states
    BlockState air = BlockState::default_state(BlockType::AIR);
    BlockState stone = BlockState::default_state(BlockType::STONE);
    BlockState grass = BlockState::default_state(BlockType::GRASS_BLOCK);
    
    // Get protocol IDs
    std::uint32_t air_id = air.get_protocol_id();
    std::uint32_t stone_id = stone.get_protocol_id();
    std::uint32_t grass_id = grass.get_protocol_id();
    
    std::cout << "Air protocol ID: " << air_id << "\n";
    std::cout << "Stone protocol ID: " << stone_id << "\n";
    std::cout << "Grass protocol ID: " << grass_id << "\n";
    
    // Reverse lookup
    auto air_from_id = BlockStateRegistry::from_protocol_id(air_id);
    auto stone_from_id = BlockStateRegistry::from_protocol_id(stone_id);
    
    std::cout << "Air round-trip: " << (air_from_id && *air_from_id == air ? "Success" : "Failed") << "\n";
    std::cout << "Stone round-trip: " << (stone_from_id && *stone_from_id == stone ? "Success" : "Failed") << "\n";
    
    // Complex state with properties
    BlockState log_x = BlockState::builder(BlockType::OAK_LOG)
        .with("axis", "x")
        .build();
    
    std::uint32_t log_id = log_x.get_protocol_id();
    auto log_from_id = BlockStateRegistry::from_protocol_id(log_id);
    
    std::cout << "Log X protocol ID: " << log_id << "\n";
    std::cout << "Log round-trip: " << (log_from_id && *log_from_id == log_x ? "Success" : "Failed") << "\n";
    
    std::cout << "\n";
}

void demonstrate_utility_functions() {
    std::cout << "=== Utility Functions Demo ===\n";
    
    // Block category checks
    std::cout << "Stone is ore: " << (block_utils::is_ore(BlockType::STONE) ? "yes" : "no") << "\n";
    std::cout << "Iron ore is ore: " << (block_utils::is_ore(BlockType::IRON_ORE) ? "yes" : "no") << "\n";
    std::cout << "Oak log is log: " << (block_utils::is_log(BlockType::OAK_LOG) ? "yes" : "no") << "\n";
    std::cout << "Water is liquid: " << (block_utils::is_liquid(BlockType::WATER) ? "yes" : "no") << "\n";
    std::cout << "Air is air: " << (block_utils::is_air(BlockType::AIR) ? "yes" : "no") << "\n";
    std::cout << "Water is replaceable: " << (block_utils::is_replaceable(BlockType::WATER) ? "yes" : "no") << "\n";
    
    // State utility functions
    BlockState north_stairs = state_utils::facing_block(BlockType::OAK_STAIRS, "north");
    BlockState bottom_slab = state_utils::slab_block(BlockType::OAK_SLAB, "bottom");
    BlockState waterlogged_fence = state_utils::waterlogged_block(BlockType::OAK_FENCE, true);
    
    std::cout << "North stairs: " << north_stairs.to_string() << "\n";
    std::cout << "Bottom slab: " << bottom_slab.to_string() << "\n";
    std::cout << "Waterlogged fence: " << waterlogged_fence.to_string() << "\n";
    
    // Vector3 utilities
    Vector3i pos(10, 64, 15);
    Vector3i above = pos.above();
    Vector3i north = pos.north();
    Vector3i offset = pos.offset(5, -2, 3);
    
    std::cout << "Position: (" << pos.x << "," << pos.y << "," << pos.z << ")\n";
    std::cout << "Above: (" << above.x << "," << above.y << "," << above.z << ")\n";
    std::cout << "North: (" << north.x << "," << north.y << "," << north.z << ")\n";
    std::cout << "Offset: (" << offset.x << "," << offset.y << "," << offset.z << ")\n";
    
    std::cout << "\n";
}

int main() {
    std::cout << "ParallelStone Block System Demonstration\n";
    std::cout << "=========================================\n\n";
    
    try {
        demonstrate_block_types();
        demonstrate_block_states();
        demonstrate_chunk_storage();
        demonstrate_performance();
        demonstrate_state_registry();
        demonstrate_utility_functions();
        
        std::cout << "All demonstrations completed successfully!\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}