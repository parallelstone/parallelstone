#pragma once

#include "world.hpp"
#include "ecs/core.hpp"
#include "ecs/world_ecs.hpp"
#include <memory>

namespace parallelstone {
namespace world {

/**
 * @brief Integration layer between World system and ECS architecture
 * 
 * Provides seamless integration between the traditional chunk-based world
 * management and the Entity Component System for blocks, entities, and players.
 */
class WorldECSIntegration {
public:
    /**
     * @brief Initialize ECS integration for a world
     */
    explicit WorldECSIntegration(std::shared_ptr<World> world);
    
    /**
     * @brief Get the ECS registry for this world
     */
    ecs::Registry& get_registry() { return registry_; }
    const ecs::Registry& get_registry() const { return registry_; }
    
    /**
     * @brief Get the world instance
     */
    std::shared_ptr<World> get_world() { return world_; }
    const std::shared_ptr<World> get_world() const { return world_; }
    
    // ==================== BLOCK ECS OPERATIONS ====================
    
    /**
     * @brief Create block entity at world coordinates
     */
    ecs::Entity create_block_entity(std::int32_t x, std::int32_t y, std::int32_t z, BlockType block_type);
    
    /**
     * @brief Get block entity at coordinates (if exists)
     */
    std::optional<ecs::Entity> get_block_entity(std::int32_t x, std::int32_t y, std::int32_t z) const;
    
    /**
     * @brief Update block in both world and ECS
     */
    void set_block(std::int32_t x, std::int32_t y, std::int32_t z, BlockType block_type, bool update_entity = true);
    
    /**
     * @brief Sync chunk blocks with ECS entities
     */
    void sync_chunk_blocks(std::int32_t chunk_x, std::int32_t chunk_z);
    
    /**
     * @brief Remove block entities from unloaded chunk
     */
    void cleanup_chunk_entities(std::int32_t chunk_x, std::int32_t chunk_z);
    
    // ==================== ENTITY MANAGEMENT ====================
    
    /**
     * @brief Create player entity
     */
    ecs::Entity create_player(const std::string& username, std::int32_t x, std::int32_t y, std::int32_t z);
    
    /**
     * @brief Create mob entity
     */
    ecs::Entity create_mob(const std::string& mob_type, std::int32_t x, std::int32_t y, std::int32_t z);
    
    /**
     * @brief Create item entity
     */
    ecs::Entity create_item(ecs::ItemStack item_stack, std::int32_t x, std::int32_t y, std::int32_t z);
    
    /**
     * @brief Get all entities in chunk
     */
    std::vector<ecs::Entity> get_entities_in_chunk(std::int32_t chunk_x, std::int32_t chunk_z) const;
    
    /**
     * @brief Get entities within radius
     */
    std::vector<ecs::Entity> get_entities_in_radius(std::int32_t center_x, std::int32_t center_y, std::int32_t center_z, 
                                                    double radius) const;
    
    // ==================== SYSTEM MANAGEMENT ====================
    
    /**
     * @brief Add system to the ECS
     */
    template<typename SystemType, typename... Args>
    void add_system(Args&&... args) {
        registry_.add_system<SystemType>(std::forward<Args>(args)...);
    }
    
    /**
     * @brief Update all ECS systems
     */
    void update_systems(float delta_time);
    
    /**
     * @brief Register default world systems
     */
    void register_default_systems();
    
    // ==================== COORDINATION ====================
    
    /**
     * @brief Handle chunk loading event
     */
    void on_chunk_loaded(std::int32_t chunk_x, std::int32_t chunk_z);
    
    /**
     * @brief Handle chunk unloading event
     */
    void on_chunk_unloading(std::int32_t chunk_x, std::int32_t chunk_z);
    
    /**
     * @brief Handle block change event
     */
    void on_block_changed(std::int32_t x, std::int32_t y, std::int32_t z, BlockType old_block, BlockType new_block);
    
    /**
     * @brief Synchronize world state with ECS
     */
    void synchronize();
    
private:
    /**
     * @brief Convert world coordinates to entity key
     */
    std::uint64_t coordinates_to_key(std::int32_t x, std::int32_t y, std::int32_t z) const;
    
    /**
     * @brief Convert entity key to world coordinates
     */
    std::tuple<std::int32_t, std::int32_t, std::int32_t> key_to_coordinates(std::uint64_t key) const;
    
    std::shared_ptr<World> world_;
    ecs::Registry registry_;
    
    // Block entity tracking
    std::unordered_map<std::uint64_t, ecs::Entity> coordinate_to_entity_;
    std::unordered_map<ecs::Entity, std::uint64_t> entity_to_coordinate_;
    
    // Chunk entity tracking
    std::unordered_map<std::uint64_t, std::vector<ecs::Entity>> chunk_entities_;
};

/**
 * @brief World-specific ECS systems
 */
namespace systems {

/**
 * @brief Block update system for handling block state changes
 */
class BlockUpdateSystem : public ecs::System {
public:
    explicit BlockUpdateSystem(WorldECSIntegration* integration) : integration_(integration) {}
    
    void update(ecs::Registry& registry, float delta_time) override;
    
private:
    WorldECSIntegration* integration_;
};

/**
 * @brief Physics system for entity movement and collision
 */
class PhysicsSystem : public ecs::System {
public:
    explicit PhysicsSystem(WorldECSIntegration* integration) : integration_(integration) {}
    
    void update(ecs::Registry& registry, float delta_time) override;
    
private:
    WorldECSIntegration* integration_;
    
    void handle_gravity(ecs::Registry& registry, float delta_time);
    void handle_collisions(ecs::Registry& registry);
    void update_positions(ecs::Registry& registry, float delta_time);
};

/**
 * @brief Lighting system for dynamic light updates
 */
class LightingSystem : public ecs::System {
public:
    explicit LightingSystem(WorldECSIntegration* integration) : integration_(integration) {}
    
    void update(ecs::Registry& registry, float delta_time) override;
    
private:
    WorldECSIntegration* integration_;
    std::vector<std::tuple<std::int32_t, std::int32_t, std::int32_t>> pending_light_updates_;
};

/**
 * @brief Chunk loading system for managing entity visibility
 */
class ChunkLoadingSystem : public ecs::System {
public:
    explicit ChunkLoadingSystem(WorldECSIntegration* integration) : integration_(integration) {}
    
    void update(ecs::Registry& registry, float delta_time) override;
    
private:
    WorldECSIntegration* integration_;
    
    void update_player_chunk_loading(ecs::Registry& registry);
    void manage_entity_visibility(ecs::Registry& registry);
};

/**
 * @brief Item system for handling dropped items and pickup
 */
class ItemSystem : public ecs::System {
public:
    explicit ItemSystem(WorldECSIntegration* integration) : integration_(integration) {}
    
    void update(ecs::Registry& registry, float delta_time) override;
    
private:
    WorldECSIntegration* integration_;
    
    void handle_item_pickup(ecs::Registry& registry);
    void handle_item_despawn(ecs::Registry& registry, float delta_time);
    void update_item_physics(ecs::Registry& registry, float delta_time);
};

} // namespace systems

/**
 * @brief Factory for creating world ECS integrations
 */
class WorldECSFactory {
public:
    /**
     * @brief Create ECS integration for world
     */
    static std::unique_ptr<WorldECSIntegration> create_integration(std::shared_ptr<World> world);
    
    /**
     * @brief Register custom system factory
     */
    template<typename SystemType>
    static void register_system_factory(std::function<std::unique_ptr<SystemType>(WorldECSIntegration*)> factory) {
        system_factories_[typeid(SystemType).name()] = [factory](WorldECSIntegration* integration) -> std::unique_ptr<ecs::System> {
            return factory(integration);
        };
    }
    
private:
    static std::unordered_map<std::string, std::function<std::unique_ptr<ecs::System>(WorldECSIntegration*)>> system_factories_;
};

} // namespace world
} // namespace parallelstone