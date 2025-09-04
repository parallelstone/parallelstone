#pragma once

#include "world/compile_time_blocks.hpp"
#include "utils/noise.hpp"
#include <vector>
#include <unordered_map>
#include <memory>
#include <string>

namespace parallelstone {
namespace world {

/**
 * @brief Comprehensive biome system for world generation
 * 
 * Implements Minecraft-compatible biome generation with temperature,
 * humidity, multi-noise systems, and biome-specific terrain features.
 */

// ==================== BIOME DEFINITIONS ====================

/**
 * @brief All Minecraft biome types (Java Edition 1.21.7)
 */
enum class BiomeType : std::uint8_t {
    // === OVERWORLD BIOMES ===
    
    // Offshore Biomes
    OCEAN = 0,
    DEEP_OCEAN = 1,
    WARM_OCEAN = 2,
    LUKEWARM_OCEAN = 3,
    COLD_OCEAN = 4,
    DEEP_LUKEWARM_OCEAN = 5,
    DEEP_COLD_OCEAN = 6,
    DEEP_FROZEN_OCEAN = 7,
    FROZEN_OCEAN = 8,
    MUSHROOM_FIELDS = 9,
    
    // Highland Biomes
    MOUNTAINS = 10,
    WINDSWEPT_HILLS = 11,
    WINDSWEPT_FOREST = 12,
    WINDSWEPT_GRAVELLY_HILLS = 13,
    WINDSWEPT_SAVANNA = 14,
    JAGGED_PEAKS = 15,
    FROZEN_PEAKS = 16,
    STONY_PEAKS = 17,
    MEADOW = 18,
    GROVE = 19,
    SNOWY_SLOPES = 20,
    
    // Woodland Biomes
    FOREST = 21,
    FLOWER_FOREST = 22,
    BIRCH_FOREST = 23,
    DARK_FOREST = 24,
    OLD_GROWTH_BIRCH_FOREST = 25,
    OLD_GROWTH_PINE_TAIGA = 26,
    OLD_GROWTH_SPRUCE_TAIGA = 27,
    TAIGA = 28,
    SNOWY_TAIGA = 29,
    JUNGLE = 30,
    BAMBOO_JUNGLE = 31,
    SPARSE_JUNGLE = 32,
    
    // Wetland Biomes
    SWAMP = 33,
    MANGROVE_SWAMP = 34,
    RIVER = 35,
    FROZEN_RIVER = 36,
    BEACH = 37,
    SNOWY_BEACH = 38,
    STONY_SHORE = 39,
    
    // Flatland Biomes
    PLAINS = 40,
    SUNFLOWER_PLAINS = 41,
    SNOWY_PLAINS = 42,
    ICE_SPIKES = 43,
    
    // Arid-land Biomes
    DESERT = 44,
    SAVANNA = 45,
    SAVANNA_PLATEAU = 46,
    BADLANDS = 47,
    WOODED_BADLANDS = 48,
    ERODED_BADLANDS = 49,
    
    // Cave Biomes
    DEEP_DARK = 50,
    DRIPSTONE_CAVES = 51,
    LUSH_CAVES = 52,
    
    // Special
    THE_VOID = 53,
    
    // === NETHER BIOMES ===
    NETHER_WASTES = 54,
    SOUL_SAND_VALLEY = 55,
    CRIMSON_FOREST = 56,
    WARPED_FOREST = 57,
    BASALT_DELTAS = 58,
    
    // === END BIOMES ===
    THE_END = 59,
    END_HIGHLANDS = 60,
    END_MIDLANDS = 61,
    SMALL_END_ISLANDS = 62,
    END_BARRENS = 63,
    
    // Total: 64 biomes
    INVALID = 255
};

/**
 * @brief Biome categories for generation logic
 */
enum class BiomeCategory : std::uint8_t {
    OFFSHORE,    // Oceans, deep waters
    HIGHLAND,    // Mountains, peaks, hills
    WOODLAND,    // Forests, taigas, jungles
    WETLAND,     // Swamps, rivers, beaches
    FLATLAND,    // Plains, tundra
    ARIDLAND,    // Deserts, savannas, badlands
    CAVE,        // Underground biomes
    NETHER,      // Nether dimension
    END,         // End dimension
    SPECIAL      // Void, mushroom fields
};

/**
 * @brief Biome characteristics and generation parameters
 */
struct BiomeData {
    BiomeType type;
    BiomeCategory category;
    std::string name;
    
    // Climate parameters
    float temperature;      // 0.0-2.0 (0.0 = frozen, 1.0 = temperate, 2.0 = hot)
    float humidity;         // 0.0-1.0 (0.0 = arid, 1.0 = humid)
    float downfall;         // 0.0-1.0 (precipitation amount)
    
    // Terrain parameters
    float base_height;      // Base terrain height (-2.0 to 2.0)
    float height_variation; // Terrain height variation (0.0-2.0)
    float terrain_scale;    // Terrain feature scaling
    
    // Generation flags
    bool has_precipitation; // Can rain/snow
    bool freezes_water;     // Water freezes to ice
    bool allows_snow;       // Snow can fall/accumulate
    bool is_cold;          // Temperature < 0.15
    bool is_dry;           // Humidity < 0.3
    bool is_ocean;         // Ocean-type biome
    bool is_nether;        // Nether biome
    bool is_end;           // End biome
    
    // Block composition
    BlockType surface_block;     // Top layer block
    BlockType subsurface_block;  // Second layer block
    BlockType stone_block;       // Deep layer block
    BlockType fluid_block;       // Water/lava for this biome
    
    // Vegetation and features
    std::vector<BlockType> vegetation_blocks;
    float vegetation_density;    // 0.0-1.0
    float tree_density;         // 0.0-1.0
    std::vector<std::string> structure_types;
    
    // Color tinting (RGB multipliers)
    struct {
        float grass_r, grass_g, grass_b;
        float foliage_r, foliage_g, foliage_b;
        float water_r, water_g, water_b;
        float sky_r, sky_g, sky_b;
    } colors;
};

// ==================== BIOME GENERATION SYSTEM ====================

/**
 * @brief Multi-noise biome generation system
 * 
 * Uses layered noise functions to generate realistic biome distributions
 * following Minecraft's generation patterns.
 */
class BiomeGenerator {
public:
    /**
     * @brief Initialize biome generator with seed
     */
    explicit BiomeGenerator(std::uint64_t seed);
    
    /**
     * @brief Generate biome at world coordinates
     */
    BiomeType generate_biome(std::int32_t x, std::int32_t z, DimensionType dimension = DimensionType::OVERWORLD);
    
    /**
     * @brief Generate biome map for chunk (16x16)
     */
    std::vector<std::vector<BiomeType>> generate_chunk_biomes(std::int32_t chunk_x, std::int32_t chunk_z, 
                                                             DimensionType dimension = DimensionType::OVERWORLD);
    
    /**
     * @brief Get biome data for specific biome type
     */
    const BiomeData& get_biome_data(BiomeType biome) const;
    
    /**
     * @brief Get temperature at specific coordinates
     */
    float get_temperature(std::int32_t x, std::int32_t z) const;
    
    /**
     * @brief Get humidity at specific coordinates
     */
    float get_humidity(std::int32_t x, std::int32_t z) const;
    
    /**
     * @brief Check if precipitation occurs at coordinates
     */
    bool has_precipitation_at(std::int32_t x, std::int32_t y, std::int32_t z) const;
    
    /**
     * @brief Get biome transition factor between two points
     */
    float get_transition_factor(std::int32_t x1, std::int32_t z1, std::int32_t x2, std::int32_t z2) const;

private:
    std::uint64_t seed_;
    
    // Noise generators for different aspects
    std::unique_ptr<utils::PerlinNoise> temperature_noise_;
    std::unique_ptr<utils::PerlinNoise> humidity_noise_;
    std::unique_ptr<utils::PerlinNoise> elevation_noise_;
    std::unique_ptr<utils::PerlinNoise> weirdness_noise_;
    std::unique_ptr<utils::PerlinNoise> erosion_noise_;
    std::unique_ptr<utils::PerlinNoise> ridge_noise_;
    
    // Biome data registry
    std::unordered_map<BiomeType, BiomeData> biome_data_;
    
    // Biome selection logic
    BiomeType select_overworld_biome(float temperature, float humidity, float elevation, 
                                   float erosion, float weirdness, float ridge) const;
    BiomeType select_nether_biome(std::int32_t x, std::int32_t z) const;
    BiomeType select_end_biome(std::int32_t x, std::int32_t z) const;
    
    void initialize_biome_data();
    void initialize_noise_generators();
};

// ==================== BIOME-AWARE TERRAIN GENERATION ====================

/**
 * @brief Biome-aware terrain generator
 * 
 * Generates terrain features based on biome characteristics,
 * including height maps, surface composition, and vegetation.
 */
class BiomeTerrainGenerator {
public:
    /**
     * @brief Initialize with biome generator
     */
    explicit BiomeTerrainGenerator(std::shared_ptr<BiomeGenerator> biome_gen);
    
    /**
     * @brief Generate terrain for chunk with biome awareness
     */
    void generate_terrain(Chunk& chunk, std::int32_t chunk_x, std::int32_t chunk_z,
                         DimensionType dimension = DimensionType::OVERWORLD);
    
    /**
     * @brief Generate surface layer for specific biome
     */
    void generate_surface_layer(Chunk& chunk, BiomeType biome, 
                              std::uint8_t x, std::uint8_t z, std::int32_t surface_y);
    
    /**
     * @brief Generate vegetation for biome
     */
    void generate_vegetation(Chunk& chunk, BiomeType biome, 
                           std::uint8_t x, std::int32_t y, std::uint8_t z);
    
    /**
     * @brief Generate biome-specific structures
     */
    void generate_biome_features(Chunk& chunk, const std::vector<std::vector<BiomeType>>& biome_map,
                               std::int32_t chunk_x, std::int32_t chunk_z);
    
    /**
     * @brief Calculate terrain height for biome
     */
    std::int32_t calculate_terrain_height(BiomeType biome, std::int32_t x, std::int32_t z) const;

private:
    std::int32_t chunk_x, chunk_z; // Current chunk coordinates for terrain generation
    
    void generate_basic_terrain_column(Chunk& chunk, std::uint8_t x, std::uint8_t z, 
                                     std::int32_t surface_height, BiomeType biome);
    void generate_caves(Chunk& chunk, const std::vector<std::vector<BiomeType>>& biome_map,
                       std::int32_t chunk_x, std::int32_t chunk_z);
    void place_simple_tree(Chunk& chunk, std::uint8_t x, std::int32_t y, std::uint8_t z,
                          BlockType log_type, BlockType leaf_type, int height);
    void place_nether_fungus(Chunk& chunk, std::uint8_t x, std::int32_t y, std::uint8_t z,
                           BlockType stem_type, BlockType wart_type, int height);
    void generate_ice_spike(Chunk& chunk, std::uint8_t x, std::int32_t y, std::uint8_t z, int height);
    void generate_cactus(Chunk& chunk, std::uint8_t x, std::int32_t y, std::uint8_t z, int height);
    void generate_giant_mushroom(Chunk& chunk, std::uint8_t x, std::int32_t y, std::uint8_t z, BlockType mushroom_type);
    void generate_bamboo_grove(Chunk& chunk, std::uint8_t x, std::int32_t y, std::uint8_t z);
    void generate_main_end_island(Chunk& chunk, std::uint8_t x, std::uint8_t z, std::int32_t world_x, std::int32_t world_z);
    void generate_outer_end_islands(Chunk& chunk, std::uint8_t x, std::uint8_t z, std::int32_t world_x, std::int32_t world_z);
    
    std::shared_ptr<BiomeGenerator> biome_generator_;
    
    // Terrain noise generators
    std::unique_ptr<utils::PerlinNoise> height_noise_;
    std::unique_ptr<utils::PerlinNoise> surface_noise_;
    std::unique_ptr<utils::PerlinNoise> cave_noise_;
    std::unique_ptr<utils::PerlinNoise> ore_noise_;
    
    void generate_overworld_terrain(Chunk& chunk, const std::vector<std::vector<BiomeType>>& biome_map,
                                  std::int32_t chunk_x, std::int32_t chunk_z);
    void generate_nether_terrain(Chunk& chunk, std::int32_t chunk_x, std::int32_t chunk_z);
    void generate_end_terrain(Chunk& chunk, std::int32_t chunk_x, std::int32_t chunk_z);
    
    void place_trees(Chunk& chunk, BiomeType biome, std::uint8_t x, std::int32_t y, std::uint8_t z);
    void place_grass_and_flowers(Chunk& chunk, BiomeType biome, std::uint8_t x, std::int32_t y, std::uint8_t z);
    void generate_ore_veins(Chunk& chunk, BiomeType biome);
};

// ==================== BIOME TRANSITION SYSTEM ====================

/**
 * @brief Handles smooth transitions between different biomes
 */
class BiomeTransitionSystem {
public:
    /**
     * @brief Initialize transition system
     */
    explicit BiomeTransitionSystem(std::shared_ptr<BiomeGenerator> biome_gen);
    
    /**
     * @brief Apply biome transitions to chunk boundaries
     */
    void apply_transitions(Chunk& chunk, std::int32_t chunk_x, std::int32_t chunk_z);
    
    /**
     * @brief Get blended biome characteristics at specific point
     */
    BiomeData get_blended_biome_data(std::int32_t x, std::int32_t z, float blend_radius = 4.0f) const;
    
    /**
     * @brief Check if coordinates are in transition zone
     */
    bool is_transition_zone(std::int32_t x, std::int32_t z) const;

private:
    struct PairHash {
        std::size_t operator()(const std::pair<BlockType, BlockType>& p) const;
    };

    std::shared_ptr<BiomeGenerator> biome_generator_;
    
    struct TransitionRule {
        BiomeType from_biome;
        BiomeType to_biome;
        BiomeType transition_biome;
        float min_distance;
        float blend_strength;
    };
    
    std::vector<TransitionRule> transition_rules_;
    
    void initialize_transition_rules();
    float calculate_biome_influence(BiomeType biome, std::int32_t center_x, std::int32_t center_z,
                                  std::int32_t sample_x, std::int32_t sample_z, float radius) const;
    
    BiomeType determine_transition_biome(const std::vector<std::vector<BiomeType>>& biome_map, 
                                       int center_x, int center_z, BiomeType current_biome);
    void apply_biome_transition_to_column(Chunk& chunk, std::uint8_t x, std::uint8_t z,
                                        BiomeType from_biome, BiomeType to_biome);
    void apply_terrain_blending(Chunk& chunk, std::uint8_t x, std::uint8_t z,
                               const std::vector<std::vector<BiomeType>>& biome_map,
                               int center_x, int center_z);
    void apply_height_blending(Chunk& chunk, std::uint8_t x, std::uint8_t z, float blend_factor);
    void handle_special_transitions(Chunk& chunk, std::uint8_t x, std::uint8_t z, 
                                  std::int32_t surface_y, BiomeType from_biome, BiomeType to_biome);
    BlockType blend_surface_blocks(BlockType block1, BlockType block2);
};

// ==================== UTILITY FUNCTIONS ====================

/**
 * @brief Get biome category from biome type
 */
BiomeCategory get_biome_category(BiomeType biome);

/**
 * @brief Check if biome supports specific block type
 */
bool biome_supports_block(BiomeType biome, BlockType block);

/**
 * @brief Calculate biome-specific color tinting
 */
struct ColorTint {
    std::uint8_t r, g, b;
};

ColorTint calculate_grass_color(BiomeType biome, float temperature, float humidity);
ColorTint calculate_foliage_color(BiomeType biome, float temperature, float humidity);
ColorTint calculate_water_color(BiomeType biome);

/**
 * @brief Get spawn-appropriate biomes for dimension
 */
std::vector<BiomeType> get_spawn_biomes(DimensionType dimension);

/**
 * @brief Check biome compatibility for structure generation
 */
bool can_generate_structure(const std::string& structure_type, BiomeType biome);

} // namespace world
} // namespace parallelstone