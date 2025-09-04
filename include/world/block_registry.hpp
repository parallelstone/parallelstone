#pragma once

#include "world/compile_time_blocks.hpp"
#include <unordered_map>
#include <unordered_set>
#include <string_view>

namespace parallelstone {
namespace world {

/**
 * @brief Compile-time block registry for the selected Minecraft version
 * 
 * This registry contains only the blocks available in the compiled version.
 * Use -DMINECRAFT_VERSION=121700 to select version 1.21.7
 */
class BlockRegistry {
public:
    /**
     * @brief Get block properties for a given block type
     */
    static const BlockProperties& get_properties(BlockType type) noexcept;
    
    /**
     * @brief Get the canonical name for a block type
     */
    static std::string_view get_name(BlockType type) noexcept;
    
    /**
     * @brief Get block type from canonical name
     */
    static BlockType from_name(std::string_view name) noexcept;
    
    /**
     * @brief Check if block type is valid for current version
     */
    static bool is_valid(BlockType type) noexcept;
    
    /**
     * @brief Check if block is available in current compiled version
     */
    static bool is_available(BlockType type) noexcept;
    
    /**
     * @brief Get the network protocol ID for transmission
     */
    static std::uint16_t get_protocol_id(BlockType type) noexcept {
        return static_cast<std::uint16_t>(type);
    }
    
    /**
     * @brief Create block type from protocol ID
     */
    static BlockType from_protocol_id(std::uint16_t protocol_id) noexcept {
        if (protocol_id >= MAX_BLOCK_ID) {
            return BlockType::UNKNOWN;
        }
        auto block_type = static_cast<BlockType>(protocol_id);
        return is_available(block_type) ? block_type : BlockType::UNKNOWN;
    }
    
    /**
     * @brief Get the current Minecraft version
     */
    static constexpr int get_version() noexcept {
        return MinecraftVersion::CURRENT;
    }
    
    /**
     * @brief Get version string
     */
    static std::string_view get_version_string() noexcept;
    
    /**
     * @brief Get total number of blocks in current version
     */
    static size_t get_block_count() noexcept;

private:
    static constexpr std::uint16_t MAX_BLOCK_ID = 1000;
    
    // Static registry data
    static const std::unordered_map<BlockType, BlockProperties> PROPERTIES;
    static const std::unordered_map<BlockType, std::string_view> NAMES;
    static const std::unordered_map<std::string_view, BlockType> NAME_TO_TYPE;
    
    // Version availability data
    static const std::unordered_set<BlockType> AVAILABLE_BLOCKS;
};

// Utility functions for common block categories
namespace block_utils {
    
    /**
     * @brief Check if block is a type of log
     */
    constexpr bool is_log(BlockType type) noexcept {
        return (type >= BlockType::OAK_LOG && type <= BlockType::STRIPPED_WARPED_HYPHAE);
    }
    
    /**
     * @brief Check if block is a type of ore
     */
    constexpr bool is_ore(BlockType type) noexcept {
        return (type >= BlockType::COAL_ORE && type <= BlockType::ANCIENT_DEBRIS);
    }
    
    /**
     * @brief Check if block is a liquid
     */
    constexpr bool is_liquid(BlockType type) noexcept {
        return type == BlockType::WATER || type == BlockType::LAVA;
    }
    
    /**
     * @brief Check if block is air or void
     */
    constexpr bool is_air(BlockType type) noexcept {
        return type == BlockType::AIR;
    }
    
    /**
     * @brief Check if block can be replaced (like air, water, tall grass)
     */
    bool is_replaceable(BlockType type) noexcept;
    
    /**
     * @brief Check if block is a wood plank variant
     */
    constexpr bool is_planks(BlockType type) noexcept {
        return type >= BlockType::OAK_PLANKS && type <= BlockType::WARPED_PLANKS;
    }
    
    /**
     * @brief Check if block is a leaves variant
     */
    constexpr bool is_leaves(BlockType type) noexcept {
        return type >= BlockType::OAK_LEAVES && type <= BlockType::FLOWERING_AZALEA_LEAVES;
    }
    
    /**
     * @brief Check if block is a flower
     */
    constexpr bool is_flower(BlockType type) noexcept {
        return type >= BlockType::DANDELION && type <= BlockType::PITCHER_PLANT;
    }
    
    /**
     * @brief Check if block is wool
     */
    constexpr bool is_wool(BlockType type) noexcept {
        return type >= BlockType::WHITE_WOOL && type <= BlockType::BLACK_WOOL;
    }
    
    /**
     * @brief Check if block is concrete
     */
    constexpr bool is_concrete(BlockType type) noexcept {
        return type >= BlockType::WHITE_CONCRETE && type <= BlockType::BLACK_CONCRETE;
    }
    
    /**
     * @brief Check if block is terracotta
     */
    constexpr bool is_terracotta(BlockType type) noexcept {
        return type >= BlockType::TERRACOTTA && type <= BlockType::BLACK_TERRACOTTA;
    }
    
    /**
     * @brief Check if block is a copper variant
     */
    constexpr bool is_copper(BlockType type) noexcept {
        return (type >= BlockType::COPPER_BLOCK && type <= BlockType::WAXED_OXIDIZED_CHISELED_COPPER) ||
               type == BlockType::COPPER_ORE || type == BlockType::DEEPSLATE_COPPER_ORE ||
               type == BlockType::RAW_COPPER_BLOCK;
    }
    
    /**
     * @brief Check if block is version-specific (1.17+)
     */
    constexpr bool is_caves_and_cliffs(BlockType type) noexcept {
        return supports_version(MinecraftVersion::MC_1_20_1) && (
            type >= BlockType::DEEPSLATE && type <= BlockType::CHISELED_DEEPSLATE ||
            type >= BlockType::COPPER_ORE && type <= BlockType::RAW_COPPER_BLOCK ||
            type >= BlockType::SCULK && type <= BlockType::CALIBRATED_SCULK_SENSOR
        );
    }
    
    /**
     * @brief Check if block is version-specific (1.20+)
     */
    constexpr bool is_trails_and_tales(BlockType type) noexcept {
        return supports_version(MinecraftVersion::MC_1_20_1) && (
            type == BlockType::CHERRY_LOG || type == BlockType::CHERRY_PLANKS ||
            type == BlockType::CHERRY_SAPLING || type == BlockType::CHERRY_LEAVES ||
            type == BlockType::BAMBOO_PLANKS || type == BlockType::SUSPICIOUS_SAND ||
            type == BlockType::SUSPICIOUS_GRAVEL || type == BlockType::TORCHFLOWER ||
            type == BlockType::PITCHER_PLANT
        );
    }
    
    /**
     * @brief Check if block is version-specific (1.21+)
     */
    constexpr bool is_trial_update(BlockType type) noexcept {
        return supports_version(MinecraftVersion::MC_1_21_1) && (
            type == BlockType::CRAFTER || type == BlockType::TRIAL_SPAWNER ||
            type == BlockType::VAULT || type == BlockType::HEAVY_CORE ||
            (type >= BlockType::COPPER_DOOR && type <= BlockType::WAXED_OXIDIZED_CHISELED_COPPER)
        );
    }
    
} // namespace block_utils

/**
 * @brief Compile-time version info
 */
struct VersionInfo {
    static constexpr int version = MinecraftVersion::CURRENT;
    static constexpr const char* version_string = 
#if MINECRAFT_VERSION == 120100
        "1.20.1";
#elif MINECRAFT_VERSION == 120400
        "1.20.4";
#elif MINECRAFT_VERSION == 121100
        "1.21.1";
#elif MINECRAFT_VERSION == 121300
        "1.21.3";
#elif MINECRAFT_VERSION == 121700
        "1.21.7";
#else
        "unknown";
#endif
};

} // namespace world
} // namespace parallelstone