#pragma once

#include "chunk_section.hpp"
#include "compile_time_blocks.hpp"
#include "biome_system.hpp"
#include <random>
#include <memory>
#include <vector>
#include <functional>

namespace parallelstone {
namespace world {

/**
 * @brief Enhanced world generator with comprehensive biome-aware generation
 * 
 * Integrates the biome system for realistic terrain generation that considers
 * biome characteristics, temperature, humidity, and transition zones.
 */
class WorldGenerator {
public:
    /**
     * @brief Initialize world generator with seed and biome system
     */
    explicit WorldGenerator(std::uint64_t seed);
    
    /**
     * @brief Set biome generator for terrain generation
     */
    void set_biome_generator(std::shared_ptr<BiomeGenerator> biome_gen);
    
    /**
     * @brief Generate chunk terrain for specific dimension
     */
    virtual void generate_chunk(Chunk& chunk, std::int32_t chunk_x, std::int32_t chunk_z,
                              DimensionType dimension = DimensionType::OVERWORLD);
    
    /**
     * @brief Generate biome for specific coordinates
     */
    virtual BiomeType get_biome(std::int32_t x, std::int32_t z, 
                               DimensionType dimension = DimensionType::OVERWORLD);
    
    /**
     * @brief Calculate spawn point coordinates
     */
    virtual std::tuple<std::int32_t, std::int32_t, std::int32_t> get_spawn_point(
        DimensionType dimension = DimensionType::OVERWORLD);
    
    /**
     * @brief Check if structure can spawn at location
     */
    virtual bool can_spawn_structure(const std::string& structure_type, 
                                   std::int32_t x, std::int32_t z, BiomeType biome);
    
    /**
     * @brief Get world seed
     */
    std::uint64_t get_seed() const { return seed_; }

protected:
    std::uint64_t seed_;
    std::shared_ptr<BiomeGenerator> biome_generator_;
    std::shared_ptr<BiomeTerrainGenerator> terrain_generator_;
    std::shared_ptr<BiomeTransitionSystem> transition_system_;
    
    std::mt19937_64 rng_;
    
    virtual void generate_overworld_chunk(Chunk& chunk, std::int32_t chunk_x, std::int32_t chunk_z);
    virtual void generate_nether_chunk(Chunk& chunk, std::int32_t chunk_x, std::int32_t chunk_z);
    virtual void generate_end_chunk(Chunk& chunk, std::int32_t chunk_x, std::int32_t chunk_z);
};

/**
 * @brief Overworld generator with realistic biome-based terrain
 */
class OverworldGenerator : public WorldGenerator {
public:
    explicit OverworldGenerator(std::uint64_t seed);
    
    void generate_chunk(Chunk& chunk, std::int32_t chunk_x, std::int32_t chunk_z,
                       DimensionType dimension = DimensionType::OVERWORLD) override;
    
    BiomeType get_biome(std::int32_t x, std::int32_t z,
                       DimensionType dimension = DimensionType::OVERWORLD) override;
    
    std::tuple<std::int32_t, std::int32_t, std::int32_t> get_spawn_point(
        DimensionType dimension = DimensionType::OVERWORLD) override;

private:
    void generate_structures(Chunk& chunk, std::int32_t chunk_x, std::int32_t chunk_z,
                           const std::vector<std::vector<BiomeType>>& biome_map);
    void generate_villages(Chunk& chunk, std::int32_t chunk_x, std::int32_t chunk_z, BiomeType biome);
    void generate_dungeons(Chunk& chunk, std::int32_t chunk_x, std::int32_t chunk_z);
    void generate_ores(Chunk& chunk, std::int32_t chunk_x, std::int32_t chunk_z);
    
    std::vector<BiomeType> suitable_spawn_biomes_ = {
        BiomeType::PLAINS, BiomeType::FOREST, BiomeType::TAIGA,
        BiomeType::BIRCH_FOREST, BiomeType::MOUNTAINS
    };
};

/**
 * @brief Nether generator with biome-specific terrain
 */
class NetherGenerator : public WorldGenerator {
public:
    explicit NetherGenerator(std::uint64_t seed);
    
    void generate_chunk(Chunk& chunk, std::int32_t chunk_x, std::int32_t chunk_z,
                       DimensionType dimension = DimensionType::NETHER) override;
    
    BiomeType get_biome(std::int32_t x, std::int32_t z,
                       DimensionType dimension = DimensionType::NETHER) override;
    
    std::tuple<std::int32_t, std::int32_t, std::int32_t> get_spawn_point(
        DimensionType dimension = DimensionType::NETHER) override;

private:
    void generate_nether_structures(Chunk& chunk, std::int32_t chunk_x, std::int32_t chunk_z);
    void generate_nether_fortresses(Chunk& chunk, std::int32_t chunk_x, std::int32_t chunk_z);
    void generate_bastion_remnants(Chunk& chunk, std::int32_t chunk_x, std::int32_t chunk_z);
};

/**
 * @brief End generator with floating islands
 */
class EndGenerator : public WorldGenerator {
public:
    explicit EndGenerator(std::uint64_t seed);
    
    void generate_chunk(Chunk& chunk, std::int32_t chunk_x, std::int32_t chunk_z,
                       DimensionType dimension = DimensionType::END) override;
    
    BiomeType get_biome(std::int32_t x, std::int32_t z,
                       DimensionType dimension = DimensionType::END) override;
    
    std::tuple<std::int32_t, std::int32_t, std::int32_t> get_spawn_point(
        DimensionType dimension = DimensionType::END) override;

private:
    void generate_end_structures(Chunk& chunk, std::int32_t chunk_x, std::int32_t chunk_z);
    void generate_end_cities(Chunk& chunk, std::int32_t chunk_x, std::int32_t chunk_z);
    void generate_chorus_trees(Chunk& chunk, std::int32_t chunk_x, std::int32_t chunk_z);
};

/**
 * @brief World generator factory for creating dimension-specific generators
 */
class WorldGeneratorFactory {
public:
    /**
     * @brief Create generator for specific dimension
     */
    static std::unique_ptr<WorldGenerator> create_generator(DimensionType dimension, std::uint64_t seed);
    
    /**
     * @brief Create overworld generator with custom biome configuration
     */
    static std::unique_ptr<OverworldGenerator> create_overworld_generator(std::uint64_t seed);
    
    /**
     * @brief Create nether generator
     */
    static std::unique_ptr<NetherGenerator> create_nether_generator(std::uint64_t seed);
    
    /**
     * @brief Create end generator
     */
    static std::unique_ptr<EndGenerator> create_end_generator(std::uint64_t seed);
};

/**
 * @brief Structure generator for placing world structures
 */
class StructureGenerator {
public:
    explicit StructureGenerator(std::uint64_t seed);
    
    /**
     * @brief Check if structure should generate at location
     */
    bool should_generate_structure(const std::string& structure_type,
                                 std::int32_t chunk_x, std::int32_t chunk_z, BiomeType biome);
    
    /**
     * @brief Generate structure at specific location
     */
    void generate_structure(Chunk& chunk, const std::string& structure_type,
                          std::int32_t x, std::int32_t y, std::int32_t z);

private:
    std::uint64_t seed_;
    std::mt19937_64 rng_;
    
    void generate_village(Chunk& chunk, std::int32_t x, std::int32_t y, std::int32_t z);
    void generate_desert_pyramid(Chunk& chunk, std::int32_t x, std::int32_t y, std::int32_t z);
    void generate_jungle_temple(Chunk& chunk, std::int32_t x, std::int32_t y, std::int32_t z);
    void generate_witch_hut(Chunk& chunk, std::int32_t x, std::int32_t y, std::int32_t z);
    void generate_ocean_monument(Chunk& chunk, std::int32_t x, std::int32_t y, std::int32_t z);
    void generate_woodland_mansion(Chunk& chunk, std::int32_t x, std::int32_t y, std::int32_t z);
    
    // Nether structures
    void generate_nether_fortress(Chunk& chunk, std::int32_t x, std::int32_t y, std::int32_t z);
    void generate_bastion_remnant(Chunk& chunk, std::int32_t x, std::int32_t y, std::int32_t z);
    
    // End structures
    void generate_end_city(Chunk& chunk, std::int32_t x, std::int32_t y, std::int32_t z);
    void generate_end_ship(Chunk& chunk, std::int32_t x, std::int32_t y, std::int32_t z);
};

/**
 * @brief Utility functions for world generation
 */
namespace generation_utils {
    /**
     * @brief Calculate distance between two points
     */
    inline double distance_2d(double x1, double z1, double x2, double z2) {
        double dx = x2 - x1;
        double dz = z2 - z1;
        return std::sqrt(dx * dx + dz * dz);
    }
    
    /**
     * @brief Check if point is within chunk bounds
     */
    inline bool is_within_chunk(std::int32_t x, std::int32_t z, std::int32_t chunk_x, std::int32_t chunk_z) {
        std::int32_t min_x = chunk_x * 16;
        std::int32_t max_x = min_x + 15;
        std::int32_t min_z = chunk_z * 16;
        std::int32_t max_z = min_z + 15;
        
        return x >= min_x && x <= max_x && z >= min_z && z <= max_z;
    }
    
    /**
     * @brief Convert world coordinates to chunk coordinates
     */
    inline std::pair<std::int32_t, std::int32_t> world_to_chunk(std::int32_t x, std::int32_t z) {
        return {x >> 4, z >> 4};
    }
    
    /**
     * @brief Convert world coordinates to chunk-relative coordinates
     */
    inline std::pair<std::uint8_t, std::uint8_t> world_to_chunk_relative(std::int32_t x, std::int32_t z) {
        return {static_cast<std::uint8_t>(x & 15), static_cast<std::uint8_t>(z & 15)};
    }
    
    /**
     * @brief Generate random integer in range
     */
    template<typename Generator>
    inline int random_int(Generator& gen, int min, int max) {
        std::uniform_int_distribution<int> dist(min, max);
        return dist(gen);
    }
    
    /**
     * @brief Generate random double in range
     */
    template<typename Generator>
    inline double random_double(Generator& gen, double min = 0.0, double max = 1.0) {
        std::uniform_real_distribution<double> dist(min, max);
        return dist(gen);
    }
}

} // namespace world
} // namespace parallelstone