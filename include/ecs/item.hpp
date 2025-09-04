#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <variant>

namespace parallelstone {
namespace ecs {

/**
 * @brief Item type enumeration
 */
enum class ItemType : std::uint16_t {
    AIR = 0,
    
    // Basic blocks as items
    STONE = 1,
    GRASS_BLOCK = 2,
    DIRT = 3,
    COBBLESTONE = 4,
    OAK_PLANKS = 5,
    OAK_LOG = 6,
    
    // Tools
    WOODEN_PICKAXE = 100,
    STONE_PICKAXE = 101,
    IRON_PICKAXE = 102,
    DIAMOND_PICKAXE = 103,
    
    // Food
    APPLE = 200,
    BREAD = 201,
    COOKED_BEEF = 202,
    
    // Materials
    STICK = 300,
    COAL = 301,
    IRON_INGOT = 302,
    DIAMOND = 303,
    
    UNKNOWN = 65535
};

/**
 * @brief NBT-like data storage for item properties
 */
using ItemProperty = std::variant<bool, std::int32_t, std::int64_t, float, double, std::string>;

/**
 * @brief Individual item with count and metadata
 */
struct ItemStack {
    ItemType type = ItemType::AIR;
    std::uint32_t count = 0;
    std::uint16_t damage = 0;  // Durability damage for tools
    std::unordered_map<std::string, ItemProperty> properties;
    
    ItemStack() = default;
    
    explicit ItemStack(ItemType item_type, std::uint32_t item_count = 1) 
        : type(item_type), count(item_count) {}
    
    ItemStack(ItemType item_type, std::uint32_t item_count, std::uint16_t item_damage) 
        : type(item_type), count(item_count), damage(item_damage) {}
    
    bool is_empty() const {
        return type == ItemType::AIR || count == 0;
    }
    
    bool is_stackable() const {
        // Tools and damaged items don't stack
        return damage == 0 && !is_tool();
    }
    
    bool is_tool() const {
        return static_cast<std::uint16_t>(type) >= 100 && static_cast<std::uint16_t>(type) < 200;
    }
    
    std::uint32_t max_stack_size() const {
        if (is_tool()) return 1;
        return 64;  // Default stack size
    }
    
    bool can_stack_with(const ItemStack& other) const {
        return type == other.type && 
               damage == other.damage && 
               properties == other.properties &&
               is_stackable();
    }
    
    // Combine this stack with another (returns overflow)
    ItemStack combine_with(const ItemStack& other) {
        if (!can_stack_with(other)) {
            return other;  // Can't combine, return the other stack unchanged
        }
        
        std::uint32_t max_size = max_stack_size();
        std::uint32_t total = count + other.count;
        
        if (total <= max_size) {
            count = total;
            return ItemStack();  // No overflow
        } else {
            count = max_size;
            return ItemStack(type, total - max_size);  // Return overflow
        }
    }
    
    // Split this stack
    ItemStack split(std::uint32_t amount) {
        if (amount >= count) {
            ItemStack result = *this;
            *this = ItemStack();
            return result;
        } else {
            ItemStack result(type, amount, damage);
            result.properties = properties;
            count -= amount;
            return result;
        }
    }
};

} // namespace ecs
} // namespace parallelstone