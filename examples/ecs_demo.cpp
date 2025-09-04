/**
 * @file ecs_demo.cpp
 * @brief Demonstration of ParallelStone's ECS-based block system
 * 
 * This demo shows how blocks, entities, and players are managed using
 * the Entity Component System architecture, providing:
 * - Component-based block representation
 * - System-driven game logic
 * - Flexible entity composition
 * - High-performance spatial queries
 */

#include "ecs/core.hpp"
#include "ecs/world_ecs.hpp"
#include "world/block_type.hpp"
#include <iostream>
#include <chrono>

using namespace parallelstone::ecs;
using namespace parallelstone::world;
using namespace parallelstone::utils;

void demonstrate_ecs_basics() {
    std::cout << "=== ECS Core Demonstration ===\n";
    
    Registry registry;
    
    // Register component types
    registry.register_component<Position>();
    registry.register_component<Block>();
    registry.register_component<Physics>();
    registry.register_component<Lighting>();
    registry.register_component<Player>();
    registry.register_component<EntityData>();
    
    std::cout << "Registered ECS components\n";
    
    // Create some entities
    Entity stone_block = registry.create();
    Entity grass_block = registry.create();
    Entity player_entity = registry.create();
    
    std::cout << "Created entities: " << stone_block << ", " << grass_block << ", " << player_entity << "\n";
    
    // Add components to stone block
    registry.emplace<Position>(stone_block, 10.0, 64.0, 15.0);
    registry.emplace<Block>(stone_block, BlockType::STONE);
    registry.emplace<Physics>(stone_block, Physics{
        .solid = true,
        .transparent = false,
        .hardness = 1.5f,
        .blast_resistance = 6.0f
    });
    
    // Add components to grass block
    registry.emplace<Position>(grass_block, 11.0, 64.0, 15.0);
    registry.emplace<Block>(grass_block, BlockType::GRASS_BLOCK);
    registry.emplace<Physics>(grass_block, Physics{
        .solid = true,
        .transparent = false,
        .hardness = 0.6f,
        .blast_resistance = 0.6f
    });
    registry.emplace<Lighting>(grass_block, Lighting{
        .current_light = 15,
        .sky_light = 15
    });
    
    // Add components to player
    registry.emplace<Position>(player_entity, 10.5, 66.0, 15.5);
    registry.emplace<Player>(player_entity, Player{
        .username = "TestPlayer",
        .uuid = generate_uuid("TestPlayer"),
        .health = 20.0f,
        .hunger = 20.0f
    });
    registry.emplace<EntityData>(player_entity, EntityData{
        .width = 0.6f,
        .height = 1.8f
    });
    
    std::cout << "Added components to entities\n";
    
    // Query entities with specific components
    auto block_view = registry.view<Position, Block>();
    std::cout << "Blocks in world:\n";
    for (auto entity : block_view) {
        auto& pos = block_view.get<Position>(entity);
        auto& block = block_view.get<Block>(entity);
        
        std::cout << "  Entity " << entity << ": " 
                  << static_cast<int>(block.type) << " at ("
                  << pos.world_pos.x << ", " << pos.world_pos.y << ", " << pos.world_pos.z << ")\n";
    }
    
    auto player_view = registry.view<Position, Player>();
    std::cout << "Players in world:\n";
    for (auto entity : player_view) {
        auto& pos = player_view.get<Position>(entity);
        auto& player = player_view.get<Player>(entity);
        
        std::cout << "  Player " << player.username << " (" << player.uuid << ") at ("
                  << pos.world_pos.x << ", " << pos.world_pos.y << ", " << pos.world_pos.z << ")\n";
    }
    
    std::cout << "Living entities: " << registry.get_living_entity_count() << "\n\n";
}

void demonstrate_block_system() {
    std::cout << "=== Block System Demonstration ===\n";
    
    Registry registry;
    
    // Register all required components
    registry.register_component<Position>();
    registry.register_component<Block>();
    registry.register_component<Physics>();
    registry.register_component<Lighting>();
    registry.register_component<RandomTick>();
    registry.register_component<ChunkRef>();
    
    // Add block system
    auto& block_system = registry.add_system<BlockSystem>();
    
    std::cout << "Initialized Block System\n";
    
    // Create some blocks using the block system
    Position stone_pos(5, 64, 8);
    Position grass_pos(6, 64, 8);
    Position air_pos(7, 64, 8);
    
    Block stone_block(BlockType::STONE);
    Block grass_block(BlockType::GRASS_BLOCK);
    
    Entity stone_entity = block_system.create_block(registry, stone_pos, stone_block);
    Entity grass_entity = block_system.create_block(registry, grass_pos, grass_block);
    
    std::cout << "Created blocks: Stone=" << stone_entity << ", Grass=" << grass_entity << "\n";
    
    // Query blocks
    auto found_stone = block_system.get_block(registry, stone_pos);
    auto found_grass = block_system.get_block(registry, grass_pos);
    auto found_air = block_system.get_block(registry, air_pos);
    
    std::cout << "Block queries:\n";
    std::cout << "  Stone at (5,64,8): " << (found_stone ? "Found" : "Not found") << "\n";
    std::cout << "  Grass at (6,64,8): " << (found_grass ? "Found" : "Not found") << "\n";
    std::cout << "  Air at (7,64,8): " << (found_air ? "Found" : "Not found") << "\n";
    
    // Test block properties
    if (found_stone) {
        bool is_solid = block_system.is_solid(registry, *found_stone);
        bool is_transparent = block_system.is_transparent(registry, *found_stone);
        BlockType type = block_system.get_block_type(registry, *found_stone);
        
        std::cout << "Stone properties: solid=" << is_solid 
                  << ", transparent=" << is_transparent 
                  << ", type=" << static_cast<int>(type) << "\n";
    }
    
    // Place a new block
    bool placed = block_system.set_block(registry, air_pos, BlockType::OAK_LOG);
    std::cout << "Placed oak log: " << (placed ? "Success" : "Failed") << "\n";
    
    // Verify the new block
    auto found_log = block_system.get_block(registry, air_pos);
    if (found_log) {
        BlockType log_type = block_system.get_block_type(registry, *found_log);
        std::cout << "Log type: " << static_cast<int>(log_type) << "\n";
    }
    
    std::cout << "\n";
}

void demonstrate_lighting_system() {
    std::cout << "=== Lighting System Demonstration ===\n";
    
    Registry registry;
    
    // Register components
    registry.register_component<Position>();
    registry.register_component<Block>();
    registry.register_component<Physics>();
    registry.register_component<Lighting>();
    
    // Add systems
    auto& block_system = registry.add_system<BlockSystem>();
    auto& lighting_system = registry.add_system<LightingSystem>();
    
    std::cout << "Initialized Lighting System\n";
    
    // Create some blocks with lighting
    Position torch_pos(10, 65, 10);
    Position nearby_pos(11, 65, 10);
    
    // Create a torch (light source)
    Entity torch = registry.create();
    registry.emplace<Position>(torch, torch_pos);
    registry.emplace<Block>(torch, BlockType::AIR);  // Simplified - would be torch
    registry.emplace<Lighting>(torch, Lighting{
        .light_emission = 14,
        .current_light = 14,
        .needs_update = true
    });
    
    // Create a nearby block
    Entity nearby_block = block_system.create_block(registry, nearby_pos, Block(BlockType::STONE));
    if (registry.has<Lighting>(nearby_block)) {
        registry.get<Lighting>(nearby_block).needs_update = true;
    }
    
    std::cout << "Created torch and nearby block\n";
    
    // Update lighting system
    lighting_system.update(registry, 0.016f);  // Simulate one frame
    
    // Check lighting results
    auto& torch_lighting = registry.get<Lighting>(torch);
    std::cout << "Torch light level: " << static_cast<int>(torch_lighting.current_light) << "\n";
    
    if (registry.has<Lighting>(nearby_block)) {
        auto& nearby_lighting = registry.get<Lighting>(nearby_block);
        std::cout << "Nearby block light level: " << static_cast<int>(nearby_lighting.current_light) << "\n";
    }
    
    // Test light propagation
    lighting_system.propagate_light(registry, torch);
    std::cout << "Propagated light from torch\n";
    
    std::cout << "\n";
}

void demonstrate_physics_system() {
    std::cout << "=== Physics System Demonstration ===\n";
    
    Registry registry;
    
    // Register components
    registry.register_component<Position>();
    registry.register_component<Block>();
    registry.register_component<Physics>();
    registry.register_component<EntityData>();
    registry.register_component<Player>();
    
    // Add systems
    auto& physics_system = registry.add_system<PhysicsSystem>();
    
    std::cout << "Initialized Physics System\n";
    
    // Create a falling player
    Entity player = registry.create();
    registry.emplace<Position>(player, 0.0, 100.0, 0.0);  // High in the air
    registry.emplace<Player>(player, Player{
        .username = "FallingPlayer",
        .health = 20.0f
    });
    registry.emplace<EntityData>(player, EntityData{
        .velocity = Vector3d(0, 0, 0),
        .width = 0.6f,
        .height = 1.8f
    });
    registry.emplace<Physics>(player, Physics{
        .solid = true,
        .affected_by_gravity = true
    });
    
    std::cout << "Created falling player at height 100\n";
    
    // Simulate physics for a few frames
    float delta_time = 0.016f;  // 60 FPS
    
    for (int frame = 0; frame < 10; ++frame) {
        physics_system.update(registry, delta_time);
        
        auto& pos = registry.get<Position>(player);
        auto& entity_data = registry.get<EntityData>(player);
        
        if (frame % 3 == 0) {  // Print every 3rd frame
            std::cout << "Frame " << frame << ": Player at height " 
                      << pos.world_pos.y << ", velocity=" << entity_data.velocity.y 
                      << ", on_ground=" << entity_data.on_ground << "\n";
        }
        
        if (entity_data.on_ground) {
            std::cout << "Player hit the ground!\n";
            break;
        }
    }
    
    std::cout << "\n";
}

void demonstrate_chunk_system() {
    std::cout << "=== Chunk System Demonstration ===\n";
    
    Registry registry;
    
    // Register components
    registry.register_component<Position>();
    registry.register_component<Block>();
    registry.register_component<Physics>();
    registry.register_component<ChunkRef>();
    
    // Add systems
    auto& chunk_system = registry.add_system<ChunkSystem>();
    auto& block_system = registry.add_system<BlockSystem>();
    
    std::cout << "Initialized Chunk System\n";
    
    // Load some chunks
    chunk_system.load_chunk(registry, 0, 0);
    chunk_system.load_chunk(registry, 1, 0);
    chunk_system.load_chunk(registry, 0, 1);
    
    std::cout << "Loaded 3 chunks\n";
    
    // Count blocks in each chunk
    auto blocks_00 = chunk_system.get_blocks_in_chunk(registry, 0, 0);
    auto blocks_10 = chunk_system.get_blocks_in_chunk(registry, 1, 0);
    auto blocks_01 = chunk_system.get_blocks_in_chunk(registry, 0, 1);
    
    std::cout << "Block counts:\n";
    std::cout << "  Chunk (0,0): " << blocks_00.size() << " blocks\n";
    std::cout << "  Chunk (1,0): " << blocks_10.size() << " blocks\n";
    std::cout << "  Chunk (0,1): " << blocks_01.size() << " blocks\n";
    
    // Show some block positions
    if (!blocks_00.empty()) {
        std::cout << "Sample blocks from chunk (0,0):\n";
        for (size_t i = 0; i < std::min(size_t(5), blocks_00.size()); ++i) {
            auto& pos = registry.get<Position>(blocks_00[i]);
            auto& block = registry.get<Block>(blocks_00[i]);
            
            std::cout << "  Block " << static_cast<int>(block.type) 
                      << " at (" << pos.world_pos.x << ", " << pos.world_pos.y << ", " << pos.world_pos.z << ")\n";
        }
    }
    
    // Unload a chunk
    chunk_system.unload_chunk(registry, 1, 0);
    std::cout << "Unloaded chunk (1,0)\n";
    
    // Verify blocks are gone
    auto blocks_10_after = chunk_system.get_blocks_in_chunk(registry, 1, 0);
    std::cout << "Chunk (1,0) now has " << blocks_10_after.size() << " blocks\n";
    
    std::cout << "\n";
}

void demonstrate_player_interaction() {
    std::cout << "=== Player Interaction Demonstration ===\n";
    
    Registry registry;
    
    // Register all components
    registry.register_component<Position>();
    registry.register_component<Block>();
    registry.register_component<Physics>();
    registry.register_component<Player>();
    registry.register_component<EntityData>();
    registry.register_component<Interactable>();
    
    // Add systems
    auto& block_system = registry.add_system<BlockSystem>();
    auto& interaction_system = registry.add_system<InteractionSystem>();
    
    std::cout << "Initialized Interaction System\n";
    
    // Create a player
    Entity player = create_player_entity(registry, "TestPlayer", Position(0, 65, 0));
    std::cout << "Created player: " << registry.get<Player>(player).username << "\n";
    
    // Create some blocks to interact with
    Position block_pos(1, 64, 0);
    Entity stone_block = block_system.create_block(registry, block_pos, Block(BlockType::STONE));
    
    std::cout << "Created stone block at (1, 64, 0)\n";
    
    // Player breaks the block
    interaction_system.handle_block_break(registry, player, stone_block);
    
    // Verify block is gone
    auto found_block = block_system.get_block(registry, block_pos);
    std::cout << "Block after breaking: " << (found_block ? "Still exists" : "Destroyed") << "\n";
    
    // Player places a new block
    Position new_block_pos(2, 64, 0);
    interaction_system.handle_block_place(registry, player, new_block_pos, BlockType::OAK_LOG);
    
    // Verify new block exists
    auto new_block = block_system.get_block(registry, new_block_pos);
    if (new_block) {
        BlockType type = block_system.get_block_type(registry, *new_block);
        std::cout << "Placed block type: " << static_cast<int>(type) << "\n";
    }
    
    std::cout << "\n";
}

void demonstrate_performance() {
    std::cout << "=== ECS Performance Demonstration ===\n";
    
    Registry registry;
    
    // Register components
    registry.register_component<Position>();
    registry.register_component<Block>();
    registry.register_component<Physics>();
    
    auto& block_system = registry.add_system<BlockSystem>();
    
    std::cout << "Setting up performance test...\n";
    
    const int BLOCK_COUNT = 10000;
    std::vector<Entity> blocks;
    blocks.reserve(BLOCK_COUNT);
    
    // Measure block creation time
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < BLOCK_COUNT; ++i) {
        int x = i % 100;
        int y = 64 + (i / 10000);
        int z = (i / 100) % 100;
        
        Position pos(x, y, z);
        BlockType type = (i % 2 == 0) ? BlockType::STONE : BlockType::GRASS_BLOCK;
        Block block(type);
        
        Entity entity = block_system.create_block(registry, pos, block);
        blocks.push_back(entity);
    }
    
    auto mid = std::chrono::high_resolution_clock::now();
    
    // Measure query time
    int solid_count = 0;
    for (Entity entity : blocks) {
        if (block_system.is_solid(registry, entity)) {
            solid_count++;
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    
    auto creation_time = std::chrono::duration_cast<std::chrono::microseconds>(mid - start);
    auto query_time = std::chrono::duration_cast<std::chrono::microseconds>(end - mid);
    
    std::cout << "Created " << BLOCK_COUNT << " blocks in " << creation_time.count() << " μs\n";
    std::cout << "Queried " << BLOCK_COUNT << " blocks in " << query_time.count() << " μs\n";
    std::cout << "Creation rate: " << (BLOCK_COUNT * 1000000.0 / creation_time.count()) << " blocks/sec\n";
    std::cout << "Query rate: " << (BLOCK_COUNT * 1000000.0 / query_time.count()) << " queries/sec\n";
    std::cout << "Found " << solid_count << " solid blocks\n";
    std::cout << "Total entities: " << registry.get_living_entity_count() << "\n";
    
    std::cout << "\n";
}

int main() {
    std::cout << "ParallelStone ECS Block System Demonstration\n";
    std::cout << "===========================================\n\n";
    
    try {
        demonstrate_ecs_basics();
        demonstrate_block_system();
        demonstrate_lighting_system();
        demonstrate_physics_system();
        demonstrate_chunk_system();
        demonstrate_player_interaction();
        demonstrate_performance();
        
        std::cout << "All ECS demonstrations completed successfully!\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}