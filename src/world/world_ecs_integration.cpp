#include "world/world_ecs_integration.hpp"
#include <algorithm>
#include <spdlog/spdlog.h>

namespace parallelstone {
namespace world {

// ==================== WORLD ECS INTEGRATION ====================

WorldECSIntegration::WorldECSIntegration(std::shared_ptr<World> world) : world_(world) {
    spdlog::info("Initialized ECS integration for world '{}'", world_->config().world_name);
    register_default_systems();
}

ecs::Entity WorldECSIntegration::create_block_entity(std::int32_t x, std::int32_t y, std::int32_t z, BlockType block_type) {
    auto entity = registry_.create();
    auto key = coordinates_to_key(x, y, z);
    
    // Add block component
    ecs::Block block_component;
    block_component.universal_id = static_cast<std::uint16_t>(block_type);
    registry_.emplace<ecs::Block>(entity, std::move(block_component));
    
    // Add position component
    ecs::Position position_component;
    position_component.x = x;
    position_component.y = y;
    position_component.z = z;
    registry_.emplace<ecs::Position>(entity, std::move(position_component));
    
    // Track entity
    coordinate_to_entity_[key] = entity;
    entity_to_coordinate_[entity] = key;
    
    // Add to chunk tracking
    auto chunk_coord = World::world_to_chunk(x, z);
    std::uint64_t chunk_key = (static_cast<std::uint64_t>(chunk_coord.x) << 32) | static_cast<std::uint64_t>(chunk_coord.z);
    chunk_entities_[chunk_key].push_back(entity);
    
    return entity;
}

std::optional<ecs::Entity> WorldECSIntegration::get_block_entity(std::int32_t x, std::int32_t y, std::int32_t z) const {
    auto key = coordinates_to_key(x, y, z);
    auto it = coordinate_to_entity_.find(key);
    if (it != coordinate_to_entity_.end()) {
        return it->second;
    }
    return std::nullopt;
}

void WorldECSIntegration::set_block(std::int32_t x, std::int32_t y, std::int32_t z, BlockType block_type, bool update_entity) {
    // Update world
    BlockType old_block = world_->get_block(x, y, z);
    world_->set_block(x, y, z, block_type);
    
    if (update_entity) {
        // Update or create ECS entity
        auto entity_opt = get_block_entity(x, y, z);
        
        if (block_type == BlockType::AIR) {
            // Remove entity for air blocks
            if (entity_opt) {
                auto key = coordinates_to_key(x, y, z);
                coordinate_to_entity_.erase(key);
                entity_to_coordinate_.erase(*entity_opt);
                registry_.destroy(*entity_opt);
            }
        } else {
            if (entity_opt) {
                // Update existing entity
                if (registry_.has<ecs::Block>(*entity_opt)) {
                    auto& block_component = registry_.get<ecs::Block>(*entity_opt);
                    block_component.universal_id = static_cast<std::uint16_t>(block_type);
                }
            } else {
                // Create new entity
                create_block_entity(x, y, z, block_type);
            }
        }
        
        // Notify block change
        on_block_changed(x, y, z, old_block, block_type);
    }
}

void WorldECSIntegration::sync_chunk_blocks(std::int32_t chunk_x, std::int32_t chunk_z) {
    auto chunk = world_->get_chunk(chunk_x, chunk_z, false);
    if (!chunk) {
        return;
    }
    
    // Iterate through all blocks in chunk
    for (std::uint8_t x = 0; x < 16; x++) {
        for (std::uint8_t z = 0; z < 16; z++) {
            for (std::int32_t y = -64; y < 320; y++) {
                if (!Chunk::is_valid_y(y)) continue;
                
                auto block_state = chunk->get_block(x, y, z);
                auto block_type = block_state.get_block_type();
                
                if (block_type != BlockType::AIR) {
                    std::int32_t world_x = chunk_x * 16 + x;
                    std::int32_t world_z = chunk_z * 16 + z;
                    
                    // Create block entity if it doesn't exist
                    if (!get_block_entity(world_x, y, world_z)) {
                        create_block_entity(world_x, y, world_z, block_type);
                    }
                }
            }
        }
    }
    
    spdlog::debug("Synced chunk ({}, {}) blocks with ECS", chunk_x, chunk_z);
}

void WorldECSIntegration::cleanup_chunk_entities(std::int32_t chunk_x, std::int32_t chunk_z) {
    std::uint64_t chunk_key = (static_cast<std::uint64_t>(chunk_x) << 32) | static_cast<std::uint64_t>(chunk_z);
    
    auto it = chunk_entities_.find(chunk_key);
    if (it != chunk_entities_.end()) {
        for (auto entity : it->second) {
            // Remove from coordinate tracking
            auto coord_it = entity_to_coordinate_.find(entity);
            if (coord_it != entity_to_coordinate_.end()) {
                coordinate_to_entity_.erase(coord_it->second);
                entity_to_coordinate_.erase(coord_it);
            }
            
            // Destroy entity
            registry_.destroy(entity);
        }
        
        chunk_entities_.erase(it);
        spdlog::debug("Cleaned up {} entities from chunk ({}, {})", it->second.size(), chunk_x, chunk_z);
    }
}

ecs::Entity WorldECSIntegration::create_player(const std::string& username, std::int32_t x, std::int32_t y, std::int32_t z) {
    auto entity = registry_.create();
    
    // Add player component
    ecs::Player player_component;
    player_component.username = username;
    player_component.health = 20.0f;
    player_component.food_level = 20;
    registry_.emplace<ecs::Player>(entity, std::move(player_component));
    
    // Add position component
    ecs::Position position_component;
    position_component.x = x;
    position_component.y = y;
    position_component.z = z;
    registry_.emplace<ecs::Position>(entity, std::move(position_component));
    
    // Add velocity component for physics
    ecs::Velocity velocity_component;
    registry_.emplace<ecs::Velocity>(entity, std::move(velocity_component));
    
    // Add inventory component
    ecs::Inventory inventory_component;
    inventory_component.size = 36; // Standard player inventory size
    inventory_component.items.resize(36);
    registry_.emplace<ecs::Inventory>(entity, std::move(inventory_component));
    
    spdlog::info("Created player entity for '{}' at ({}, {}, {})", username, x, y, z);
    return entity;
}

ecs::Entity WorldECSIntegration::create_mob(const std::string& mob_type, std::int32_t x, std::int32_t y, std::int32_t z) {
    auto entity = registry_.create();
    
    // Add mob component
    ecs::Mob mob_component;
    mob_component.mob_type = mob_type;
    mob_component.health = 20.0f; // Default health
    registry_.emplace<ecs::Mob>(entity, std::move(mob_component));
    
    // Add position component
    ecs::Position position_component;
    position_component.x = x;
    position_component.y = y;
    position_component.z = z;
    registry_.emplace<ecs::Position>(entity, std::move(position_component));
    
    // Add velocity component for physics
    ecs::Velocity velocity_component;
    registry_.emplace<ecs::Velocity>(entity, std::move(velocity_component));
    
    spdlog::debug("Created mob entity '{}' at ({}, {}, {})", mob_type, x, y, z);
    return entity;
}

ecs::Entity WorldECSIntegration::create_item(ecs::ItemStack item_stack, std::int32_t x, std::int32_t y, std::int32_t z) {
    auto entity = registry_.create();
    
    // Add item component
    ecs::DroppedItem item_component;
    item_component.item = std::move(item_stack);
    item_component.pickup_delay = 1.0f; // 1 second pickup delay
    item_component.despawn_timer = 300.0f; // 5 minutes despawn timer
    registry_.emplace<ecs::DroppedItem>(entity, std::move(item_component));
    
    // Add position component
    ecs::Position position_component;
    position_component.x = x;
    position_component.y = y;
    position_component.z = z;
    registry_.emplace<ecs::Position>(entity, std::move(position_component));
    
    // Add velocity component for physics
    ecs::Velocity velocity_component;
    velocity_component.dy = 0.2f; // Initial upward velocity
    registry_.emplace<ecs::Velocity>(entity, std::move(velocity_component));
    
    return entity;
}

std::vector<ecs::Entity> WorldECSIntegration::get_entities_in_chunk(std::int32_t chunk_x, std::int32_t chunk_z) const {
    std::uint64_t chunk_key = (static_cast<std::uint64_t>(chunk_x) << 32) | static_cast<std::uint64_t>(chunk_z);
    
    auto it = chunk_entities_.find(chunk_key);
    if (it != chunk_entities_.end()) {
        return it->second;
    }
    
    return {};
}

std::vector<ecs::Entity> WorldECSIntegration::get_entities_in_radius(std::int32_t center_x, std::int32_t center_y, 
                                                                    std::int32_t center_z, double radius) const {
    std::vector<ecs::Entity> result;
    double radius_squared = radius * radius;
    
    auto view = registry_.view<ecs::Position>();
    for (auto entity : view) {
        const auto& pos = view.get<ecs::Position>(entity);
        
        double dx = pos.x - center_x;
        double dy = pos.y - center_y;
        double dz = pos.z - center_z;
        double distance_squared = dx * dx + dy * dy + dz * dz;
        
        if (distance_squared <= radius_squared) {
            result.push_back(entity);
        }
    }
    
    return result;
}

void WorldECSIntegration::update_systems(float delta_time) {
    registry_.update_systems(delta_time);
}

void WorldECSIntegration::register_default_systems() {
    add_system<systems::BlockUpdateSystem>(this);
    add_system<systems::PhysicsSystem>(this);
    add_system<systems::LightingSystem>(this);
    add_system<systems::ChunkLoadingSystem>(this);
    add_system<systems::ItemSystem>(this);
    
    spdlog::debug("Registered default world systems");
}

void WorldECSIntegration::on_chunk_loaded(std::int32_t chunk_x, std::int32_t chunk_z) {
    // Sync chunk blocks with ECS
    sync_chunk_blocks(chunk_x, chunk_z);
}

void WorldECSIntegration::on_chunk_unloading(std::int32_t chunk_x, std::int32_t chunk_z) {
    // Clean up chunk entities
    cleanup_chunk_entities(chunk_x, chunk_z);
}

void WorldECSIntegration::on_block_changed(std::int32_t x, std::int32_t y, std::int32_t z, 
                                          BlockType old_block, BlockType new_block) {
    // Handle block change events
    // TODO: Implement block change notification system
}

void WorldECSIntegration::synchronize() {
    // Synchronize world state with ECS
    auto loaded_chunks = world_->get_loaded_chunks();
    
    for (const auto& coord : loaded_chunks) {
        sync_chunk_blocks(coord.x, coord.z);
    }
    
    spdlog::debug("Synchronized {} chunks with ECS", loaded_chunks.size());
}

std::uint64_t WorldECSIntegration::coordinates_to_key(std::int32_t x, std::int32_t y, std::int32_t z) const {
    // Pack coordinates into 64-bit key
    std::uint64_t ux = static_cast<std::uint64_t>(static_cast<std::uint32_t>(x));
    std::uint64_t uy = static_cast<std::uint64_t>(static_cast<std::uint32_t>(y));
    std::uint64_t uz = static_cast<std::uint64_t>(static_cast<std::uint32_t>(z));
    
    return (ux << 32) | (uy << 16) | uz;
}

std::tuple<std::int32_t, std::int32_t, std::int32_t> WorldECSIntegration::key_to_coordinates(std::uint64_t key) const {
    std::int32_t x = static_cast<std::int32_t>(key >> 32);
    std::int32_t y = static_cast<std::int32_t>((key >> 16) & 0xFFFF);
    std::int32_t z = static_cast<std::int32_t>(key & 0xFFFF);
    
    return std::make_tuple(x, y, z);
}

// ==================== ECS SYSTEMS ====================

namespace systems {

void BlockUpdateSystem::update(ecs::Registry& registry, float delta_time) {
    // Handle block updates and state changes
    auto view = registry.view<ecs::Block, ecs::Position>();
    
    for (auto entity : view) {
        const auto& block = view.get<ecs::Block>(entity);
        const auto& pos = view.get<ecs::Position>(entity);
        
        // TODO: Implement block update logic (growth, decay, etc.)
        // For now, just ensure consistency with world state
        
        auto world_block = integration_->get_world()->get_block(pos.x, pos.y, pos.z);
        if (static_cast<std::uint16_t>(world_block) != block.universal_id) {
            // Block mismatch - update ECS to match world
            // This shouldn't happen in normal operation
            spdlog::warn("Block mismatch at ({}, {}, {}): ECS={}, World={}", 
                        pos.x, pos.y, pos.z, block.universal_id, static_cast<std::uint16_t>(world_block));
        }
    }
}

void PhysicsSystem::update(ecs::Registry& registry, float delta_time) {
    handle_gravity(registry, delta_time);
    handle_collisions(registry);
    update_positions(registry, delta_time);
}

void PhysicsSystem::handle_gravity(ecs::Registry& registry, float delta_time) {
    auto view = registry.view<ecs::Velocity, ecs::Position>();
    
    for (auto entity : view) {
        auto& velocity = view.get<ecs::Velocity>(entity);
        const auto& pos = view.get<ecs::Position>(entity);
        
        // Apply gravity (skip if on ground)
        std::int32_t below_y = static_cast<std::int32_t>(pos.y) - 1;
        if (below_y >= -64) {
            auto below_block = integration_->get_world()->get_block(
                static_cast<std::int32_t>(pos.x), below_y, static_cast<std::int32_t>(pos.z));
            
            if (below_block == BlockType::AIR) {
                velocity.dy -= 9.8f * delta_time; // Gravity acceleration
            } else {
                // On ground - stop vertical velocity
                if (velocity.dy < 0) {
                    velocity.dy = 0;
                }
            }
        }
    }
}

void PhysicsSystem::handle_collisions(ecs::Registry& registry) {
    auto view = registry.view<ecs::Position, ecs::Velocity>();
    
    for (auto entity : view) {
        const auto& pos = view.get<ecs::Position>(entity);
        auto& velocity = view.get<ecs::Velocity>(entity);
        
        // Simple collision detection
        std::int32_t check_x = static_cast<std::int32_t>(pos.x + velocity.dx);
        std::int32_t check_y = static_cast<std::int32_t>(pos.y + velocity.dy);
        std::int32_t check_z = static_cast<std::int32_t>(pos.z + velocity.dz);
        
        // Check collision with blocks
        auto block = integration_->get_world()->get_block(check_x, check_y, check_z);
        if (block != BlockType::AIR) {
            // Simple collision response - stop movement
            if (velocity.dx > 0 && check_x > pos.x) velocity.dx = 0;
            if (velocity.dx < 0 && check_x < pos.x) velocity.dx = 0;
            if (velocity.dz > 0 && check_z > pos.z) velocity.dz = 0;
            if (velocity.dz < 0 && check_z < pos.z) velocity.dz = 0;
            if (velocity.dy < 0 && check_y < pos.y) velocity.dy = 0;
        }
    }
}

void PhysicsSystem::update_positions(ecs::Registry& registry, float delta_time) {
    auto view = registry.view<ecs::Position, ecs::Velocity>();
    
    for (auto entity : view) {
        auto& pos = view.get<ecs::Position>(entity);
        const auto& velocity = view.get<ecs::Velocity>(entity);
        
        // Update position based on velocity
        pos.x += velocity.dx * delta_time;
        pos.y += velocity.dy * delta_time;
        pos.z += velocity.dz * delta_time;
        
        // Apply friction/damping
        auto& vel = view.get<ecs::Velocity>(entity);
        vel.dx *= 0.98f;
        vel.dz *= 0.98f;
    }
}

void LightingSystem::update(ecs::Registry& registry, float delta_time) {
    // Process pending light updates
    for (const auto& [x, y, z] : pending_light_updates_) {
        integration_->get_world()->update_lighting(x, y, z, x, y, z);
    }
    pending_light_updates_.clear();
    
    // TODO: Implement dynamic lighting from light-emitting blocks
}

void ChunkLoadingSystem::update(ecs::Registry& registry, float delta_time) {
    update_player_chunk_loading(registry);
    manage_entity_visibility(registry);
}

void ChunkLoadingSystem::update_player_chunk_loading(ecs::Registry& registry) {
    auto view = registry.view<ecs::Player, ecs::Position>();
    
    for (auto entity : view) {
        const auto& player = view.get<ecs::Player>(entity);
        const auto& pos = view.get<ecs::Position>(entity);
        
        std::int32_t player_chunk_x = static_cast<std::int32_t>(pos.x) >> 4;
        std::int32_t player_chunk_z = static_cast<std::int32_t>(pos.z) >> 4;
        
        // Load chunks around player
        std::uint32_t view_distance = integration_->get_world()->config().chunk_view_distance;
        integration_->get_world()->load_chunks_around(player_chunk_x, player_chunk_z, view_distance);
        
        // Unload distant chunks
        integration_->get_world()->unload_chunks_outside(player_chunk_x, player_chunk_z, view_distance + 2);
    }
}

void ChunkLoadingSystem::manage_entity_visibility(ecs::Registry& registry) {
    // TODO: Implement entity visibility management based on chunk loading
}

void ItemSystem::update(ecs::Registry& registry, float delta_time) {
    handle_item_pickup(registry);
    handle_item_despawn(registry, delta_time);
    update_item_physics(registry, delta_time);
}

void ItemSystem::handle_item_pickup(ecs::Registry& registry) {
    auto item_view = registry.view<ecs::DroppedItem, ecs::Position>();
    auto player_view = registry.view<ecs::Player, ecs::Position, ecs::Inventory>();
    
    for (auto item_entity : item_view) {
        auto& dropped_item = item_view.get<ecs::DroppedItem>(item_entity);
        const auto& item_pos = item_view.get<ecs::Position>(item_entity);
        
        // Skip if pickup delay is active
        if (dropped_item.pickup_delay > 0) {
            continue;
        }
        
        // Check for nearby players
        for (auto player_entity : player_view) {
            const auto& player_pos = player_view.get<ecs::Position>(player_entity);
            auto& inventory = player_view.get<ecs::Inventory>(player_entity);
            
            // Calculate distance
            double dx = item_pos.x - player_pos.x;
            double dy = item_pos.y - player_pos.y;
            double dz = item_pos.z - player_pos.z;
            double distance_squared = dx * dx + dy * dy + dz * dz;
            
            // Pickup range: 1 block
            if (distance_squared <= 1.0) {
                // Try to add item to inventory
                for (auto& slot : inventory.items) {
                    if (!slot.has_value()) {
                        slot = dropped_item.item;
                        registry.destroy(item_entity);
                        goto next_item; // Break out of nested loops
                    }
                }
            }
        }
        
        next_item:;
    }
}

void ItemSystem::handle_item_despawn(ecs::Registry& registry, float delta_time) {
    auto view = registry.view<ecs::DroppedItem>();
    std::vector<ecs::Entity> to_destroy;
    
    for (auto entity : view) {
        auto& dropped_item = view.get<ecs::DroppedItem>(entity);
        
        // Update timers
        dropped_item.pickup_delay -= delta_time;
        dropped_item.despawn_timer -= delta_time;
        
        // Despawn if timer expired
        if (dropped_item.despawn_timer <= 0) {
            to_destroy.push_back(entity);
        }
    }
    
    // Destroy expired items
    for (auto entity : to_destroy) {
        registry.destroy(entity);
    }
}

void ItemSystem::update_item_physics(ecs::Registry& registry, float delta_time) {
    auto view = registry.view<ecs::DroppedItem, ecs::Position, ecs::Velocity>();
    
    for (auto entity : view) {
        auto& velocity = view.get<ecs::Velocity>(entity);
        const auto& pos = view.get<ecs::Position>(entity);
        
        // Items move toward nearby players
        auto player_view = registry.view<ecs::Player, ecs::Position>();
        for (auto player_entity : player_view) {
            const auto& player_pos = player_view.get<ecs::Position>(player_entity);
            
            double dx = player_pos.x - pos.x;
            double dy = player_pos.y - pos.y;
            double dz = player_pos.z - pos.z;
            double distance = std::sqrt(dx * dx + dy * dy + dz * dz);
            
            // Attraction range: 2 blocks
            if (distance <= 2.0 && distance > 0.1) {
                double attraction_strength = 3.0 * delta_time;
                velocity.dx += (dx / distance) * attraction_strength;
                velocity.dy += (dy / distance) * attraction_strength;
                velocity.dz += (dz / distance) * attraction_strength;
                break; // Only attract to nearest player
            }
        }
    }
}

} // namespace systems

// ==================== FACTORY ====================

std::unordered_map<std::string, std::function<std::unique_ptr<ecs::System>(WorldECSIntegration*)>> 
    WorldECSFactory::system_factories_;

std::unique_ptr<WorldECSIntegration> WorldECSFactory::create_integration(std::shared_ptr<World> world) {
    auto integration = std::make_unique<WorldECSIntegration>(world);
    
    // Register any custom systems from factories
    for (const auto& [name, factory] : system_factories_) {
        auto system = factory(integration.get());
        // TODO: Add system to registry (requires extending ECS API)
    }
    
    return integration;
}

} // namespace world
} // namespace parallelstone