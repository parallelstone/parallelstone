#pragma once

#include "ecs/core.hpp"
#include "ecs/item.hpp"
#include "world/compile_time_blocks.hpp"
#include "world/block_registry.hpp"
#include "utils/vector3.hpp"
#include <optional>
#include <queue>

namespace parallelstone {
namespace ecs {

// ==================== COMPONENTS ====================

/**
 * @brief Position component for all world objects
 */
struct Position {
    utils::Vector3d world_pos;
    utils::Vector3i chunk_pos;
    utils::Vector3i block_pos;
    
    Position(double x, double y, double z) 
        : world_pos(x, y, z)
        , chunk_pos(static_cast<int>(x) >> 4, 0, static_cast<int>(z) >> 4)
        , block_pos(static_cast<int>(x) & 15, static_cast<int>(y), static_cast<int>(z) & 15) {}
};

/**
 * @brief Block component - makes an entity a block
 */
struct Block {
    std::uint16_t universal_id = 0;  // Universal ID (0 = air)
    
    Block() = default;
    explicit Block(std::uint16_t id) : universal_id(id) {}
    explicit Block(world::BlockType t) {
        universal_id = static_cast<std::uint16_t>(t);
    }
    
    // Legacy compatibility
    world::BlockType type() const {
        return static_cast<world::BlockType>(universal_id);
    }
};

/**
 * @brief Physics properties for blocks and entities
 */
struct Physics {
    bool solid = true;
    bool transparent = false;
    float hardness = 1.0f;
    float blast_resistance = 1.0f;
    bool affected_by_gravity = false;
};

/**
 * @brief Lighting component for light-emitting or light-affecting objects
 */
struct Lighting {
    std::uint8_t light_emission = 0;    // 0-15
    std::uint8_t light_filter = 0;      // How much light this blocks
    std::uint8_t current_light = 0;     // Current light level at this position
    std::uint8_t sky_light = 15;        // Current sky light level
    bool needs_update = false;
};

/**
 * @brief Random tick component for blocks that need periodic updates
 */
struct RandomTick {
    float tick_rate = 1.0f;           // Ticks per second
    float accumulated_time = 0.0f;    // Time since last tick
    bool enabled = true;
};

/**
 * @brief Interaction component for player-interactable objects
 */
struct Interactable {
    bool can_right_click = false;
    bool can_left_click = false;
    bool requires_tool = false;
    std::function<void(Entity, Entity)> on_interact; // (target, player)
};

/**
 * @brief Growth component for plants and growing blocks
 */
struct Growable {
    std::uint8_t growth_stage = 0;
    std::uint8_t max_stages = 1;
    float growth_time = 60.0f;       // Seconds per stage
    float accumulated_time = 0.0f;
    bool can_spread = false;
};

/**
 * @brief Inventory component for blocks that can store items
 */
struct Inventory {
    std::vector<ItemStack> items;
    std::uint32_t max_slots = 0;
    bool is_public = true;  // Can other players access
};

/**
 * @brief Player-specific component
 */
struct Player {
    std::string username;
    std::string uuid;
    float health = 20.0f;
    float hunger = 20.0f;
    std::uint32_t experience = 0;
    bool is_creative = false;
    bool is_flying = false;
};

/**
 * @brief Entity metadata (velocity, rotation, etc.)
 */
struct EntityData {
    utils::Vector3d velocity{0, 0, 0};
    float yaw = 0.0f;
    float pitch = 0.0f;
    float width = 1.0f;
    float height = 1.0f;
    bool on_ground = false;
};

/**
 * @brief Chunk reference component
 */
struct ChunkRef {
    std::int32_t chunk_x;
    std::int32_t chunk_z;
    std::uint8_t section_y;
};

// ==================== SYSTEMS ====================

/**
 * @brief Block management system
 */
class BlockSystem : public System {
public:
    void update(Registry& registry, float delta_time) override;
    
    // Block operations
    Entity create_block(Registry& registry, const Position& pos, const Block& block);
    bool destroy_block(Registry& registry, Entity block_entity);
    bool set_block(Registry& registry, const Position& pos, world::BlockType type);
    std::optional<Entity> get_block(Registry& registry, const Position& pos);
    
    // Block state queries
    bool is_solid(Registry& registry, Entity block_entity);
    bool is_transparent(Registry& registry, Entity block_entity);
    world::BlockType get_block_type(Registry& registry, Entity block_entity);

private:
    // Spatial indexing for fast block lookups
    std::unordered_map<utils::Vector3i, Entity> block_index_;
    
    void update_block_index(Registry& registry);
    void handle_block_placement(Registry& registry, Entity entity);
    void handle_block_destruction(Registry& registry, Entity entity);
};

/**
 * @brief Lighting calculation system
 */
class LightingSystem : public System {
public:
    void update(Registry& registry, float delta_time) override;
    
    void recalculate_lighting(Registry& registry, const utils::Vector3i& chunk_pos);
    void propagate_light(Registry& registry, Entity source_entity);
    void remove_light(Registry& registry, Entity removed_entity);

private:
    std::queue<Entity> light_update_queue_;
    
    void update_block_light(Registry& registry, Entity entity);
    void update_sky_light(Registry& registry, Entity entity);
    std::uint8_t calculate_light_level(Registry& registry, const Position& pos);
};

/**
 * @brief Random tick system for block updates
 */
class RandomTickSystem : public System {
public:
    void update(Registry& registry, float delta_time) override;

private:
    void process_grass_spread(Registry& registry, Entity grass_entity);
    void process_crop_growth(Registry& registry, Entity crop_entity);
    void process_tree_growth(Registry& registry, Entity sapling_entity);
};

/**
 * @brief Physics system for entities and falling blocks
 */
class PhysicsSystem : public System {
public:
    void update(Registry& registry, float delta_time) override;

private:
    void apply_gravity(Registry& registry, Entity entity, float delta_time);
    void handle_collisions(Registry& registry, Entity entity);
    void check_block_support(Registry& registry, Entity block_entity);
};

/**
 * @brief Player interaction system
 */
class InteractionSystem : public System {
public:
    void update(Registry& registry, float delta_time) override;
    
    void handle_block_break(Registry& registry, Entity player, Entity block);
    void handle_block_place(Registry& registry, Entity player, const Position& pos, world::BlockType type);
    void handle_block_use(Registry& registry, Entity player, Entity block);

private:
    void drop_block_items(Registry& registry, Entity block_entity, const Position& pos);
    bool can_place_block(Registry& registry, Entity player, const Position& pos);
    bool can_break_block(Registry& registry, Entity player, Entity block);
};

/**
 * @brief Chunk management system
 */
class ChunkSystem : public System {
public:
    void update(Registry& registry, float delta_time) override;
    
    void load_chunk(Registry& registry, std::int32_t chunk_x, std::int32_t chunk_z);
    void unload_chunk(Registry& registry, std::int32_t chunk_x, std::int32_t chunk_z);
    std::vector<Entity> get_blocks_in_chunk(Registry& registry, std::int32_t chunk_x, std::int32_t chunk_z);

private:
    std::unordered_set<utils::Vector3<std::int32_t>> loaded_chunks_;
    
    void serialize_chunk(Registry& registry, std::int32_t chunk_x, std::int32_t chunk_z);
    void deserialize_chunk(Registry& registry, std::int32_t chunk_x, std::int32_t chunk_z);
};

// ==================== UTILITY FUNCTIONS ====================

/**
 * @brief Create a block entity with common components
 */
inline Entity create_block_entity(Registry& registry, const Position& pos, world::BlockType type) {
    Entity entity = registry.create();
    
    registry.emplace<Position>(entity, pos);
    registry.emplace<Block>(entity, type);
    
    // Add physics based on block type
    bool is_solid = (type != world::BlockType::AIR);
    registry.emplace<Physics>(entity, Physics{
        .solid = is_solid,
        .transparent = !is_solid,
        .hardness = 1.0f,
        .blast_resistance = 1.0f
    });
    
    // Add lighting for transparent blocks
    if (!is_solid) {
        registry.emplace<Lighting>(entity, Lighting{
            .light_emission = 0,
            .light_filter = 0
        });
    }
    
    return entity;
}

/**
 * @brief Create a player entity
 */
inline Entity create_player_entity(Registry& registry, const std::string& username, const Position& spawn_pos) {
    Entity entity = registry.create();
    
    registry.emplace<Position>(entity, spawn_pos);
    registry.emplace<Player>(entity, Player{
        .username = username,
        .uuid = username + "_uuid" // Simplified UUID
    });
    registry.emplace<EntityData>(entity, EntityData{
        .width = 0.6f,
        .height = 1.8f
    });
    registry.emplace<Physics>(entity, Physics{
        .solid = true,
        .affected_by_gravity = true
    });
    
    return entity;
}

/**
 * @brief Query blocks in a specific area
 */
template<typename Func>
void query_blocks_in_area(Registry& registry, const utils::Vector3i& min_pos, const utils::Vector3i& max_pos, Func&& func) {
    auto view = registry.view<Position, Block>();
    
    for (auto entity : view) {
        auto& pos = view.get<Position>(entity);
        auto& block = view.get<Block>(entity);
        
        utils::Vector3i block_pos{
            static_cast<int>(pos.world_pos.x),
            static_cast<int>(pos.world_pos.y),
            static_cast<int>(pos.world_pos.z)
        };
        
        if (block_pos.x >= min_pos.x && block_pos.x <= max_pos.x &&
            block_pos.y >= min_pos.y && block_pos.y <= max_pos.y &&
            block_pos.z >= min_pos.z && block_pos.z <= max_pos.z) {
            func(entity, pos, block);
        }
    }
}

} // namespace ecs
} // namespace parallelstone