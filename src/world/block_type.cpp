#include "world/block_type.hpp"
#include <unordered_map>
#include <string>

namespace parallelstone {
namespace world {

// Block properties database - based on Minecraft 1.21.7 data
const std::unordered_map<BlockType, BlockProperties> BlockRegistry::PROPERTIES = {
    // Air
    {BlockType::AIR, {0.0f, 0.0f, 0, 0, false, true, false, false, false, false}},
    
    // Stone family
    {BlockType::STONE, {1.5f, 6.0f, 0, 0, true, false, true, false, false, false}},
    {BlockType::GRANITE, {1.5f, 6.0f, 0, 0, true, false, true, false, false, false}},
    {BlockType::POLISHED_GRANITE, {1.5f, 6.0f, 0, 0, true, false, true, false, false, false}},
    {BlockType::DIORITE, {1.5f, 6.0f, 0, 0, true, false, true, false, false, false}},
    {BlockType::POLISHED_DIORITE, {1.5f, 6.0f, 0, 0, true, false, true, false, false, false}},
    {BlockType::ANDESITE, {1.5f, 6.0f, 0, 0, true, false, true, false, false, false}},
    {BlockType::POLISHED_ANDESITE, {1.5f, 6.0f, 0, 0, true, false, true, false, false, false}},
    
    // Deepslate family
    {BlockType::DEEPSLATE, {3.0f, 6.0f, 0, 0, true, false, true, false, false, false}},
    {BlockType::COBBLED_DEEPSLATE, {3.5f, 6.0f, 0, 0, true, false, true, false, false, false}},
    {BlockType::POLISHED_DEEPSLATE, {3.5f, 6.0f, 0, 0, true, false, true, false, false, false}},
    
    // Natural blocks
    {BlockType::GRASS_BLOCK, {0.6f, 0.6f, 0, 0, true, false, false, false, false, true}},
    {BlockType::DIRT, {0.5f, 0.5f, 0, 0, true, false, false, false, false, false}},
    {BlockType::COARSE_DIRT, {0.5f, 0.5f, 0, 0, true, false, false, false, false, false}},
    {BlockType::PODZOL, {0.5f, 0.5f, 0, 0, true, false, false, false, false, false}},
    {BlockType::ROOTED_DIRT, {0.5f, 0.5f, 0, 0, true, false, false, false, false, false}},
    {BlockType::MUD, {0.5f, 0.5f, 0, 0, true, false, false, false, false, false}},
    
    // Cobblestone
    {BlockType::COBBLESTONE, {2.0f, 6.0f, 0, 0, true, false, true, false, false, false}},
    {BlockType::MOSSY_COBBLESTONE, {2.0f, 6.0f, 0, 0, true, false, true, false, false, false}},
    
    // Wood planks
    {BlockType::OAK_PLANKS, {2.0f, 3.0f, 0, 0, true, false, false, true, false, false}},
    {BlockType::SPRUCE_PLANKS, {2.0f, 3.0f, 0, 0, true, false, false, true, false, false}},
    {BlockType::BIRCH_PLANKS, {2.0f, 3.0f, 0, 0, true, false, false, true, false, false}},
    {BlockType::JUNGLE_PLANKS, {2.0f, 3.0f, 0, 0, true, false, false, true, false, false}},
    {BlockType::ACACIA_PLANKS, {2.0f, 3.0f, 0, 0, true, false, false, true, false, false}},
    {BlockType::DARK_OAK_PLANKS, {2.0f, 3.0f, 0, 0, true, false, false, true, false, false}},
    {BlockType::MANGROVE_PLANKS, {2.0f, 3.0f, 0, 0, true, false, false, true, false, false}},
    {BlockType::CHERRY_PLANKS, {2.0f, 3.0f, 0, 0, true, false, false, true, false, false}},
    {BlockType::BAMBOO_PLANKS, {2.0f, 3.0f, 0, 0, true, false, false, true, false, false}},
    {BlockType::CRIMSON_PLANKS, {2.0f, 3.0f, 0, 0, true, false, false, false, false, false}},
    {BlockType::WARPED_PLANKS, {2.0f, 3.0f, 0, 0, true, false, false, false, false, false}},
    
    // Saplings (non-solid, transparent)
    {BlockType::OAK_SAPLING, {0.0f, 0.0f, 0, 0, false, true, false, false, false, true}},
    {BlockType::SPRUCE_SAPLING, {0.0f, 0.0f, 0, 0, false, true, false, false, false, true}},
    {BlockType::BIRCH_SAPLING, {0.0f, 0.0f, 0, 0, false, true, false, false, false, true}},
    {BlockType::JUNGLE_SAPLING, {0.0f, 0.0f, 0, 0, false, true, false, false, false, true}},
    {BlockType::ACACIA_SAPLING, {0.0f, 0.0f, 0, 0, false, true, false, false, false, true}},
    {BlockType::DARK_OAK_SAPLING, {0.0f, 0.0f, 0, 0, false, true, false, false, false, true}},
    {BlockType::MANGROVE_PROPAGULE, {0.0f, 0.0f, 0, 0, false, true, false, false, false, true}},
    {BlockType::CHERRY_SAPLING, {0.0f, 0.0f, 0, 0, false, true, false, false, false, true}},
    
    // Bedrock
    {BlockType::BEDROCK, {-1.0f, 3600000.0f, 0, 0, true, false, false, false, false, false}},
    
    // Liquids
    {BlockType::WATER, {100.0f, 100.0f, 0, 2, false, true, false, false, false, false}},
    {BlockType::LAVA, {100.0f, 100.0f, 15, 0, false, true, false, false, false, false}},
    
    // Sand
    {BlockType::SAND, {0.5f, 0.5f, 0, 0, true, false, false, false, false, false}},
    {BlockType::SUSPICIOUS_SAND, {0.25f, 0.25f, 0, 0, true, false, false, false, false, false}},
    {BlockType::RED_SAND, {0.5f, 0.5f, 0, 0, true, false, false, false, false, false}},
    
    // Gravel
    {BlockType::GRAVEL, {0.6f, 0.6f, 0, 0, true, false, false, false, false, false}},
    {BlockType::SUSPICIOUS_GRAVEL, {0.25f, 0.25f, 0, 0, true, false, false, false, false, false}},
    
    // Ores
    {BlockType::GOLD_ORE, {3.0f, 3.0f, 0, 0, true, false, true, false, false, false}},
    {BlockType::DEEPSLATE_GOLD_ORE, {4.5f, 3.0f, 0, 0, true, false, true, false, false, false}},
    {BlockType::IRON_ORE, {3.0f, 3.0f, 0, 0, true, false, true, false, false, false}},
    {BlockType::DEEPSLATE_IRON_ORE, {4.5f, 3.0f, 0, 0, true, false, true, false, false, false}},
    {BlockType::COAL_ORE, {3.0f, 3.0f, 0, 0, true, false, true, false, false, false}},
    {BlockType::DEEPSLATE_COAL_ORE, {4.5f, 3.0f, 0, 0, true, false, true, false, false, false}},
    
    // Glass
    {BlockType::GLASS, {0.3f, 0.3f, 0, 0, true, true, false, false, false, false}},
    {BlockType::TINTED_GLASS, {0.3f, 0.3f, 0, 15, true, false, false, false, false, false}},
    
    // Logs
    {BlockType::OAK_LOG, {2.0f, 2.0f, 0, 0, true, false, false, true, false, false}},
    {BlockType::SPRUCE_LOG, {2.0f, 2.0f, 0, 0, true, false, false, true, false, false}},
    {BlockType::BIRCH_LOG, {2.0f, 2.0f, 0, 0, true, false, false, true, false, false}},
    {BlockType::JUNGLE_LOG, {2.0f, 2.0f, 0, 0, true, false, false, true, false, false}},
    {BlockType::ACACIA_LOG, {2.0f, 2.0f, 0, 0, true, false, false, true, false, false}},
    {BlockType::DARK_OAK_LOG, {2.0f, 2.0f, 0, 0, true, false, false, true, false, false}},
    {BlockType::MANGROVE_LOG, {2.0f, 2.0f, 0, 0, true, false, false, true, false, false}},
    {BlockType::CHERRY_LOG, {2.0f, 2.0f, 0, 0, true, false, false, true, false, false}},
};

// Block names database
const std::unordered_map<BlockType, std::string_view> BlockRegistry::NAMES = {
    {BlockType::AIR, "minecraft:air"},
    {BlockType::STONE, "minecraft:stone"},
    {BlockType::GRANITE, "minecraft:granite"},
    {BlockType::POLISHED_GRANITE, "minecraft:polished_granite"},
    {BlockType::DIORITE, "minecraft:diorite"},
    {BlockType::POLISHED_DIORITE, "minecraft:polished_diorite"},
    {BlockType::ANDESITE, "minecraft:andesite"},
    {BlockType::POLISHED_ANDESITE, "minecraft:polished_andesite"},
    {BlockType::DEEPSLATE, "minecraft:deepslate"},
    {BlockType::COBBLED_DEEPSLATE, "minecraft:cobbled_deepslate"},
    {BlockType::POLISHED_DEEPSLATE, "minecraft:polished_deepslate"},
    {BlockType::GRASS_BLOCK, "minecraft:grass_block"},
    {BlockType::DIRT, "minecraft:dirt"},
    {BlockType::COARSE_DIRT, "minecraft:coarse_dirt"},
    {BlockType::PODZOL, "minecraft:podzol"},
    {BlockType::ROOTED_DIRT, "minecraft:rooted_dirt"},
    {BlockType::MUD, "minecraft:mud"},
    {BlockType::COBBLESTONE, "minecraft:cobblestone"},
    {BlockType::MOSSY_COBBLESTONE, "minecraft:mossy_cobblestone"},
    {BlockType::OAK_PLANKS, "minecraft:oak_planks"},
    {BlockType::SPRUCE_PLANKS, "minecraft:spruce_planks"},
    {BlockType::BIRCH_PLANKS, "minecraft:birch_planks"},
    {BlockType::JUNGLE_PLANKS, "minecraft:jungle_planks"},
    {BlockType::ACACIA_PLANKS, "minecraft:acacia_planks"},
    {BlockType::DARK_OAK_PLANKS, "minecraft:dark_oak_planks"},
    {BlockType::MANGROVE_PLANKS, "minecraft:mangrove_planks"},
    {BlockType::CHERRY_PLANKS, "minecraft:cherry_planks"},
    {BlockType::BAMBOO_PLANKS, "minecraft:bamboo_planks"},
    {BlockType::CRIMSON_PLANKS, "minecraft:crimson_planks"},
    {BlockType::WARPED_PLANKS, "minecraft:warped_planks"},
    {BlockType::OAK_SAPLING, "minecraft:oak_sapling"},
    {BlockType::SPRUCE_SAPLING, "minecraft:spruce_sapling"},
    {BlockType::BIRCH_SAPLING, "minecraft:birch_sapling"},
    {BlockType::JUNGLE_SAPLING, "minecraft:jungle_sapling"},
    {BlockType::ACACIA_SAPLING, "minecraft:acacia_sapling"},
    {BlockType::DARK_OAK_SAPLING, "minecraft:dark_oak_sapling"},
    {BlockType::MANGROVE_PROPAGULE, "minecraft:mangrove_propagule"},
    {BlockType::CHERRY_SAPLING, "minecraft:cherry_sapling"},
    {BlockType::BEDROCK, "minecraft:bedrock"},
    {BlockType::WATER, "minecraft:water"},
    {BlockType::LAVA, "minecraft:lava"},
    {BlockType::SAND, "minecraft:sand"},
    {BlockType::SUSPICIOUS_SAND, "minecraft:suspicious_sand"},
    {BlockType::RED_SAND, "minecraft:red_sand"},
    {BlockType::GRAVEL, "minecraft:gravel"},
    {BlockType::SUSPICIOUS_GRAVEL, "minecraft:suspicious_gravel"},
    {BlockType::GLASS, "minecraft:glass"},
    {BlockType::TINTED_GLASS, "minecraft:tinted_glass"},
    {BlockType::OAK_LOG, "minecraft:oak_log"},
    {BlockType::SPRUCE_LOG, "minecraft:spruce_log"},
    {BlockType::BIRCH_LOG, "minecraft:birch_log"},
    {BlockType::JUNGLE_LOG, "minecraft:jungle_log"},
    {BlockType::ACACIA_LOG, "minecraft:acacia_log"},
    {BlockType::DARK_OAK_LOG, "minecraft:dark_oak_log"},
    {BlockType::MANGROVE_LOG, "minecraft:mangrove_log"},
    {BlockType::CHERRY_LOG, "minecraft:cherry_log"},
    {BlockType::UNKNOWN, "minecraft:unknown"}
};

// Reverse lookup map (name to type)
const std::unordered_map<std::string_view, BlockType> BlockRegistry::NAME_TO_TYPE = []() {
    std::unordered_map<std::string_view, BlockType> map;
    for (const auto& [type, name] : NAMES) {
        map[name] = type;
    }
    return map;
}();

// Default properties for unknown blocks
static const BlockProperties DEFAULT_PROPERTIES = {
    1.0f, 1.0f, 0, 0, true, false, false, false, false, false
};

const BlockProperties& BlockRegistry::get_properties(BlockType type) noexcept {
    auto it = PROPERTIES.find(type);
    return (it != PROPERTIES.end()) ? it->second : DEFAULT_PROPERTIES;
}

std::string_view BlockRegistry::get_name(BlockType type) noexcept {
    auto it = NAMES.find(type);
    return (it != NAMES.end()) ? it->second : "minecraft:unknown";
}

BlockType BlockRegistry::from_name(std::string_view name) noexcept {
    auto it = NAME_TO_TYPE.find(name);
    return (it != NAME_TO_TYPE.end()) ? it->second : BlockType::UNKNOWN;
}

// Block utility functions
namespace block_utils {

bool is_replaceable(BlockType type) noexcept {
    switch (type) {
        case BlockType::AIR:
        case BlockType::WATER:
        case BlockType::LAVA:
            return true;
        default:
            return false;
    }
}

} // namespace block_utils

} // namespace world
} // namespace parallelstone