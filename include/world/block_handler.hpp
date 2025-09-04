#pragma once

#include "block_state.hpp"
#include "../utils/vector3.hpp"
#include <memory>
#include <functional>
#include <vector>

namespace parallelstone {
namespace world {

// Forward declarations
class World;
class Chunk;
class Player;

/**
 * @brief Direction enumeration for block interactions
 */
enum class Direction : std::uint8_t {
    DOWN = 0,   // -Y
    UP = 1,     // +Y  
    NORTH = 2,  // -Z
    SOUTH = 3,  // +Z
    WEST = 4,   // -X
    EAST = 5    // +X
};

/**
 * @brief Context for block updates and interactions
 */
struct BlockContext {
    World* world;
    utils::Vector3i position;
    Chunk* chunk;
    Player* player; // May be null for non-player interactions
    Direction face; // Face that was clicked/interacted with
};

/**
 * @brief Base class for block behavior implementations
 * 
 * Modern C++20 implementation of Minecraft's block handler system.
 * Provides extensible behavior for different block types through inheritance.
 * 
 * Key improvements over Cuberite:
 * - Uses modern C++20 features and strong typing
 * - Simplified interface with fewer virtual methods
 * - Better performance through compile-time polymorphism where possible
 * - Thread-safe design for concurrent chunk operations
 */
class BlockHandler {
public:
    /**
     * @brief Constructor taking the block type this handler manages
     */
    explicit BlockHandler(BlockType type) noexcept : block_type_(type) {}
    
    /**
     * @brief Virtual destructor
     */
    virtual ~BlockHandler() = default;
    
    /**
     * @brief Get the block type this handler manages
     */
    BlockType block_type() const noexcept { return block_type_; }
    
    /**
     * @brief Called when block is placed
     */
    virtual void on_placed(const BlockContext& context, const BlockState& state) const {}
    
    /**
     * @brief Called when block is broken
     */
    virtual void on_broken(const BlockContext& context, const BlockState& old_state) const {}
    
    /**
     * @brief Called when player right-clicks the block
     * @return true if interaction was handled, false to use default behavior
     */
    virtual bool on_use(const BlockContext& context, const BlockState& state) const {
        return false;
    }
    
    /**
     * @brief Called when a neighboring block changes
     */
    virtual void on_neighbor_changed(const BlockContext& context, const BlockState& state, Direction neighbor_direction) const {}
    
    /**
     * @brief Called during random tick updates
     */
    virtual void on_random_tick(const BlockContext& context, const BlockState& state) const {}
    
    /**
     * @brief Called during scheduled tick updates
     */
    virtual void on_scheduled_tick(const BlockContext& context, const BlockState& state) const {}
    
    /**
     * @brief Get items dropped when block is broken
     */
    virtual std::vector<ItemStack> get_drops(const BlockState& state, const ItemStack* tool = nullptr) const;
    
    /**
     * @brief Check if block can be placed at the given location
     */
    virtual bool can_be_placed(const BlockContext& context, const BlockState& state) const {
        return true;
    }
    
    /**
     * @brief Get the bounding box for collision detection
     */
    virtual BoundingBox get_collision_box(const BlockState& state) const;
    
    /**
     * @brief Get the bounding box for selection/interaction
     */
    virtual BoundingBox get_selection_box(const BlockState& state) const;
    
    /**
     * @brief Check if block is transparent to light
     */
    virtual bool is_transparent(const BlockState& state) const {
        return BlockRegistry::get_properties(block_type_).is_transparent;
    }
    
    /**
     * @brief Get light emission level (0-15)
     */
    virtual std::uint8_t get_light_emission(const BlockState& state) const {
        return BlockRegistry::get_properties(block_type_).light_emission;
    }
    
    /**
     * @brief Get light filtering amount (0-15)
     */
    virtual std::uint8_t get_light_filter(const BlockState& state) const {
        return BlockRegistry::get_properties(block_type_).light_filter;
    }
    
    /**
     * @brief Schedule a tick update for this block
     */
    void schedule_tick(const BlockContext& context, std::uint32_t delay_ticks) const;

protected:
    /**
     * @brief Helper to notify neighboring blocks of change
     */
    void notify_neighbors(const BlockContext& context) const;
    
    /**
     * @brief Helper to drop item at block location
     */
    void drop_item(const BlockContext& context, const ItemStack& item) const;

private:
    BlockType block_type_;
};

/**
 * @brief Registry for block handlers
 */
class BlockHandlerRegistry {
public:
    /**
     * @brief Register a handler for a block type
     */
    static void register_handler(BlockType type, std::unique_ptr<BlockHandler> handler);
    
    /**
     * @brief Get handler for a block type
     */
    static const BlockHandler& get_handler(BlockType type);
    
    /**
     * @brief Initialize default handlers for all block types
     */
    static void initialize_defaults();
    
private:
    static std::unordered_map<BlockType, std::unique_ptr<BlockHandler>> handlers_;
    static std::unique_ptr<BlockHandler> default_handler_;
};

// Forward declaration for common item types
struct ItemStack {
    BlockType type;
    std::uint32_t count;
    std::uint16_t damage;
    
    ItemStack(BlockType t, std::uint32_t c = 1, std::uint16_t d = 0) 
        : type(t), count(c), damage(d) {}
};

// Forward declaration for bounding boxes
struct BoundingBox {
    double min_x, min_y, min_z;
    double max_x, max_y, max_z;
    
    BoundingBox(double x1, double y1, double z1, double x2, double y2, double z2)
        : min_x(x1), min_y(y1), min_z(z1), max_x(x2), max_y(y2), max_z(z2) {}
    
    static BoundingBox full_block() {
        return BoundingBox(0.0, 0.0, 0.0, 1.0, 1.0, 1.0);
    }
    
    static BoundingBox empty() {
        return BoundingBox(0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
    }
};

/**
 * @brief Specialized handlers for common block types
 */
namespace handlers {

/**
 * @brief Handler for air blocks (no-op for most operations)
 */
class AirHandler : public BlockHandler {
public:
    AirHandler() : BlockHandler(BlockType::AIR) {}
    
    bool is_transparent(const BlockState& state) const override { return true; }
    BoundingBox get_collision_box(const BlockState& state) const override { return BoundingBox::empty(); }
    BoundingBox get_selection_box(const BlockState& state) const override { return BoundingBox::empty(); }
    std::vector<ItemStack> get_drops(const BlockState& state, const ItemStack* tool) const override { return {}; }
};

/**
 * @brief Handler for grass blocks with spreading behavior
 */
class GrassHandler : public BlockHandler {
public:
    GrassHandler() : BlockHandler(BlockType::GRASS_BLOCK) {}
    
    void on_random_tick(const BlockContext& context, const BlockState& state) const override;
    std::vector<ItemStack> get_drops(const BlockState& state, const ItemStack* tool) const override;
    
private:
    void try_spread_to(const BlockContext& context, const utils::Vector3i& target) const;
    bool can_survive(const BlockContext& context) const;
};

/**
 * @brief Handler for directional blocks (logs, stairs, etc.)
 */
class DirectionalHandler : public BlockHandler {
public:
    explicit DirectionalHandler(BlockType type) : BlockHandler(type) {}
    
    void on_placed(const BlockContext& context, const BlockState& state) const override;
    
protected:
    virtual BlockState get_placed_state(const BlockContext& context, const BlockState& default_state) const;
};

/**
 * @brief Handler for liquid blocks (water, lava)
 */
class LiquidHandler : public BlockHandler {
public:
    explicit LiquidHandler(BlockType type) : BlockHandler(type) {}
    
    void on_scheduled_tick(const BlockContext& context, const BlockState& state) const override;
    void on_neighbor_changed(const BlockContext& context, const BlockState& state, Direction neighbor_direction) const override;
    bool is_transparent(const BlockState& state) const override { return true; }
    BoundingBox get_collision_box(const BlockState& state) const override { return BoundingBox::empty(); }
    
private:
    void try_flow(const BlockContext& context, const BlockState& state) const;
    bool can_flow_to(const BlockContext& context, const utils::Vector3i& target) const;
};

} // namespace handlers

} // namespace world
} // namespace parallelstone