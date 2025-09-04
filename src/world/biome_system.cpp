#include "world/biome_system.hpp"
#include "world/chunk_section.hpp"
#include "utils/noise.hpp"
#include <algorithm>
#include <cmath>
#include <random>
#include <spdlog/spdlog.h>

namespace parallelstone {
namespace world {

// ==================== BIOME DATA INITIALIZATION ====================

BiomeGenerator::BiomeGenerator(std::uint64_t seed) : seed_(seed) {
    initialize_noise_generators();
    initialize_biome_data();
    spdlog::info("Initialized biome generator with seed: {}", seed);
}

void BiomeGenerator::initialize_noise_generators() {
    // Multi-scale noise for realistic biome distribution
    temperature_noise_ = std::make_unique<utils::PerlinNoise>(seed_);
    humidity_noise_ = std::make_unique<utils::PerlinNoise>(seed_ + 1);
    elevation_noise_ = std::make_unique<utils::PerlinNoise>(seed_ + 2);
    weirdness_noise_ = std::make_unique<utils::PerlinNoise>(seed_ + 3);
    erosion_noise_ = std::make_unique<utils::PerlinNoise>(seed_ + 4);
    ridge_noise_ = std::make_unique<utils::PerlinNoise>(seed_ + 5);
}

void BiomeGenerator::initialize_biome_data() {
    // === OVERWORLD BIOMES ===
    
    // Ocean Biomes
    biome_data_[BiomeType::OCEAN] = {
        BiomeType::OCEAN, BiomeCategory::OFFSHORE, "Ocean",
        0.5f, 0.5f, 0.5f,  // temperature, humidity, downfall
        -1.0f, 0.1f, 1.0f,  // base_height, height_variation, terrain_scale
        true, false, false, false, false, true, false, false,  // flags
        BlockType::GRAVEL, BlockType::DIRT, BlockType::STONE, BlockType::WATER,  // blocks
        {}, 0.0f, 0.0f, {},  // vegetation
        {0.4f, 0.9f, 0.4f, 0.4f, 0.9f, 0.4f, 0.3f, 0.5f, 1.0f, 0.5f, 0.8f, 1.0f}  // colors
    };
    
    biome_data_[BiomeType::DEEP_OCEAN] = {
        BiomeType::DEEP_OCEAN, BiomeCategory::OFFSHORE, "Deep Ocean",
        0.5f, 0.5f, 0.5f,
        -1.8f, 0.1f, 1.0f,
        true, false, false, false, false, true, false, false,
        BlockType::GRAVEL, BlockType::DIRT, BlockType::STONE, BlockType::WATER,
        {}, 0.0f, 0.0f, {},
        {0.4f, 0.9f, 0.4f, 0.4f, 0.9f, 0.4f, 0.1f, 0.3f, 0.8f, 0.5f, 0.8f, 1.0f}
    };
    
    biome_data_[BiomeType::WARM_OCEAN] = {
        BiomeType::WARM_OCEAN, BiomeCategory::OFFSHORE, "Warm Ocean",
        0.8f, 0.5f, 0.5f,
        -1.0f, 0.1f, 1.0f,
        true, false, false, false, false, true, false, false,
        BlockType::SAND, BlockType::SAND, BlockType::SANDSTONE, BlockType::WATER,
        {}, 0.0f, 0.0f, {},
        {0.4f, 0.9f, 0.4f, 0.4f, 0.9f, 0.4f, 0.4f, 0.8f, 1.0f, 0.7f, 0.9f, 1.0f}
    };
    
    // Plains Biomes
    biome_data_[BiomeType::PLAINS] = {
        BiomeType::PLAINS, BiomeCategory::FLATLAND, "Plains",
        0.8f, 0.4f, 0.4f,
        0.125f, 0.05f, 1.0f,
        true, false, false, false, false, false, false, false,
        BlockType::GRASS_BLOCK, BlockType::DIRT, BlockType::STONE, BlockType::WATER,
        {BlockType::SHORT_GRASS, BlockType::DANDELION, BlockType::POPPY}, 0.6f, 0.05f, {"village"},
        {0.5f, 1.0f, 0.5f, 0.5f, 1.0f, 0.5f, 0.3f, 0.5f, 1.0f, 0.7f, 0.8f, 1.0f}
    };
    
    biome_data_[BiomeType::SUNFLOWER_PLAINS] = {
        BiomeType::SUNFLOWER_PLAINS, BiomeCategory::FLATLAND, "Sunflower Plains",
        0.8f, 0.4f, 0.4f,
        0.125f, 0.05f, 1.0f,
        true, false, false, false, false, false, false, false,
        BlockType::GRASS_BLOCK, BlockType::DIRT, BlockType::STONE, BlockType::WATER,
        {BlockType::SHORT_GRASS, BlockType::SUNFLOWER, BlockType::DANDELION}, 0.8f, 0.02f, {},
        {0.5f, 1.0f, 0.5f, 0.5f, 1.0f, 0.5f, 0.3f, 0.5f, 1.0f, 0.7f, 0.8f, 1.0f}
    };
    
    // Forest Biomes
    biome_data_[BiomeType::FOREST] = {
        BiomeType::FOREST, BiomeCategory::WOODLAND, "Forest",
        0.7f, 0.8f, 0.8f,
        0.1f, 0.2f, 1.0f,
        true, false, false, false, false, false, false, false,
        BlockType::GRASS_BLOCK, BlockType::DIRT, BlockType::STONE, BlockType::WATER,
        {BlockType::SHORT_GRASS, BlockType::FERN, BlockType::DANDELION}, 0.7f, 0.5f, {"village"},
        {0.4f, 0.9f, 0.4f, 0.4f, 0.8f, 0.4f, 0.3f, 0.5f, 1.0f, 0.7f, 0.8f, 1.0f}
    };
    
    biome_data_[BiomeType::BIRCH_FOREST] = {
        BiomeType::BIRCH_FOREST, BiomeCategory::WOODLAND, "Birch Forest",
        0.6f, 0.6f, 0.6f,
        0.1f, 0.2f, 1.0f,
        true, false, false, false, false, false, false, false,
        BlockType::GRASS_BLOCK, BlockType::DIRT, BlockType::STONE, BlockType::WATER,
        {BlockType::SHORT_GRASS, BlockType::DANDELION}, 0.6f, 0.4f, {},
        {0.4f, 0.9f, 0.4f, 0.4f, 0.8f, 0.4f, 0.3f, 0.5f, 1.0f, 0.7f, 0.8f, 1.0f}
    };
    
    biome_data_[BiomeType::DARK_FOREST] = {
        BiomeType::DARK_FOREST, BiomeCategory::WOODLAND, "Dark Forest",
        0.7f, 0.8f, 0.8f,
        0.1f, 0.2f, 1.0f,
        true, false, false, false, false, false, false, false,
        BlockType::GRASS_BLOCK, BlockType::DIRT, BlockType::STONE, BlockType::WATER,
        {BlockType::FERN, BlockType::LARGE_FERN, BlockType::RED_MUSHROOM}, 0.9f, 0.8f, {"woodland_mansion"},
        {0.3f, 0.7f, 0.3f, 0.3f, 0.6f, 0.3f, 0.3f, 0.5f, 1.0f, 0.6f, 0.7f, 0.9f}
    };
    
    // Mountain Biomes
    biome_data_[BiomeType::MOUNTAINS] = {
        BiomeType::MOUNTAINS, BiomeCategory::HIGHLAND, "Mountains",
        0.2f, 0.3f, 0.3f,
        1.0f, 0.5f, 1.5f,
        true, false, false, false, false, false, false, false,
        BlockType::GRASS_BLOCK, BlockType::DIRT, BlockType::STONE, BlockType::WATER,
        {BlockType::SHORT_GRASS}, 0.3f, 0.1f, {},
        {0.4f, 0.8f, 0.4f, 0.4f, 0.8f, 0.4f, 0.3f, 0.5f, 1.0f, 0.6f, 0.7f, 1.0f}
    };
    
    biome_data_[BiomeType::JAGGED_PEAKS] = {
        BiomeType::JAGGED_PEAKS, BiomeCategory::HIGHLAND, "Jagged Peaks",
        -0.7f, 0.4f, 0.9f,
        1.4f, 0.8f, 2.0f,
        true, true, true, true, false, false, false, false,
        BlockType::SNOW_BLOCK, BlockType::SNOW_BLOCK, BlockType::STONE, BlockType::ICE,
        {}, 0.0f, 0.0f, {},
        {0.8f, 0.9f, 0.8f, 0.8f, 0.9f, 0.8f, 0.8f, 0.9f, 1.0f, 0.8f, 0.9f, 1.0f}
    };
    
    // Desert Biomes
    biome_data_[BiomeType::DESERT] = {
        BiomeType::DESERT, BiomeCategory::ARIDLAND, "Desert",
        2.0f, 0.0f, 0.0f,
        0.125f, 0.05f, 1.0f,
        false, false, false, false, true, false, false, false,
        BlockType::SAND, BlockType::SAND, BlockType::SANDSTONE, BlockType::WATER,
        {BlockType::DEAD_BUSH, BlockType::CACTUS}, 0.1f, 0.0f, {"village", "desert_pyramid"},
        {0.9f, 0.9f, 0.5f, 0.9f, 0.9f, 0.5f, 0.3f, 0.5f, 1.0f, 0.9f, 0.8f, 0.6f}
    };
    
    // Taiga Biomes
    biome_data_[BiomeType::TAIGA] = {
        BiomeType::TAIGA, BiomeCategory::WOODLAND, "Taiga",
        0.25f, 0.8f, 0.8f,
        0.2f, 0.2f, 1.0f,
        true, false, false, false, false, false, false, false,
        BlockType::GRASS_BLOCK, BlockType::DIRT, BlockType::STONE, BlockType::WATER,
        {BlockType::FERN, BlockType::LARGE_FERN}, 0.6f, 0.6f, {"village"},
        {0.4f, 0.8f, 0.4f, 0.4f, 0.7f, 0.4f, 0.3f, 0.5f, 1.0f, 0.6f, 0.7f, 1.0f}
    };
    
    biome_data_[BiomeType::SNOWY_TAIGA] = {
        BiomeType::SNOWY_TAIGA, BiomeCategory::WOODLAND, "Snowy Taiga",
        -0.5f, 0.4f, 0.4f,
        0.2f, 0.2f, 1.0f,
        true, true, true, true, false, false, false, false,
        BlockType::GRASS_BLOCK, BlockType::DIRT, BlockType::STONE, BlockType::ICE,
        {BlockType::FERN}, 0.4f, 0.3f, {"village"},
        {0.6f, 0.8f, 0.6f, 0.6f, 0.8f, 0.6f, 0.7f, 0.8f, 1.0f, 0.8f, 0.8f, 1.0f}
    };
    
    // Jungle Biomes
    biome_data_[BiomeType::JUNGLE] = {
        BiomeType::JUNGLE, BiomeCategory::WOODLAND, "Jungle",
        0.95f, 0.9f, 0.9f,
        0.1f, 0.2f, 1.0f,
        true, false, false, false, false, false, false, false,
        BlockType::GRASS_BLOCK, BlockType::DIRT, BlockType::STONE, BlockType::WATER,
        {BlockType::FERN, BlockType::LARGE_FERN, BlockType::VINE}, 0.95f, 0.8f, {"jungle_temple"},
        {0.3f, 0.9f, 0.3f, 0.2f, 0.8f, 0.2f, 0.3f, 0.5f, 1.0f, 0.7f, 0.8f, 1.0f}
    };
    
    biome_data_[BiomeType::BAMBOO_JUNGLE] = {
        BiomeType::BAMBOO_JUNGLE, BiomeCategory::WOODLAND, "Bamboo Jungle",
        0.95f, 0.9f, 0.9f,
        0.1f, 0.2f, 1.0f,
        true, false, false, false, false, false, false, false,
        BlockType::GRASS_BLOCK, BlockType::DIRT, BlockType::STONE, BlockType::WATER,
        {BlockType::BAMBOO, BlockType::FERN}, 0.9f, 0.7f, {},
        {0.3f, 0.9f, 0.3f, 0.2f, 0.8f, 0.2f, 0.3f, 0.5f, 1.0f, 0.7f, 0.8f, 1.0f}
    };
    
    // Swamp Biomes
    biome_data_[BiomeType::SWAMP] = {
        BiomeType::SWAMP, BiomeCategory::WETLAND, "Swamp",
        0.8f, 0.9f, 0.9f,
        -0.2f, 0.1f, 1.0f,
        true, false, false, false, false, false, false, false,
        BlockType::GRASS_BLOCK, BlockType::DIRT, BlockType::CLAY, BlockType::WATER,
        {BlockType::LILY_PAD, BlockType::BLUE_ORCHID, BlockType::VINE}, 0.8f, 0.6f, {"swamp_hut"},
        {0.4f, 0.6f, 0.2f, 0.4f, 0.6f, 0.2f, 0.2f, 0.3f, 0.2f, 0.6f, 0.7f, 0.8f}
    };
    
    biome_data_[BiomeType::MANGROVE_SWAMP] = {
        BiomeType::MANGROVE_SWAMP, BiomeCategory::WETLAND, "Mangrove Swamp",
        0.8f, 0.9f, 0.9f,
        -0.2f, 0.1f, 1.0f,
        true, false, false, false, false, false, false, false,
        BlockType::MUD, BlockType::MUD, BlockType::DEEPSLATE, BlockType::WATER,
        {BlockType::MANGROVE_PROPAGULE}, 0.9f, 0.8f, {},
        {0.4f, 0.7f, 0.3f, 0.4f, 0.7f, 0.3f, 0.2f, 0.4f, 0.3f, 0.6f, 0.7f, 0.8f}
    };
    
    // Snowy Biomes
    biome_data_[BiomeType::SNOWY_PLAINS] = {
        BiomeType::SNOWY_PLAINS, BiomeCategory::FLATLAND, "Snowy Plains",
        0.0f, 0.5f, 0.5f,
        0.125f, 0.05f, 1.0f,
        true, true, true, true, false, false, false, false,
        BlockType::GRASS_BLOCK, BlockType::DIRT, BlockType::STONE, BlockType::ICE,
        {}, 0.1f, 0.0f, {"village", "igloo"},
        {0.8f, 0.9f, 0.8f, 0.8f, 0.9f, 0.8f, 0.8f, 0.9f, 1.0f, 0.8f, 0.9f, 1.0f}
    };
    
    biome_data_[BiomeType::ICE_SPIKES] = {
        BiomeType::ICE_SPIKES, BiomeCategory::FLATLAND, "Ice Spikes",
        0.0f, 0.5f, 0.5f,
        0.425f, 0.45f, 1.0f,
        true, true, true, true, false, false, false, false,
        BlockType::SNOW_BLOCK, BlockType::DIRT, BlockType::STONE, BlockType::ICE,
        {}, 0.0f, 0.0f, {},
        {0.8f, 0.9f, 0.8f, 0.8f, 0.9f, 0.8f, 0.8f, 0.9f, 1.0f, 0.8f, 0.9f, 1.0f}
    };
    
    // === NETHER BIOMES ===
    
    biome_data_[BiomeType::NETHER_WASTES] = {
        BiomeType::NETHER_WASTES, BiomeCategory::NETHER, "Nether Wastes",
        2.0f, 0.0f, 0.0f,
        0.1f, 0.2f, 1.0f,
        false, false, false, false, true, false, true, false,
        BlockType::NETHERRACK, BlockType::NETHERRACK, BlockType::NETHERRACK, BlockType::LAVA,
        {BlockType::NETHER_WART}, 0.1f, 0.0f, {"nether_fortress"},
        {1.0f, 0.5f, 0.5f, 1.0f, 0.5f, 0.5f, 1.0f, 0.3f, 0.0f, 0.8f, 0.4f, 0.4f}
    };
    
    biome_data_[BiomeType::SOUL_SAND_VALLEY] = {
        BiomeType::SOUL_SAND_VALLEY, BiomeCategory::NETHER, "Soul Sand Valley",
        2.0f, 0.0f, 0.0f,
        0.1f, 0.2f, 1.0f,
        false, false, false, false, true, false, true, false,
        BlockType::SOUL_SAND, BlockType::SOUL_SAND, BlockType::SOUL_SOIL, BlockType::LAVA,
        {BlockType::SOUL_FIRE}, 0.05f, 0.0f, {"bastion_remnant"},
        {0.5f, 0.8f, 1.0f, 0.5f, 0.8f, 1.0f, 1.0f, 0.3f, 0.0f, 0.4f, 0.6f, 0.8f}
    };
    
    biome_data_[BiomeType::CRIMSON_FOREST] = {
        BiomeType::CRIMSON_FOREST, BiomeCategory::NETHER, "Crimson Forest",
        2.0f, 0.0f, 0.0f,
        0.1f, 0.2f, 1.0f,
        false, false, false, false, true, false, true, false,
        BlockType::CRIMSON_NYLIUM, BlockType::NETHERRACK, BlockType::NETHERRACK, BlockType::LAVA,
        {BlockType::CRIMSON_FUNGUS, BlockType::CRIMSON_ROOTS}, 0.6f, 0.4f, {},
        {1.0f, 0.2f, 0.2f, 1.0f, 0.2f, 0.2f, 1.0f, 0.3f, 0.0f, 0.8f, 0.4f, 0.4f}
    };
    
    biome_data_[BiomeType::WARPED_FOREST] = {
        BiomeType::WARPED_FOREST, BiomeCategory::NETHER, "Warped Forest",
        2.0f, 0.0f, 0.0f,
        0.1f, 0.2f, 1.0f,
        false, false, false, false, true, false, true, false,
        BlockType::WARPED_NYLIUM, BlockType::NETHERRACK, BlockType::NETHERRACK, BlockType::LAVA,
        {BlockType::WARPED_FUNGUS, BlockType::WARPED_ROOTS}, 0.6f, 0.4f, {},
        {0.2f, 1.0f, 0.8f, 0.2f, 1.0f, 0.8f, 1.0f, 0.3f, 0.0f, 0.4f, 0.8f, 0.6f}
    };
    
    biome_data_[BiomeType::BASALT_DELTAS] = {
        BiomeType::BASALT_DELTAS, BiomeCategory::NETHER, "Basalt Deltas",
        2.0f, 0.0f, 0.0f,
        0.1f, 0.2f, 1.0f,
        false, false, false, false, true, false, true, false,
        BlockType::BASALT, BlockType::BLACKSTONE, BlockType::BASALT, BlockType::LAVA,
        {}, 0.0f, 0.0f, {},
        {0.4f, 0.4f, 0.4f, 0.4f, 0.4f, 0.4f, 1.0f, 0.3f, 0.0f, 0.5f, 0.5f, 0.5f}
    };
    
    // === END BIOMES ===
    
    biome_data_[BiomeType::THE_END] = {
        BiomeType::THE_END, BiomeCategory::END, "The End",
        0.5f, 0.5f, 0.5f,
        0.1f, 0.2f, 1.0f,
        false, false, false, false, true, false, false, true,
        BlockType::END_STONE, BlockType::END_STONE, BlockType::END_STONE, BlockType::WATER,
        {}, 0.0f, 0.0f, {},
        {0.8f, 0.8f, 0.5f, 0.8f, 0.8f, 0.5f, 0.3f, 0.5f, 1.0f, 0.0f, 0.0f, 0.2f}
    };
    
    biome_data_[BiomeType::END_HIGHLANDS] = {
        BiomeType::END_HIGHLANDS, BiomeCategory::END, "End Highlands",
        0.5f, 0.5f, 0.5f,
        1.0f, 0.2f, 1.0f,
        false, false, false, false, true, false, false, true,
        BlockType::END_STONE, BlockType::END_STONE, BlockType::END_STONE, BlockType::WATER,
        {BlockType::CHORUS_PLANT}, 0.1f, 0.05f, {"end_city"},
        {0.8f, 0.8f, 0.5f, 0.8f, 0.8f, 0.5f, 0.3f, 0.5f, 1.0f, 0.0f, 0.0f, 0.2f}
    };
    
    spdlog::info("Initialized {} biome types with complete data", biome_data_.size());
}

// ==================== BIOME GENERATION IMPLEMENTATION ====================

BiomeType BiomeGenerator::generate_biome(std::int32_t x, std::int32_t z, DimensionType dimension) {
    switch (dimension) {
        case DimensionType::OVERWORLD: {
            // Multi-noise biome generation (Minecraft 1.18+ system)
            float temp = get_temperature(x, z);
            float humidity = get_humidity(x, z);
            float elevation = elevation_noise_->sample(x * 0.0001f, z * 0.0001f) * 0.5f + 0.5f;
            float erosion = erosion_noise_->sample(x * 0.0002f, z * 0.0002f) * 0.5f + 0.5f;
            float weirdness = weirdness_noise_->sample(x * 0.00015f, z * 0.00015f) * 0.5f + 0.5f;
            float ridge = ridge_noise_->sample(x * 0.0003f, z * 0.0003f) * 0.5f + 0.5f;
            
            return select_overworld_biome(temp, humidity, elevation, erosion, weirdness, ridge);
        }
        case DimensionType::NETHER:
            return select_nether_biome(x, z);
        case DimensionType::END:
            return select_end_biome(x, z);
        default:
            return BiomeType::PLAINS;
    }
}

std::vector<std::vector<BiomeType>> BiomeGenerator::generate_chunk_biomes(std::int32_t chunk_x, std::int32_t chunk_z, DimensionType dimension) {
    std::vector<std::vector<BiomeType>> biome_map(16, std::vector<BiomeType>(16));
    
    for (int x = 0; x < 16; x++) {
        for (int z = 0; z < 16; z++) {
            std::int32_t world_x = chunk_x * 16 + x;
            std::int32_t world_z = chunk_z * 16 + z;
            biome_map[x][z] = generate_biome(world_x, world_z, dimension);
        }
    }
    
    return biome_map;
}

float BiomeGenerator::get_temperature(std::int32_t x, std::int32_t z) const {
    // Multi-octave noise for realistic temperature variation
    float temp = 0.0f;
    temp += temperature_noise_->sample(x * 0.00005f, z * 0.00005f) * 0.7f;  // Large scale
    temp += temperature_noise_->sample(x * 0.0002f, z * 0.0002f) * 0.2f;    // Medium scale
    temp += temperature_noise_->sample(x * 0.001f, z * 0.001f) * 0.1f;      // Small scale
    
    // Normalize to 0.0-1.0 range and apply temperature curve
    temp = (temp + 1.0f) * 0.5f;
    temp = std::pow(temp, 1.2f); // Slightly favor warmer temperatures
    
    // Add latitude-based temperature gradient (optional)
    float latitude_factor = 1.0f - std::abs(z * 0.000001f);
    temp *= std::max(0.1f, latitude_factor);
    
    return std::clamp(temp, 0.0f, 1.0f);
}

float BiomeGenerator::get_humidity(std::int32_t x, std::int32_t z) const {
    // Multi-octave noise for humidity
    float humidity = 0.0f;
    humidity += humidity_noise_->sample(x * 0.0001f, z * 0.0001f) * 0.6f;   // Large scale
    humidity += humidity_noise_->sample(x * 0.0005f, z * 0.0005f) * 0.3f;   // Medium scale
    humidity += humidity_noise_->sample(x * 0.002f, z * 0.002f) * 0.1f;     // Small scale
    
    // Normalize to 0.0-1.0 range
    humidity = (humidity + 1.0f) * 0.5f;
    
    return std::clamp(humidity, 0.0f, 1.0f);
}

BiomeType BiomeGenerator::select_overworld_biome(float temperature, float humidity, float elevation, 
                                                float erosion, float weirdness, float ridge) const {
    // Simplified Minecraft 1.18+ biome selection logic
    // This is a condensed version - full implementation would have more complex rules
    
    // High elevation areas (mountains)
    if (elevation > 0.8f) {
        if (temperature < 0.15f) {
            return (ridge > 0.6f) ? BiomeType::JAGGED_PEAKS : BiomeType::FROZEN_PEAKS;
        } else if (temperature < 0.5f) {
            return BiomeType::STONY_PEAKS;
        } else {
            return BiomeType::MOUNTAINS;
        }
    }
    
    // Water areas
    if (elevation < 0.3f && erosion < 0.4f) {
        if (temperature > 0.8f) {
            return BiomeType::WARM_OCEAN;
        } else if (temperature < 0.2f) {
            return (elevation < 0.1f) ? BiomeType::DEEP_FROZEN_OCEAN : BiomeType::FROZEN_OCEAN;
        } else {
            return (elevation < 0.15f) ? BiomeType::DEEP_OCEAN : BiomeType::OCEAN;
        }
    }
    
    // Extreme biomes based on temperature and humidity
    if (temperature > 1.5f && humidity < 0.2f) {
        return BiomeType::DESERT;
    }
    
    if (temperature < 0.15f) {
        if (humidity > 0.7f) {
            return BiomeType::SNOWY_TAIGA;
        } else {
            return (weirdness > 0.7f) ? BiomeType::ICE_SPIKES : BiomeType::SNOWY_PLAINS;
        }
    }
    
    // Jungle conditions
    if (temperature > 0.9f && humidity > 0.8f) {
        return (weirdness > 0.6f) ? BiomeType::BAMBOO_JUNGLE : BiomeType::JUNGLE;
    }
    
    // Swamp conditions
    if (humidity > 0.85f && elevation < 0.4f && temperature > 0.6f) {
        return (weirdness > 0.5f) ? BiomeType::MANGROVE_SWAMP : BiomeType::SWAMP;
    }
    
    // Forest conditions
    if (humidity > 0.6f && temperature > 0.3f && temperature < 0.8f) {
        if (weirdness > 0.7f) {
            return BiomeType::DARK_FOREST;
        } else if (temperature < 0.6f) {
            return BiomeType::BIRCH_FOREST;
        } else {
            return BiomeType::FOREST;
        }
    }
    
    // Taiga conditions
    if (temperature > 0.15f && temperature < 0.5f && humidity > 0.5f) {
        return BiomeType::TAIGA;
    }
    
    // Default to plains with variants
    if (weirdness > 0.8f) {
        return BiomeType::SUNFLOWER_PLAINS;
    }
    
    return BiomeType::PLAINS;
}

BiomeType BiomeGenerator::select_nether_biome(std::int32_t x, std::int32_t z) const {
    // 3D noise for Nether biomes (using z as pseudo-y for 3D effect)
    float noise1 = temperature_noise_->sample(x * 0.0008f, z * 0.0008f);
    float noise2 = humidity_noise_->sample(x * 0.0012f, z * 0.0012f);
    float noise3 = elevation_noise_->sample(x * 0.0006f, z * 0.0006f);
    
    // Combine noises for biome selection
    float combined = noise1 * 0.5f + noise2 * 0.3f + noise3 * 0.2f;
    
    if (combined < -0.4f) {
        return BiomeType::SOUL_SAND_VALLEY;
    } else if (combined < -0.1f) {
        return BiomeType::CRIMSON_FOREST;
    } else if (combined < 0.2f) {
        return BiomeType::NETHER_WASTES;
    } else if (combined < 0.5f) {
        return BiomeType::WARPED_FOREST;
    } else {
        return BiomeType::BASALT_DELTAS;
    }
}

BiomeType BiomeGenerator::select_end_biome(std::int32_t x, std::int32_t z) const {
    // Distance from origin for End biome selection
    float distance = std::sqrt(static_cast<float>(x * x + z * z));
    
    if (distance < 1000.0f) {
        return BiomeType::THE_END; // Central island
    } else if (distance < 10000.0f) {
        // Use noise for variation in outer islands
        float noise = elevation_noise_->sample(x * 0.001f, z * 0.001f);
        return (noise > 0.2f) ? BiomeType::END_HIGHLANDS : BiomeType::END_MIDLANDS;
    } else {
        return BiomeType::SMALL_END_ISLANDS;
    }
}

const BiomeData& BiomeGenerator::get_biome_data(BiomeType biome) const {
    auto it = biome_data_.find(biome);
    if (it != biome_data_.end()) {
        return it->second;
    }
    // Return plains as fallback
    return biome_data_.at(BiomeType::PLAINS);
}

bool BiomeGenerator::has_precipitation_at(std::int32_t x, std::int32_t y, std::int32_t z) const {
    BiomeType biome = generate_biome(x, z);
    const BiomeData& data = get_biome_data(biome);
    
    if (!data.has_precipitation) {
        return false;
    }
    
    // Check if above sea level and not in cave
    if (y < 63 || y > 255) {
        return false;
    }
    
    // Use noise to add variation to precipitation
    float precipitation_noise = temperature_noise_->sample(x * 0.01f, z * 0.01f);
    float threshold = data.downfall * 0.8f; // 80% base chance
    
    return precipitation_noise < threshold;
}

// ==================== UTILITY FUNCTIONS ====================

BiomeCategory get_biome_category(BiomeType biome) {
    // This would be a lookup table in practice
    switch (biome) {
        case BiomeType::OCEAN:
        case BiomeType::DEEP_OCEAN:
        case BiomeType::WARM_OCEAN:
        case BiomeType::LUKEWARM_OCEAN:
        case BiomeType::COLD_OCEAN:
        case BiomeType::DEEP_LUKEWARM_OCEAN:
        case BiomeType::DEEP_COLD_OCEAN:
        case BiomeType::DEEP_FROZEN_OCEAN:
        case BiomeType::FROZEN_OCEAN:
        case BiomeType::MUSHROOM_FIELDS:
            return BiomeCategory::OFFSHORE;
            
        case BiomeType::MOUNTAINS:
        case BiomeType::WINDSWEPT_HILLS:
        case BiomeType::JAGGED_PEAKS:
        case BiomeType::FROZEN_PEAKS:
        case BiomeType::STONY_PEAKS:
            return BiomeCategory::HIGHLAND;
            
        case BiomeType::FOREST:
        case BiomeType::BIRCH_FOREST:
        case BiomeType::DARK_FOREST:
        case BiomeType::TAIGA:
        case BiomeType::SNOWY_TAIGA:
        case BiomeType::JUNGLE:
        case BiomeType::BAMBOO_JUNGLE:
            return BiomeCategory::WOODLAND;
            
        case BiomeType::SWAMP:
        case BiomeType::MANGROVE_SWAMP:
        case BiomeType::RIVER:
        case BiomeType::FROZEN_RIVER:
        case BiomeType::BEACH:
        case BiomeType::SNOWY_BEACH:
        case BiomeType::STONY_SHORE:
            return BiomeCategory::WETLAND;
            
        case BiomeType::PLAINS:
        case BiomeType::SUNFLOWER_PLAINS:
        case BiomeType::SNOWY_PLAINS:
        case BiomeType::ICE_SPIKES:
            return BiomeCategory::FLATLAND;
            
        case BiomeType::DESERT:
        case BiomeType::SAVANNA:
        case BiomeType::BADLANDS:
            return BiomeCategory::ARIDLAND;
            
        case BiomeType::NETHER_WASTES:
        case BiomeType::SOUL_SAND_VALLEY:
        case BiomeType::CRIMSON_FOREST:
        case BiomeType::WARPED_FOREST:
        case BiomeType::BASALT_DELTAS:
            return BiomeCategory::NETHER;
            
        case BiomeType::THE_END:
        case BiomeType::END_HIGHLANDS:
        case BiomeType::END_MIDLANDS:
        case BiomeType::SMALL_END_ISLANDS:
        case BiomeType::END_BARRENS:
            return BiomeCategory::END;
            
        default:
            return BiomeCategory::SPECIAL;
    }
}

ColorTint calculate_grass_color(BiomeType biome, float temperature, float humidity) {
    // Minecraft grass color calculation based on biome
    std::uint8_t base_r = static_cast<std::uint8_t>(91 + (1.0f - temperature) * 164);
    std::uint8_t base_g = static_cast<std::uint8_t>(183 + humidity * 72);
    std::uint8_t base_b = static_cast<std::uint8_t>(91 + (1.0f - temperature) * 164 * 0.3f);
    
    // Special biome overrides
    switch (biome) {
        case BiomeType::SWAMP:
            return {106, 112, 85};
        case BiomeType::DARK_FOREST:
            return {40, 100, 40};
        case BiomeType::BADLANDS:
            return {144, 129, 77};
        default:
            return {base_r, base_g, base_b};
    }
}

ColorTint calculate_foliage_color(BiomeType biome, float temperature, float humidity) {
    // Similar to grass but different base values
    std::uint8_t base_r = static_cast<std::uint8_t>(82 + (1.0f - temperature) * 173);
    std::uint8_t base_g = static_cast<std::uint8_t>(174 + humidity * 81);
    std::uint8_t base_b = static_cast<std::uint8_t>(82 + (1.0f - temperature) * 173 * 0.2f);
    
    // Special biome overrides
    switch (biome) {
        case BiomeType::SWAMP:
            return {102, 108, 81};
        case BiomeType::BIRCH_FOREST:
            return {128, 167, 85};
        case BiomeType::DARK_FOREST:
            return {35, 90, 35};
        default:
            return {base_r, base_g, base_b};
    }
}

std::vector<BiomeType> get_spawn_biomes(DimensionType dimension) {
    switch (dimension) {
        case DimensionType::OVERWORLD:
            return {
                BiomeType::PLAINS, BiomeType::FOREST, BiomeType::TAIGA,
                BiomeType::BIRCH_FOREST, BiomeType::MOUNTAINS
            };
        case DimensionType::NETHER:
            return {BiomeType::NETHER_WASTES};
        case DimensionType::END:
            return {BiomeType::THE_END};
        default:
            return {BiomeType::PLAINS};
    }
}

} // namespace world
} // namespace parallelstone