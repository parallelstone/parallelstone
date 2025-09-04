#include "ecs/world_ecs.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <random>

namespace parallelstone {
namespace ecs {

// ==================== BLOCK SYSTEM IMPLEMENTATION ====================

void BlockSystem::update(Registry& registry, float delta_time) {
    // Update block index if needed
    update_block_index(registry);
    
    // No continuous updates needed for blocks
    // This system mainly provides block management interface
}

Entity BlockSystem::create_block(Registry& registry, const Position& pos, const Block& block) {
    // Check if block already exists at this position
    utils::Vector3i block_pos{
        static_cast<int>(pos.world_pos.x),
        static_cast<int>(pos.world_pos.y),
        static_cast<int>(pos.world_pos.z)
    };
    
    auto existing = block_index_.find(block_pos);
    if (existing != block_index_.end()) {
        // Update existing block
        if (registry.valid(existing->second)) {
            registry.get<Block>(existing->second) = block;
            return existing->second;
        } else {
            // Clean up invalid entity
            block_index_.erase(existing);
        }
    }
    
    // Create new block entity
    Entity entity = create_block_entity(registry, pos, block.type);
    
    // Update the block component with the provided data
    registry.get<Block>(entity) = block;
    
    // Add to spatial index
    block_index_[block_pos] = entity;
    
    handle_block_placement(registry, entity);
    return entity;
}

bool BlockSystem::destroy_block(Registry& registry, Entity block_entity) {
    if (!registry.valid(block_entity) || !registry.has<Block>(block_entity)) {
        return false;
    }
    
    // Get position for index cleanup
    if (registry.has<Position>(block_entity)) {
        auto& pos = registry.get<Position>(block_entity);
        utils::Vector3i block_pos{
            static_cast<int>(pos.world_pos.x),
            static_cast<int>(pos.world_pos.y),
            static_cast<int>(pos.world_pos.z)
        };
        block_index_.erase(block_pos);
    }
    
    handle_block_destruction(registry, block_entity);
    registry.destroy(block_entity);
    return true;
}

bool BlockSystem::set_block(Registry& registry, const Position& pos, world::BlockType type) {
    Block block(type);
    create_block(registry, pos, block);
    return true;
}

std::optional<Entity> BlockSystem::get_block(Registry& registry, const Position& pos) {
    utils::Vector3i block_pos{
        static_cast<int>(pos.world_pos.x),
        static_cast<int>(pos.world_pos.y),
        static_cast<int>(pos.world_pos.z)
    };
    
    auto it = block_index_.find(block_pos);
    if (it != block_index_.end() && registry.valid(it->second)) {
        return it->second;
    }
    
    return std::nullopt;
}

bool BlockSystem::is_solid(Registry& registry, Entity block_entity) {
    if (!registry.valid(block_entity) || !registry.has<Physics>(block_entity)) {
        return false;
    }
    
    return registry.get<Physics>(block_entity).solid;
}

bool BlockSystem::is_transparent(Registry& registry, Entity block_entity) {
    if (!registry.valid(block_entity) || !registry.has<Physics>(block_entity)) {
        return true;  // Invalid blocks are considered transparent
    }
    
    return registry.get<Physics>(block_entity).transparent;
}

world::BlockType BlockSystem::get_block_type(Registry& registry, Entity block_entity) {
    if (!registry.valid(block_entity) || !registry.has<Block>(block_entity)) {
        return world::BlockType::AIR;
    }
    
    return registry.get<Block>(block_entity).type;
}

void BlockSystem::update_block_index(Registry& registry) {
    // Clean up invalid entities from index
    for (auto it = block_index_.begin(); it != block_index_.end();) {
        if (!registry.valid(it->second)) {
            it = block_index_.erase(it);
        } else {
            ++it;
        }
    }
}

void BlockSystem::handle_block_placement(Registry& registry, Entity entity) {
    // Trigger lighting updates, physics checks, etc.
    if (registry.has<Lighting>(entity)) {
        registry.get<Lighting>(entity).needs_update = true;
    }
}

void BlockSystem::handle_block_destruction(Registry& registry, Entity entity) {
    // Handle block destruction effects
    if (registry.has<Position>(entity)) {
        auto& pos = registry.get<Position>(entity);
        
        // Mark nearby blocks for lighting update
        for (int dx = -1; dx <= 1; ++dx) {
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dz = -1; dz <= 1; ++dz) {
                    if (dx == 0 && dy == 0 && dz == 0) continue;
                    
                    Position nearby_pos(
                        pos.world_pos.x + dx,
                        pos.world_pos.y + dy,
                        pos.world_pos.z + dz
                    );
                    
                    auto nearby_block = get_block(registry, nearby_pos);
                    if (nearby_block && registry.has<Lighting>(*nearby_block)) {
                        registry.get<Lighting>(*nearby_block).needs_update = true;
                    }
                }
            }
        }
    }
}

// ==================== LIGHTING SYSTEM IMPLEMENTATION ====================

void LightingSystem::update(Registry& registry, float delta_time) {
    // Process lighting update queue
    while (!light_update_queue_.empty()) {
        Entity entity = light_update_queue_.front();
        light_update_queue_.pop();
        
        if (registry.valid(entity) && registry.has<Lighting>(entity)) {
            auto& lighting = registry.get<Lighting>(entity);
            if (lighting.needs_update) {
                update_block_light(registry, entity);
                update_sky_light(registry, entity);
                lighting.needs_update = false;
            }
        }
    }
}

void LightingSystem::recalculate_lighting(Registry& registry, const utils::Vector3i& chunk_pos) {
    // Find all blocks in the chunk and mark them for lighting update
    auto view = registry.view<Position, Lighting>();
    
    for (auto entity : view) {
        auto& pos = view.get<Position>(entity);
        if (pos.chunk_pos.x == chunk_pos.x && pos.chunk_pos.z == chunk_pos.z) {
            auto& lighting = view.get<Lighting>(entity);
            lighting.needs_update = true;
            light_update_queue_.push(entity);
        }
    }
}

void LightingSystem::propagate_light(Registry& registry, Entity source_entity) {
    if (!registry.valid(source_entity) || !registry.has<Lighting>(source_entity)) {
        return;
    }
    
    auto& source_lighting = registry.get<Lighting>(source_entity);
    if (source_lighting.light_emission == 0) {
        return;
    }
    
    // Simple light propagation (simplified flood-fill)
    light_update_queue_.push(source_entity);
}

void LightingSystem::remove_light(Registry& registry, Entity removed_entity) {
    if (!registry.has<Position>(removed_entity)) {
        return;
    }
    
    auto& pos = registry.get<Position>(removed_entity);
    
    // Mark nearby blocks for lighting recalculation
    for (int dx = -15; dx <= 15; ++dx) {
        for (int dy = -15; dy <= 15; ++dy) {
            for (int dz = -15; dz <= 15; ++dz) {
                Position nearby_pos(
                    pos.world_pos.x + dx,
                    pos.world_pos.y + dy,
                    pos.world_pos.z + dz
                );
                
                utils::Vector3i block_pos{
                    static_cast<int>(nearby_pos.world_pos.x),
                    static_cast<int>(nearby_pos.world_pos.y),
                    static_cast<int>(nearby_pos.world_pos.z)
                };
                
                // Find block at this position
                auto view = registry.view<Position, Lighting>();
                for (auto entity : view) {
                    auto& entity_pos = view.get<Position>(entity);
                    utils::Vector3i entity_block_pos{
                        static_cast<int>(entity_pos.world_pos.x),
                        static_cast<int>(entity_pos.world_pos.y),
                        static_cast<int>(entity_pos.world_pos.z)
                    };
                    
                    if (entity_block_pos == block_pos) {
                        auto& lighting = view.get<Lighting>(entity);
                        lighting.needs_update = true;
                        light_update_queue_.push(entity);
                        break;
                    }
                }
            }
        }
    }
}

void LightingSystem::update_block_light(Registry& registry, Entity entity) {
    if (!registry.has<Position>(entity) || !registry.has<Lighting>(entity)) {
        return;
    }
    
    auto& lighting = registry.get<Lighting>(entity);
    auto& pos = registry.get<Position>(entity);
    
    // Simple lighting calculation
    std::uint8_t max_light = lighting.light_emission;
    
    // Check light from neighboring blocks
    for (int dx = -1; dx <= 1; ++dx) {
        for (int dy = -1; dy <= 1; ++dy) {
            for (int dz = -1; dz <= 1; ++dz) {
                if (dx == 0 && dy == 0 && dz == 0) continue;
                
                utils::Vector3i neighbor_pos{
                    static_cast<int>(pos.world_pos.x) + dx,
                    static_cast<int>(pos.world_pos.y) + dy,
                    static_cast<int>(pos.world_pos.z) + dz
                };
                
                // Find neighbor block and get its light level
                // This is simplified - in a real implementation, you'd use spatial indexing
                std::uint8_t neighbor_light = calculate_light_level(registry, Position(neighbor_pos.x, neighbor_pos.y, neighbor_pos.z));
                if (neighbor_light > 1) {
                    max_light = std::max(max_light, static_cast<std::uint8_t>(neighbor_light - 1));
                }
            }
        }
    }
    
    lighting.current_light = max_light;
}

void LightingSystem::update_sky_light(Registry& registry, Entity entity) {
    if (!registry.has<Position>(entity) || !registry.has<Lighting>(entity)) {
        return;
    }
    
    auto& lighting = registry.get<Lighting>(entity);
    auto& pos = registry.get<Position>(entity);
    
    // Simple sky light calculation - assume full sky light at surface
    if (pos.world_pos.y >= 100) {  // Simplified surface level
        lighting.sky_light = 15;
    } else {
        // Calculate based on blocks above
        lighting.sky_light = std::max(0, static_cast<int>(15 - (100 - pos.world_pos.y) / 10));
    }
}

std::uint8_t LightingSystem::calculate_light_level(Registry& registry, const Position& pos) {
    // Find block at position and return its light level
    auto view = registry.view<Position, Lighting>();
    
    for (auto entity : view) {
        auto& entity_pos = view.get<Position>(entity);
        if (static_cast<int>(entity_pos.world_pos.x) == static_cast<int>(pos.world_pos.x) &&
            static_cast<int>(entity_pos.world_pos.y) == static_cast<int>(pos.world_pos.y) &&
            static_cast<int>(entity_pos.world_pos.z) == static_cast<int>(pos.world_pos.z)) {
            return view.get<Lighting>(entity).current_light;
        }
    }
    
    return 0;  // No block found, no light
}

// ==================== RANDOM TICK SYSTEM IMPLEMENTATION ====================

void RandomTickSystem::update(Registry& registry, float delta_time) {
    auto view = registry.view<RandomTick, Position, Block>();
    
    for (auto entity : view) {
        auto& tick_comp = view.get<RandomTick>(entity);
        
        if (!tick_comp.enabled) continue;
        
        tick_comp.accumulated_time += delta_time;
        
        if (tick_comp.accumulated_time >= (1.0f / tick_comp.tick_rate)) {
            tick_comp.accumulated_time = 0.0f;
            
            auto& block = view.get<Block>(entity);
            
            // Process different block types
            switch (block.type) {
                case world::BlockType::GRASS_BLOCK:
                    if (registry.has<Growable>(entity)) {
                        process_grass_spread(registry, entity);
                    }
                    break;
                    
                case world::BlockType::OAK_SAPLING:
                case world::BlockType::SPRUCE_SAPLING:
                case world::BlockType::BIRCH_SAPLING:
                    if (registry.has<Growable>(entity)) {
                        process_tree_growth(registry, entity);
                    }
                    break;
                    
                default:
                    // Other block types
                    break;
            }
        }
    }
}

void RandomTickSystem::process_grass_spread(Registry& registry, Entity grass_entity) {
    if (!registry.has<Position>(grass_entity)) return;
    
    auto& pos = registry.get<Position>(grass_entity);
    
    // Random chance to spread to nearby dirt blocks
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> spread_chance(1, 10);
    
    if (spread_chance(gen) <= 2) {  // 20% chance
        // Try to spread to a random nearby position
        std::uniform_int_distribution<> offset(-1, 1);
        int dx = offset(gen);
        int dz = offset(gen);
        
        Position nearby_pos(
            pos.world_pos.x + dx,
            pos.world_pos.y,
            pos.world_pos.z + dz
        );
        
        // Find block at nearby position (simplified)
        // In a real implementation, you'd use the BlockSystem
    }
}

void RandomTickSystem::process_crop_growth(Registry& registry, Entity crop_entity) {
    if (!registry.has<Growable>(crop_entity)) return;
    
    auto& growable = registry.get<Growable>(crop_entity);
    
    growable.accumulated_time += 1.0f;  // One tick worth of growth
    
    if (growable.accumulated_time >= growable.growth_time && 
        growable.growth_stage < growable.max_stages) {
        
        growable.growth_stage++;
        growable.accumulated_time = 0.0f;
        
        spdlog::debug("Crop grew to stage {}/{}", growable.growth_stage, growable.max_stages);
    }
}

void RandomTickSystem::process_tree_growth(Registry& registry, Entity sapling_entity) {
    if (!registry.has<Growable>(sapling_entity) || !registry.has<Position>(sapling_entity)) {
        return;
    }
    
    auto& growable = registry.get<Growable>(sapling_entity);
    auto& pos = registry.get<Position>(sapling_entity);
    
    growable.accumulated_time += 1.0f;
    
    if (growable.accumulated_time >= growable.growth_time) {
        // Random chance for tree to grow
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> growth_chance(1, 100);
        
        if (growth_chance(gen) <= 5) {  // 5% chance per tick
            // Grow tree - replace sapling with log and add leaves
            auto& block = registry.get<Block>(sapling_entity);
            
            // Convert sapling to log based on type
            switch (block.type) {
                case world::BlockType::OAK_SAPLING:
                    block.type = world::BlockType::OAK_LOG;
                    break;
                case world::BlockType::SPRUCE_SAPLING:
                    block.type = world::BlockType::SPRUCE_LOG;
                    break;
                case world::BlockType::BIRCH_SAPLING:
                    block.type = world::BlockType::BIRCH_LOG;
                    break;
                default:
                    break;
            }
            
            // Remove growable component
            registry.remove<Growable>(sapling_entity);
            
            spdlog::info("Tree grew at position ({}, {}, {})", 
                        pos.world_pos.x, pos.world_pos.y, pos.world_pos.z);
        }
        
        growable.accumulated_time = 0.0f;
    }
}

// ==================== PHYSICS SYSTEM IMPLEMENTATION ====================

void PhysicsSystem::update(Registry& registry, float delta_time) {
    // Apply gravity to entities with physics
    auto physics_view = registry.view<Position, Physics, EntityData>();
    
    for (auto entity : physics_view) {
        auto& physics = physics_view.get<Physics>(entity);
        
        if (physics.affected_by_gravity) {
            apply_gravity(registry, entity, delta_time);
        }
    }
    
    // Check block support for gravity-affected blocks
    auto block_view = registry.view<Position, Block, Physics>();
    
    for (auto entity : block_view) {
        auto& physics = block_view.get<Physics>(entity);
        
        if (physics.affected_by_gravity) {
            check_block_support(registry, entity);
        }
    }
}

void PhysicsSystem::apply_gravity(Registry& registry, Entity entity, float delta_time) {
    if (!registry.has<EntityData>(entity)) return;
    
    auto& entity_data = registry.get<EntityData>(entity);
    
    // Apply gravity acceleration
    const float gravity = -9.81f;  // m/s²
    entity_data.velocity.y += gravity * delta_time;
    
    // Apply velocity to position
    if (registry.has<Position>(entity)) {
        auto& pos = registry.get<Position>(entity);
        pos.world_pos.x += entity_data.velocity.x * delta_time;
        pos.world_pos.y += entity_data.velocity.y * delta_time;
        pos.world_pos.z += entity_data.velocity.z * delta_time;
        
        // Update chunk and block positions
        pos.chunk_pos = utils::Vector3i(
            static_cast<int>(pos.world_pos.x) >> 4,
            0,
            static_cast<int>(pos.world_pos.z) >> 4
        );
        pos.block_pos = utils::Vector3i(
            static_cast<int>(pos.world_pos.x) & 15,
            static_cast<int>(pos.world_pos.y),
            static_cast<int>(pos.world_pos.z) & 15
        );
    }
    
    handle_collisions(registry, entity);
}

void PhysicsSystem::handle_collisions(Registry& registry, Entity entity) {
    if (!registry.has<Position>(entity) || !registry.has<EntityData>(entity)) {
        return;
    }
    
    auto& pos = registry.get<Position>(entity);
    auto& entity_data = registry.get<EntityData>(entity);
    
    // Simple ground collision detection
    if (pos.world_pos.y <= 0) {
        pos.world_pos.y = 0;
        entity_data.velocity.y = 0;
        entity_data.on_ground = true;
    } else {
        entity_data.on_ground = false;
    }
}

void PhysicsSystem::check_block_support(Registry& registry, Entity block_entity) {
    if (!registry.has<Position>(block_entity) || !registry.has<Block>(block_entity)) {
        return;
    }
    
    auto& pos = registry.get<Position>(block_entity);
    auto& block = registry.get<Block>(block_entity);
    
    // Check if block needs support (sand, gravel, etc.)
    bool needs_support = (block.type == world::BlockType::SAND || 
                         block.type == world::BlockType::GRAVEL);
    
    if (needs_support) {
        // Check block below
        Position below_pos(pos.world_pos.x, pos.world_pos.y - 1, pos.world_pos.z);
        
        // If no solid block below, convert to falling entity
        // This would require more complex entity conversion logic
        // For now, just log the event
        spdlog::debug("Block at ({}, {}, {}) needs support check", 
                     pos.world_pos.x, pos.world_pos.y, pos.world_pos.z);
    }
}

// ==================== INTERACTION SYSTEM IMPLEMENTATION ====================

void InteractionSystem::update(Registry& registry, float delta_time) {
    // This system mainly responds to events rather than continuous updates
    // In a real implementation, it would process interaction queues
}

void InteractionSystem::handle_block_break(Registry& registry, Entity player, Entity block) {
    if (!registry.valid(player) || !registry.valid(block)) {
        return;
    }
    
    if (!can_break_block(registry, player, block)) {
        return;
    }
    
    // Get block position for item drops
    Position drop_pos(0, 0, 0);
    if (registry.has<Position>(block)) {
        drop_pos = registry.get<Position>(block);
    }
    
    // Drop items
    drop_block_items(registry, block, drop_pos);
    
    // Destroy the block
    registry.destroy(block);
    
    spdlog::info("Player broke block at ({}, {}, {})", 
                drop_pos.world_pos.x, drop_pos.world_pos.y, drop_pos.world_pos.z);
}

void InteractionSystem::handle_block_place(Registry& registry, Entity player, const Position& pos, world::BlockType type) {
    if (!registry.valid(player)) {
        return;
    }
    
    if (!can_place_block(registry, player, pos)) {
        return;
    }
    
    // Create new block
    Block new_block(type);
    Entity block_entity = create_block_entity(registry, pos, type);
    
    spdlog::info("Player placed block at ({}, {}, {})", 
                pos.world_pos.x, pos.world_pos.y, pos.world_pos.z);
}

void InteractionSystem::handle_block_use(Registry& registry, Entity player, Entity block) {
    if (!registry.valid(player) || !registry.valid(block)) {
        return;
    }
    
    if (!registry.has<Interactable>(block)) {
        return;
    }
    
    auto& interactable = registry.get<Interactable>(block);
    if (interactable.can_right_click && interactable.on_interact) {
        interactable.on_interact(block, player);
    }
}

void InteractionSystem::drop_block_items(Registry& registry, Entity block_entity, const Position& pos) {
    if (!registry.has<Block>(block_entity)) {
        return;
    }
    
    auto& block = registry.get<Block>(block_entity);
    
    // Create item entity for the dropped block
    Entity item_entity = registry.create();
    registry.emplace<Position>(item_entity, pos);
    
    // Add item component (simplified)
    // In a real implementation, you'd have an Item component
    spdlog::debug("Dropped item from block type {}", static_cast<int>(block.type));
}

bool InteractionSystem::can_place_block(Registry& registry, Entity player, const Position& pos) {
    // Check if position is already occupied
    auto view = registry.view<Position, Block>();
    
    for (auto entity : view) {
        auto& entity_pos = view.get<Position>(entity);
        if (static_cast<int>(entity_pos.world_pos.x) == static_cast<int>(pos.world_pos.x) &&
            static_cast<int>(entity_pos.world_pos.y) == static_cast<int>(pos.world_pos.y) &&
            static_cast<int>(entity_pos.world_pos.z) == static_cast<int>(pos.world_pos.z)) {
            return false;  // Position occupied
        }
    }
    
    return true;
}

bool InteractionSystem::can_break_block(Registry& registry, Entity player, Entity block) {
    if (!registry.has<Block>(block)) {
        return false;
    }
    
    auto& block_comp = registry.get<Block>(block);
    
    // Bedrock cannot be broken
    if (block_comp.type == world::BlockType::BEDROCK) {
        return false;
    }
    
    return true;
}

// ==================== CHUNK SYSTEM IMPLEMENTATION ====================

void ChunkSystem::update(Registry& registry, float delta_time) {
    // Chunk loading/unloading would be triggered by player movement
    // This system mainly manages chunk state
}

void ChunkSystem::load_chunk(Registry& registry, std::int32_t chunk_x, std::int32_t chunk_z) {
    utils::Vector3<std::int32_t> chunk_pos(chunk_x, 0, chunk_z);
    
    if (loaded_chunks_.find(chunk_pos) != loaded_chunks_.end()) {
        return;  // Already loaded
    }
    
    // Load chunk data (from disk or generate)
    deserialize_chunk(registry, chunk_x, chunk_z);
    
    loaded_chunks_.insert(chunk_pos);
    
    spdlog::info("Loaded chunk ({}, {})", chunk_x, chunk_z);
}

void ChunkSystem::unload_chunk(Registry& registry, std::int32_t chunk_x, std::int32_t chunk_z) {
    utils::Vector3<std::int32_t> chunk_pos(chunk_x, 0, chunk_z);
    
    auto it = loaded_chunks_.find(chunk_pos);
    if (it == loaded_chunks_.end()) {
        return;  // Not loaded
    }
    
    // Save chunk data
    serialize_chunk(registry, chunk_x, chunk_z);
    
    // Remove all blocks in this chunk
    auto blocks = get_blocks_in_chunk(registry, chunk_x, chunk_z);
    for (Entity block : blocks) {
        registry.destroy(block);
    }
    
    loaded_chunks_.erase(it);
    
    spdlog::info("Unloaded chunk ({}, {})", chunk_x, chunk_z);
}

std::vector<Entity> ChunkSystem::get_blocks_in_chunk(Registry& registry, std::int32_t chunk_x, std::int32_t chunk_z) {
    std::vector<Entity> blocks;
    
    auto view = registry.view<Position, Block>();
    
    for (auto entity : view) {
        auto& pos = view.get<Position>(entity);
        if (pos.chunk_pos.x == chunk_x && pos.chunk_pos.z == chunk_z) {
            blocks.push_back(entity);
        }
    }
    
    return blocks;
}

void ChunkSystem::serialize_chunk(Registry& registry, std::int32_t chunk_x, std::int32_t chunk_z) {
    // Save chunk to disk (simplified - would use actual file I/O)
    auto blocks = get_blocks_in_chunk(registry, chunk_x, chunk_z);
    
    spdlog::debug("Serializing chunk ({}, {}) with {} blocks", chunk_x, chunk_z, blocks.size());
}

void ChunkSystem::deserialize_chunk(Registry& registry, std::int32_t chunk_x, std::int32_t chunk_z) {
    // Load chunk from disk or generate (simplified)
    // For now, generate a simple flat world
    
    for (int x = 0; x < 16; ++x) {
        for (int z = 0; z < 16; ++z) {
            int world_x = chunk_x * 16 + x;
            int world_z = chunk_z * 16 + z;
            
            // Generate simple terrain
            Position grass_pos(world_x, 64, world_z);
            Position dirt_pos(world_x, 63, world_z);
            Position stone_pos(world_x, 62, world_z);
            
            create_block_entity(registry, grass_pos, world::BlockType::GRASS_BLOCK);
            create_block_entity(registry, dirt_pos, world::BlockType::DIRT);
            create_block_entity(registry, stone_pos, world::BlockType::STONE);
        }
    }
    
    spdlog::debug("Generated chunk ({}, {}) with simple terrain", chunk_x, chunk_z);
}

} // namespace ecs
} // namespace parallelstone