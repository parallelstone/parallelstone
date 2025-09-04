#include "world/block_registry.hpp"
#include <unordered_set>

namespace parallelstone {
namespace world {

// Block properties database for Minecraft 1.21.7
const std::unordered_map<BlockType, BlockProperties> BlockRegistry::PROPERTIES = {
    // Air
    {BlockType::AIR, {0.0f, 0.0f, 0, 0, false, true, false, false, false, false, false}},
    
    // Stone variants
    {BlockType::STONE, {1.5f, 6.0f, 0, 0, true, false, true, false, false, false, false}},
    {BlockType::GRANITE, {1.5f, 6.0f, 0, 0, true, false, true, false, false, false, false}},
    {BlockType::POLISHED_GRANITE, {1.5f, 6.0f, 0, 0, true, false, true, false, false, false, false}},
    {BlockType::DIORITE, {1.5f, 6.0f, 0, 0, true, false, true, false, false, false, false}},
    {BlockType::POLISHED_DIORITE, {1.5f, 6.0f, 0, 0, true, false, true, false, false, false, false}},
    {BlockType::ANDESITE, {1.5f, 6.0f, 0, 0, true, false, true, false, false, false, false}},
    {BlockType::POLISHED_ANDESITE, {1.5f, 6.0f, 0, 0, true, false, true, false, false, false, false}},
    
    // Deepslate family (1.17+)
    {BlockType::DEEPSLATE, {3.0f, 6.0f, 0, 0, true, false, true, false, false, false, false}},
    {BlockType::COBBLED_DEEPSLATE, {3.5f, 6.0f, 0, 0, true, false, true, false, false, false, false}},
    {BlockType::POLISHED_DEEPSLATE, {3.5f, 6.0f, 0, 0, true, false, true, false, false, false, false}},
    {BlockType::DEEPSLATE_BRICKS, {3.5f, 6.0f, 0, 0, true, false, true, false, false, false, false}},
    {BlockType::CRACKED_DEEPSLATE_BRICKS, {3.5f, 6.0f, 0, 0, true, false, true, false, false, false, false}},
    {BlockType::DEEPSLATE_TILES, {3.5f, 6.0f, 0, 0, true, false, true, false, false, false, false}},
    {BlockType::CRACKED_DEEPSLATE_TILES, {3.5f, 6.0f, 0, 0, true, false, true, false, false, false, false}},
    {BlockType::CHISELED_DEEPSLATE, {3.5f, 6.0f, 0, 0, true, false, true, false, false, false, false}},
    
    // Natural blocks
    {BlockType::GRASS_BLOCK, {0.6f, 0.6f, 0, 0, true, false, false, false, false, true, false}},
    {BlockType::DIRT, {0.5f, 0.5f, 0, 0, true, false, false, false, false, false, false}},
    {BlockType::COARSE_DIRT, {0.5f, 0.5f, 0, 0, true, false, false, false, false, false, false}},
    {BlockType::PODZOL, {0.5f, 0.5f, 0, 0, true, false, false, false, false, false, false}},
    {BlockType::ROOTED_DIRT, {0.5f, 0.5f, 0, 0, true, false, false, false, false, false, false}},
    {BlockType::MUD, {0.5f, 0.5f, 0, 0, true, false, false, false, false, false, false}},
    {BlockType::CRIMSON_NYLIUM, {0.4f, 0.4f, 0, 0, true, false, false, false, false, false, false}},
    {BlockType::WARPED_NYLIUM, {0.4f, 0.4f, 0, 0, true, false, false, false, false, false, false}},
    
    // Cobblestone
    {BlockType::COBBLESTONE, {2.0f, 6.0f, 0, 0, true, false, true, false, false, false, false}},
    {BlockType::MOSSY_COBBLESTONE, {2.0f, 6.0f, 0, 0, true, false, true, false, false, false, false}},
    
    // Wood planks
    {BlockType::OAK_PLANKS, {2.0f, 3.0f, 0, 0, true, false, false, true, false, false, false}},
    {BlockType::SPRUCE_PLANKS, {2.0f, 3.0f, 0, 0, true, false, false, true, false, false, false}},
    {BlockType::BIRCH_PLANKS, {2.0f, 3.0f, 0, 0, true, false, false, true, false, false, false}},
    {BlockType::JUNGLE_PLANKS, {2.0f, 3.0f, 0, 0, true, false, false, true, false, false, false}},
    {BlockType::ACACIA_PLANKS, {2.0f, 3.0f, 0, 0, true, false, false, true, false, false, false}},
    {BlockType::DARK_OAK_PLANKS, {2.0f, 3.0f, 0, 0, true, false, false, true, false, false, false}},
    {BlockType::MANGROVE_PLANKS, {2.0f, 3.0f, 0, 0, true, false, false, true, false, false, false}},
    {BlockType::CHERRY_PLANKS, {2.0f, 3.0f, 0, 0, true, false, false, true, false, false, false}},
    {BlockType::BAMBOO_PLANKS, {2.0f, 3.0f, 0, 0, true, false, false, true, false, false, false}},
    {BlockType::CRIMSON_PLANKS, {2.0f, 3.0f, 0, 0, true, false, false, false, false, false, false}},
    {BlockType::WARPED_PLANKS, {2.0f, 3.0f, 0, 0, true, false, false, false, false, false, false}},
    
    // Saplings (non-solid, transparent)
    {BlockType::OAK_SAPLING, {0.0f, 0.0f, 0, 0, false, true, false, false, false, true, false}},
    {BlockType::SPRUCE_SAPLING, {0.0f, 0.0f, 0, 0, false, true, false, false, false, true, false}},
    {BlockType::BIRCH_SAPLING, {0.0f, 0.0f, 0, 0, false, true, false, false, false, true, false}},
    {BlockType::JUNGLE_SAPLING, {0.0f, 0.0f, 0, 0, false, true, false, false, false, true, false}},
    {BlockType::ACACIA_SAPLING, {0.0f, 0.0f, 0, 0, false, true, false, false, false, true, false}},
    {BlockType::DARK_OAK_SAPLING, {0.0f, 0.0f, 0, 0, false, true, false, false, false, true, false}},
    {BlockType::MANGROVE_PROPAGULE, {0.0f, 0.0f, 0, 0, false, true, false, false, false, true, false}},
    {BlockType::CHERRY_SAPLING, {0.0f, 0.0f, 0, 0, false, true, false, false, false, true, false}},
    
    // Bedrock
    {BlockType::BEDROCK, {-1.0f, 3600000.0f, 0, 0, true, false, false, false, false, false, false}},
    
    // Liquids
    {BlockType::WATER, {100.0f, 100.0f, 0, 2, false, true, false, false, true, false, false}},
    {BlockType::LAVA, {100.0f, 100.0f, 15, 0, false, true, false, false, false, false, false}},
    
    // Sand family
    {BlockType::SAND, {0.5f, 0.5f, 0, 0, true, false, false, false, false, false, true}},
    {BlockType::SUSPICIOUS_SAND, {0.25f, 0.25f, 0, 0, true, false, false, false, false, false, true}},
    {BlockType::RED_SAND, {0.5f, 0.5f, 0, 0, true, false, false, false, false, false, true}},
    
    // Gravel
    {BlockType::GRAVEL, {0.6f, 0.6f, 0, 0, true, false, false, false, false, false, true}},
    {BlockType::SUSPICIOUS_GRAVEL, {0.25f, 0.25f, 0, 0, true, false, false, false, false, false, true}},
    
    // Ores
    {BlockType::COAL_ORE, {3.0f, 3.0f, 0, 0, true, false, true, false, false, false, false}},
    {BlockType::DEEPSLATE_COAL_ORE, {4.5f, 3.0f, 0, 0, true, false, true, false, false, false, false}},
    {BlockType::IRON_ORE, {3.0f, 3.0f, 0, 0, true, false, true, false, false, false, false}},
    {BlockType::DEEPSLATE_IRON_ORE, {4.5f, 3.0f, 0, 0, true, false, true, false, false, false, false}},
    {BlockType::COPPER_ORE, {3.0f, 3.0f, 0, 0, true, false, true, false, false, false, false}},
    {BlockType::DEEPSLATE_COPPER_ORE, {4.5f, 3.0f, 0, 0, true, false, true, false, false, false, false}},
    {BlockType::GOLD_ORE, {3.0f, 3.0f, 0, 0, true, false, true, false, false, false, false}},
    {BlockType::DEEPSLATE_GOLD_ORE, {4.5f, 3.0f, 0, 0, true, false, true, false, false, false, false}},
    {BlockType::REDSTONE_ORE, {3.0f, 3.0f, 0, 0, true, false, true, false, false, false, false}},
    {BlockType::DEEPSLATE_REDSTONE_ORE, {4.5f, 3.0f, 0, 0, true, false, true, false, false, false, false}},
    {BlockType::EMERALD_ORE, {3.0f, 3.0f, 0, 0, true, false, true, false, false, false, false}},
    {BlockType::DEEPSLATE_EMERALD_ORE, {4.5f, 3.0f, 0, 0, true, false, true, false, false, false, false}},
    {BlockType::LAPIS_ORE, {3.0f, 3.0f, 0, 0, true, false, true, false, false, false, false}},
    {BlockType::DEEPSLATE_LAPIS_ORE, {4.5f, 3.0f, 0, 0, true, false, true, false, false, false, false}},
    {BlockType::DIAMOND_ORE, {3.0f, 3.0f, 0, 0, true, false, true, false, false, false, false}},
    {BlockType::DEEPSLATE_DIAMOND_ORE, {4.5f, 3.0f, 0, 0, true, false, true, false, false, false, false}},
    
    // Nether ores
    {BlockType::NETHER_GOLD_ORE, {3.0f, 3.0f, 0, 0, true, false, true, false, false, false, false}},
    {BlockType::NETHER_QUARTZ_ORE, {3.0f, 3.0f, 0, 0, true, false, true, false, false, false, false}},
    {BlockType::ANCIENT_DEBRIS, {30.0f, 1200.0f, 0, 0, true, false, true, false, false, false, false}},
    
    // Logs
    {BlockType::OAK_LOG, {2.0f, 2.0f, 0, 0, true, false, false, true, false, false, false}},
    {BlockType::SPRUCE_LOG, {2.0f, 2.0f, 0, 0, true, false, false, true, false, false, false}},
    {BlockType::BIRCH_LOG, {2.0f, 2.0f, 0, 0, true, false, false, true, false, false, false}},
    {BlockType::JUNGLE_LOG, {2.0f, 2.0f, 0, 0, true, false, false, true, false, false, false}},
    {BlockType::ACACIA_LOG, {2.0f, 2.0f, 0, 0, true, false, false, true, false, false, false}},
    {BlockType::DARK_OAK_LOG, {2.0f, 2.0f, 0, 0, true, false, false, true, false, false, false}},
    {BlockType::MANGROVE_LOG, {2.0f, 2.0f, 0, 0, true, false, false, true, false, false, false}},
    {BlockType::CHERRY_LOG, {2.0f, 2.0f, 0, 0, true, false, false, true, false, false, false}},
    {BlockType::CRIMSON_STEM, {2.0f, 2.0f, 0, 0, true, false, false, false, false, false, false}},
    {BlockType::WARPED_STEM, {2.0f, 2.0f, 0, 0, true, false, false, false, false, false, false}},
    
    // Glass
    {BlockType::GLASS, {0.3f, 0.3f, 0, 0, true, true, false, false, false, false, false}},
    {BlockType::TINTED_GLASS, {0.3f, 0.3f, 0, 15, true, false, false, false, false, false, false}},
    
    // Leaves (transparent, flammable)
    {BlockType::OAK_LEAVES, {0.2f, 0.2f, 0, 0, false, true, false, true, false, true, false}},
    {BlockType::SPRUCE_LEAVES, {0.2f, 0.2f, 0, 0, false, true, false, true, false, true, false}},
    {BlockType::BIRCH_LEAVES, {0.2f, 0.2f, 0, 0, false, true, false, true, false, true, false}},
    {BlockType::JUNGLE_LEAVES, {0.2f, 0.2f, 0, 0, false, true, false, true, false, true, false}},
    {BlockType::ACACIA_LEAVES, {0.2f, 0.2f, 0, 0, false, true, false, true, false, true, false}},
    {BlockType::DARK_OAK_LEAVES, {0.2f, 0.2f, 0, 0, false, true, false, true, false, true, false}},
    {BlockType::MANGROVE_LEAVES, {0.2f, 0.2f, 0, 0, false, true, false, true, false, true, false}},
    {BlockType::CHERRY_LEAVES, {0.2f, 0.2f, 0, 0, false, true, false, true, false, true, false}},
    {BlockType::AZALEA_LEAVES, {0.2f, 0.2f, 0, 0, false, true, false, true, false, true, false}},
    {BlockType::FLOWERING_AZALEA_LEAVES, {0.2f, 0.2f, 0, 0, false, true, false, true, false, true, false}},
    
    // More blocks can be added here...
    // For brevity, I'll add just a few more key ones
    
    // Wool
    {BlockType::WHITE_WOOL, {0.8f, 0.8f, 0, 0, true, false, false, true, false, false, false}},
    {BlockType::BLACK_WOOL, {0.8f, 0.8f, 0, 0, true, false, false, true, false, false, false}},
    
    // Flowers (non-solid, transparent)
    {BlockType::DANDELION, {0.0f, 0.0f, 0, 0, false, true, false, false, false, false, false}},
    {BlockType::POPPY, {0.0f, 0.0f, 0, 0, false, true, false, false, false, false, false}},
    
    // 1.21 blocks
    {BlockType::CRAFTER, {1.5f, 3.5f, 0, 0, true, false, true, false, false, false, false}},
    {BlockType::TRIAL_SPAWNER, {50.0f, 50.0f, 0, 0, true, false, true, false, false, false, false}},
    {BlockType::VAULT, {50.0f, 50.0f, 0, 0, true, false, true, false, false, false, false}},
    {BlockType::HEAVY_CORE, {10.0f, 1200.0f, 0, 0, true, false, true, false, false, false, false}},
    
    // Copper family
    {BlockType::COPPER_BLOCK, {3.0f, 6.0f, 0, 0, true, false, true, false, false, false, false}},
    {BlockType::EXPOSED_COPPER, {3.0f, 6.0f, 0, 0, true, false, true, false, false, false, false}},
    {BlockType::WEATHERED_COPPER, {3.0f, 6.0f, 0, 0, true, false, true, false, false, false, false}},
    {BlockType::OXIDIZED_COPPER, {3.0f, 6.0f, 0, 0, true, false, true, false, false, false, false}},
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
    {BlockType::DEEPSLATE_BRICKS, "minecraft:deepslate_bricks"},
    {BlockType::CRACKED_DEEPSLATE_BRICKS, "minecraft:cracked_deepslate_bricks"},
    {BlockType::DEEPSLATE_TILES, "minecraft:deepslate_tiles"},
    {BlockType::CRACKED_DEEPSLATE_TILES, "minecraft:cracked_deepslate_tiles"},
    {BlockType::CHISELED_DEEPSLATE, "minecraft:chiseled_deepslate"},
    {BlockType::GRASS_BLOCK, "minecraft:grass_block"},
    {BlockType::DIRT, "minecraft:dirt"},
    {BlockType::COARSE_DIRT, "minecraft:coarse_dirt"},
    {BlockType::PODZOL, "minecraft:podzol"},
    {BlockType::ROOTED_DIRT, "minecraft:rooted_dirt"},
    {BlockType::MUD, "minecraft:mud"},
    {BlockType::CRIMSON_NYLIUM, "minecraft:crimson_nylium"},
    {BlockType::WARPED_NYLIUM, "minecraft:warped_nylium"},
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
    {BlockType::COAL_ORE, "minecraft:coal_ore"},
    {BlockType::DEEPSLATE_COAL_ORE, "minecraft:deepslate_coal_ore"},
    {BlockType::IRON_ORE, "minecraft:iron_ore"},
    {BlockType::DEEPSLATE_IRON_ORE, "minecraft:deepslate_iron_ore"},
    {BlockType::COPPER_ORE, "minecraft:copper_ore"},
    {BlockType::DEEPSLATE_COPPER_ORE, "minecraft:deepslate_copper_ore"},
    {BlockType::GOLD_ORE, "minecraft:gold_ore"},
    {BlockType::DEEPSLATE_GOLD_ORE, "minecraft:deepslate_gold_ore"},
    {BlockType::REDSTONE_ORE, "minecraft:redstone_ore"},
    {BlockType::DEEPSLATE_REDSTONE_ORE, "minecraft:deepslate_redstone_ore"},
    {BlockType::EMERALD_ORE, "minecraft:emerald_ore"},
    {BlockType::DEEPSLATE_EMERALD_ORE, "minecraft:deepslate_emerald_ore"},
    {BlockType::LAPIS_ORE, "minecraft:lapis_ore"},
    {BlockType::DEEPSLATE_LAPIS_ORE, "minecraft:deepslate_lapis_ore"},
    {BlockType::DIAMOND_ORE, "minecraft:diamond_ore"},
    {BlockType::DEEPSLATE_DIAMOND_ORE, "minecraft:deepslate_diamond_ore"},
    {BlockType::NETHER_GOLD_ORE, "minecraft:nether_gold_ore"},
    {BlockType::NETHER_QUARTZ_ORE, "minecraft:nether_quartz_ore"},
    {BlockType::ANCIENT_DEBRIS, "minecraft:ancient_debris"},
    {BlockType::OAK_LOG, "minecraft:oak_log"},
    {BlockType::SPRUCE_LOG, "minecraft:spruce_log"},
    {BlockType::BIRCH_LOG, "minecraft:birch_log"},
    {BlockType::JUNGLE_LOG, "minecraft:jungle_log"},
    {BlockType::ACACIA_LOG, "minecraft:acacia_log"},
    {BlockType::DARK_OAK_LOG, "minecraft:dark_oak_log"},
    {BlockType::MANGROVE_LOG, "minecraft:mangrove_log"},
    {BlockType::CHERRY_LOG, "minecraft:cherry_log"},
    {BlockType::CRIMSON_STEM, "minecraft:crimson_stem"},
    {BlockType::WARPED_STEM, "minecraft:warped_stem"},
    {BlockType::GLASS, "minecraft:glass"},
    {BlockType::TINTED_GLASS, "minecraft:tinted_glass"},
    {BlockType::OAK_LEAVES, "minecraft:oak_leaves"},
    {BlockType::SPRUCE_LEAVES, "minecraft:spruce_leaves"},
    {BlockType::BIRCH_LEAVES, "minecraft:birch_leaves"},
    {BlockType::JUNGLE_LEAVES, "minecraft:jungle_leaves"},
    {BlockType::ACACIA_LEAVES, "minecraft:acacia_leaves"},
    {BlockType::DARK_OAK_LEAVES, "minecraft:dark_oak_leaves"},
    {BlockType::MANGROVE_LEAVES, "minecraft:mangrove_leaves"},
    {BlockType::CHERRY_LEAVES, "minecraft:cherry_leaves"},
    {BlockType::AZALEA_LEAVES, "minecraft:azalea_leaves"},
    {BlockType::FLOWERING_AZALEA_LEAVES, "minecraft:flowering_azalea_leaves"},
    {BlockType::WHITE_WOOL, "minecraft:white_wool"},
    {BlockType::BLACK_WOOL, "minecraft:black_wool"},
    {BlockType::DANDELION, "minecraft:dandelion"},
    {BlockType::POPPY, "minecraft:poppy"},
    // 1.21 blocks
    {BlockType::CRAFTER, "minecraft:crafter"},
    {BlockType::TRIAL_SPAWNER, "minecraft:trial_spawner"},
    {BlockType::VAULT, "minecraft:vault"},
    {BlockType::HEAVY_CORE, "minecraft:heavy_core"},
    // Copper family
    {BlockType::COPPER_BLOCK, "minecraft:copper_block"},
    {BlockType::EXPOSED_COPPER, "minecraft:exposed_copper"},
    {BlockType::WEATHERED_COPPER, "minecraft:weathered_copper"},
    {BlockType::OXIDIZED_COPPER, "minecraft:oxidized_copper"},
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

// Version availability data - blocks available in current compiled version
const std::unordered_set<BlockType> BlockRegistry::AVAILABLE_BLOCKS = []() {
    std::unordered_set<BlockType> blocks;
    
    // Always available blocks (base game)
    for (const auto& [type, _] : NAMES) {
        if (type == BlockType::UNKNOWN) continue;
        
        // Version-specific filtering
        #if MINECRAFT_VERSION >= 120100  // 1.20.1+
            blocks.insert(type);
        #elif MINECRAFT_VERSION >= 116500  // 1.16.5+
            // Exclude 1.17+ blocks like deepslate, copper, etc.
            if (!block_utils::is_caves_and_cliffs(type) && 
                !block_utils::is_trails_and_tales(type) && 
                !block_utils::is_trial_update(type)) {
                blocks.insert(type);
            }
        #else
            // Exclude all newer blocks for older versions
            if (!block_utils::is_caves_and_cliffs(type) && 
                !block_utils::is_trails_and_tales(type) && 
                !block_utils::is_trial_update(type)) {
                blocks.insert(type);
            }
        #endif
    }
    
    return blocks;
}();

// Default properties for unknown blocks
static const BlockProperties DEFAULT_PROPERTIES = {
    1.0f, 1.0f, 0, 0, true, false, false, false, false, false, false
};

const BlockProperties& BlockRegistry::get_properties(BlockType type) noexcept {
    if (!is_available(type)) {
        return DEFAULT_PROPERTIES;
    }
    
    auto it = PROPERTIES.find(type);
    return (it != PROPERTIES.end()) ? it->second : DEFAULT_PROPERTIES;
}

std::string_view BlockRegistry::get_name(BlockType type) noexcept {
    if (!is_available(type)) {
        return "minecraft:unknown";
    }
    
    auto it = NAMES.find(type);
    return (it != NAMES.end()) ? it->second : "minecraft:unknown";
}

BlockType BlockRegistry::from_name(std::string_view name) noexcept {
    auto it = NAME_TO_TYPE.find(name);
    if (it != NAME_TO_TYPE.end() && is_available(it->second)) {
        return it->second;
    }
    return BlockType::UNKNOWN;
}

bool BlockRegistry::is_valid(BlockType type) noexcept {
    return type != BlockType::UNKNOWN && 
           static_cast<std::uint16_t>(type) < MAX_BLOCK_ID &&
           is_available(type);
}

bool BlockRegistry::is_available(BlockType type) noexcept {
    return AVAILABLE_BLOCKS.find(type) != AVAILABLE_BLOCKS.end();
}

std::string_view BlockRegistry::get_version_string() noexcept {
    return VersionInfo::version_string;
}

size_t BlockRegistry::get_block_count() noexcept {
    return AVAILABLE_BLOCKS.size();
}

// Block utility functions
namespace block_utils {

bool is_replaceable(BlockType type) noexcept {
    switch (type) {
        case BlockType::AIR:
        case BlockType::WATER:
        case BlockType::LAVA:
        case BlockType::SHORT_GRASS:
        case BlockType::FERN:
        case BlockType::DEAD_BUSH:
        case BlockType::SEAGRASS:
        case BlockType::TALL_SEAGRASS:
            return true;
        default:
            return false;
    }
}

} // namespace block_utils

} // namespace world
} // namespace parallelstone