#include "world/world_generator.hpp"
#include "world/block_registry.hpp"
#include <algorithm>
#include <cmath>
#include <spdlog/spdlog.h>

namespace parallelstone {
namespace world {

// ==================== NOISE GENERATOR ====================

NoiseGenerator::NoiseGenerator(std::uint64_t seed) : rng_(seed) {
    // Initialize permutation table for Perlin noise
    permutation_.resize(512);
    
    // Fill with values 0-255
    for (int i = 0; i < 256; i++) {
        permutation_[i] = i;
    }
    
    // Shuffle using the seeded RNG
    std::shuffle(permutation_.begin(), permutation_.begin() + 256, rng_);
    
    // Duplicate for easy wrapping
    for (int i = 0; i < 256; i++) {
        permutation_[256 + i] = permutation_[i];
    }
}

double NoiseGenerator::noise_2d(double x, double z, double frequency, int octaves) const {
    double value = 0.0;
    double amplitude = 1.0;
    double max_value = 0.0;
    
    for (int i = 0; i < octaves; i++) {
        value += perlin_noise(x * frequency, 0, z * frequency) * amplitude;
        max_value += amplitude;
        
        amplitude *= 0.5;
        frequency *= 2.0;
    }
    
    return value / max_value;
}

double NoiseGenerator::noise_3d(double x, double y, double z, double frequency, int octaves) const {
    double value = 0.0;
    double amplitude = 1.0;
    double max_value = 0.0;
    
    for (int i = 0; i < octaves; i++) {
        value += perlin_noise(x * frequency, y * frequency, z * frequency) * amplitude;
        max_value += amplitude;
        
        amplitude *= 0.5;
        frequency *= 2.0;
    }
    
    return value / max_value;
}

double NoiseGenerator::simplex_2d(double x, double z, double frequency) const {
    // Simplified 2D simplex noise approximation
    return noise_2d(x, z, frequency, 1);
}

double NoiseGenerator::ridged_noise(double x, double z, double frequency, int octaves) const {
    double value = 0.0;
    double amplitude = 1.0;
    double max_value = 0.0;
    
    for (int i = 0; i < octaves; i++) {
        double n = std::abs(perlin_noise(x * frequency, 0, z * frequency));
        n = 1.0 - n; // Invert for ridged effect
        n = n * n; // Square for sharper ridges
        
        value += n * amplitude;
        max_value += amplitude;
        
        amplitude *= 0.5;
        frequency *= 2.0;
    }
    
    return value / max_value;
}

double NoiseGenerator::fade(double t) const {
    return t * t * t * (t * (t * 6 - 15) + 10);
}

double NoiseGenerator::lerp(double t, double a, double b) const {
    return a + t * (b - a);
}

double NoiseGenerator::grad(int hash, double x, double y, double z) const {
    int h = hash & 15;
    double u = h < 8 ? x : y;
    double v = h < 4 ? y : h == 12 || h == 14 ? x : z;
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

double NoiseGenerator::perlin_noise(double x, double y, double z) const {
    int X = static_cast<int>(std::floor(x)) & 255;
    int Y = static_cast<int>(std::floor(y)) & 255;
    int Z = static_cast<int>(std::floor(z)) & 255;
    
    x -= std::floor(x);
    y -= std::floor(y);
    z -= std::floor(z);
    
    double u = fade(x);
    double v = fade(y);
    double w = fade(z);
    
    int A = permutation_[X] + Y;
    int AA = permutation_[A] + Z;
    int AB = permutation_[A + 1] + Z;
    int B = permutation_[X + 1] + Y;
    int BA = permutation_[B] + Z;
    int BB = permutation_[B + 1] + Z;
    
    return lerp(w, lerp(v, lerp(u, grad(permutation_[AA], x, y, z),
                                   grad(permutation_[BA], x - 1, y, z)),
                           lerp(u, grad(permutation_[AB], x, y - 1, z),
                                   grad(permutation_[BB], x - 1, y - 1, z))),
                   lerp(v, lerp(u, grad(permutation_[AA + 1], x, y, z - 1),
                                   grad(permutation_[BA + 1], x - 1, y, z - 1)),
                           lerp(u, grad(permutation_[AB + 1], x, y - 1, z - 1),
                                   grad(permutation_[BB + 1], x - 1, y - 1, z - 1))));
}

// ==================== STRUCTURE GENERATOR ====================

StructureGenerator::StructureGenerator(std::uint64_t seed) : rng_(seed) {}

bool StructureGenerator::should_generate_structure(std::int32_t chunk_x, std::int32_t chunk_z, BiomeType biome) const {
    // Simple structure generation probability based on biome
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    
    switch (biome) {
        case BiomeType::PLAINS:
            return dist(rng_) < 0.01; // 1% chance for villages
        case BiomeType::DESERT:
            return dist(rng_) < 0.005; // 0.5% chance for desert temples
        case BiomeType::FOREST:
            return dist(rng_) < 0.002; // 0.2% chance for witch huts
        default:
            return false;
    }
}

std::vector<StructureGenerator::StructureInfo> StructureGenerator::generate_structures(
    std::int32_t chunk_x, std::int32_t chunk_z, BiomeType biome) const {
    
    std::vector<StructureInfo> structures;
    
    if (!should_generate_structure(chunk_x, chunk_z, biome)) {
        return structures;
    }
    
    // Generate a simple structure based on biome
    StructureInfo structure;
    structure.name = "simple_structure";
    structure.chunk_x = chunk_x;
    structure.chunk_z = chunk_z;
    structure.biome = biome;
    
    // Add some basic blocks (example: small house)
    std::uniform_int_distribution<int> pos_dist(2, 13);
    int base_x = pos_dist(rng_);
    int base_z = pos_dist(rng_);
    int base_y = 80; // Approximate surface level
    
    // Simple 3x3x3 structure
    for (int dx = 0; dx < 3; dx++) {
        for (int dz = 0; dz < 3; dz++) {
            for (int dy = 0; dy < 3; dy++) {
                if (dy == 0 || dx == 0 || dx == 2 || dz == 0 || dz == 2) {
                    // Walls and floor
                    structure.blocks.push_back(biome == BiomeType::DESERT ? 
                                             BlockType::SANDSTONE : BlockType::COBBLESTONE);
                    structure.positions.push_back(base_x + dx);
                    structure.positions.push_back(base_y + dy);
                    structure.positions.push_back(base_z + dz);
                }
            }
        }
    }
    
    structures.push_back(structure);
    return structures;
}

// ==================== OVERWORLD GENERATOR ====================

OverworldGenerator::OverworldGenerator(std::uint64_t seed) : WorldGenerator(seed) {
    terrain_noise_ = std::make_unique<NoiseGenerator>(seed);
    cave_noise_ = std::make_unique<NoiseGenerator>(seed + 1);
    ore_noise_ = std::make_unique<NoiseGenerator>(seed + 2);
    biome_noise_ = std::make_unique<NoiseGenerator>(seed + 3);
    structure_generator_ = std::make_unique<StructureGenerator>(seed + 4);
    
    // Initialize biome data
    biome_data_[BiomeType::PLAINS] = {
        BiomeType::PLAINS, BlockType::GRASS_BLOCK, BlockType::DIRT, BlockType::STONE,
        0.8f, 0.4f, 68
    };
    
    biome_data_[BiomeType::DESERT] = {
        BiomeType::DESERT, BlockType::SAND, BlockType::SAND, BlockType::SANDSTONE,
        2.0f, 0.0f, 65
    };
    
    biome_data_[BiomeType::FOREST] = {
        BiomeType::FOREST, BlockType::GRASS_BLOCK, BlockType::DIRT, BlockType::STONE,
        0.7f, 0.8f, 70
    };
    
    biome_data_[BiomeType::OCEAN] = {
        BiomeType::OCEAN, BlockType::SAND, BlockType::DIRT, BlockType::STONE,
        0.5f, 0.5f, 45
    };
    
    biome_data_[BiomeType::MOUNTAINS] = {
        BiomeType::MOUNTAINS, BlockType::STONE, BlockType::STONE, BlockType::STONE,
        0.2f, 0.3f, 120
    };
}

void OverworldGenerator::generate_chunk(Chunk& chunk, std::int32_t chunk_x, std::int32_t chunk_z) {
    // Generate heightmap
    std::array<std::int32_t, 256> heightmap;
    generate_heightmap(chunk_x, chunk_z, heightmap);
    
    // Generate basic terrain
    generate_terrain(chunk, chunk_x, chunk_z, heightmap);
    
    // Generate caves
    generate_caves(chunk, chunk_x, chunk_z);
    
    // Generate ores
    generate_ores(chunk, chunk_x, chunk_z);
}

BiomeType OverworldGenerator::get_biome(std::int32_t x, std::int32_t z) {
    double temperature = biome_noise_->noise_2d(x * 0.01, z * 0.01, 1.0, 3);
    double humidity = biome_noise_->noise_2d(x * 0.01 + 1000, z * 0.01 + 1000, 1.0, 3);
    
    // Simple biome selection based on temperature and humidity
    if (temperature < -0.5) {
        return BiomeType::ICE_PLAINS;
    } else if (temperature > 0.8) {
        return humidity < 0.0 ? BiomeType::DESERT : BiomeType::JUNGLE;
    } else if (humidity < -0.3) {
        return BiomeType::PLAINS;
    } else if (humidity > 0.5) {
        return BiomeType::FOREST;
    } else {
        return BiomeType::PLAINS;
    }
}

void OverworldGenerator::populate_chunk(Chunk& chunk, std::int32_t chunk_x, std::int32_t chunk_z) {
    // Generate structures
    BiomeType biome = get_biome(chunk_x * 16 + 8, chunk_z * 16 + 8);
    auto structures = structure_generator_->generate_structures(chunk_x, chunk_z, biome);
    
    for (const auto& structure : structures) {
        // Place structure blocks
        for (std::size_t i = 0; i < structure.blocks.size(); i++) {
            std::int32_t x = structure.positions[i * 3];
            std::int32_t y = structure.positions[i * 3 + 1];
            std::int32_t z = structure.positions[i * 3 + 2];
            
            if (x >= 0 && x < 16 && z >= 0 && z < 16 && Chunk::is_valid_y(y)) {
                BlockState block_state(structure.blocks[i]);
                chunk.set_block(static_cast<std::uint8_t>(x), y, static_cast<std::uint8_t>(z), block_state);
            }
        }
    }
    
    // Generate decorations (trees, grass, etc.)
    generate_decorations(chunk, chunk_x, chunk_z, {});
}

std::tuple<std::int32_t, std::int32_t, std::int32_t> OverworldGenerator::get_spawn_point() {
    // Find a suitable spawn location near origin
    for (std::int32_t x = -16; x <= 16; x++) {
        for (std::int32_t z = -16; z <= 16; z++) {
            BiomeType biome = get_biome(x, z);
            if (biome == BiomeType::PLAINS || biome == BiomeType::FOREST) {
                double height_noise = terrain_noise_->noise_2d(x * 0.01, z * 0.01, 1.0, 4);
                std::int32_t y = static_cast<std::int32_t>(64 + height_noise * 20);
                return std::make_tuple(x, y + 2, z);
            }
        }
    }
    
    return std::make_tuple(0, 70, 0); // Default spawn
}

void OverworldGenerator::generate_heightmap(std::int32_t chunk_x, std::int32_t chunk_z, 
                                          std::array<std::int32_t, 256>& heightmap) {
    for (std::uint8_t x = 0; x < 16; x++) {
        for (std::uint8_t z = 0; z < 16; z++) {
            std::int32_t world_x = chunk_x * 16 + x;
            std::int32_t world_z = chunk_z * 16 + z;
            
            // Base terrain height
            double height_noise = terrain_noise_->noise_2d(world_x * 0.005, world_z * 0.005, 1.0, 4);
            
            // Biome-specific height modification
            BiomeType biome = get_biome(world_x, world_z);
            const auto& biome_data = get_biome_data(biome);
            
            std::int32_t height = static_cast<std::int32_t>(biome_data.surface_height + height_noise * 30);
            height = std::max(height, 40); // Minimum height above bedrock
            height = std::min(height, 200); // Maximum height
            
            heightmap[z * 16 + x] = height;
        }
    }
}

void OverworldGenerator::generate_terrain(Chunk& chunk, std::int32_t chunk_x, std::int32_t chunk_z,
                                        const std::array<std::int32_t, 256>& heightmap) {
    for (std::uint8_t x = 0; x < 16; x++) {
        for (std::uint8_t z = 0; z < 16; z++) {
            std::int32_t world_x = chunk_x * 16 + x;
            std::int32_t world_z = chunk_z * 16 + z;
            std::int32_t height = heightmap[z * 16 + x];
            
            BiomeType biome = get_biome(world_x, world_z);
            const auto& biome_data = get_biome_data(biome);
            
            for (std::int32_t y = -64; y <= height + 10; y++) {
                if (!Chunk::is_valid_y(y)) continue;
                
                BlockType block = BlockType::AIR;
                
                if (y <= 5) {
                    // Bedrock layer
                    block = BlockType::BEDROCK;
                } else if (y <= height - 5) {
                    // Stone layer
                    block = biome_data.stone_block;
                } else if (y <= height - 1) {
                    // Subsurface layer
                    block = biome_data.subsurface_block;
                } else if (y <= height) {
                    // Surface layer
                    block = biome_data.surface_block;
                } else if (y <= 62 && biome == BiomeType::OCEAN) {
                    // Water in ocean biomes
                    block = BlockType::WATER;
                }
                
                if (block != BlockType::AIR) {
                    BlockState block_state(block);
                    chunk.set_block(x, y, z, block_state);
                }
            }
        }
    }
}

void OverworldGenerator::generate_caves(Chunk& chunk, std::int32_t chunk_x, std::int32_t chunk_z) {
    for (std::uint8_t x = 0; x < 16; x++) {
        for (std::uint8_t z = 0; z < 16; z++) {
            std::int32_t world_x = chunk_x * 16 + x;
            std::int32_t world_z = chunk_z * 16 + z;
            
            for (std::int32_t y = 10; y < 60; y++) {
                if (!Chunk::is_valid_y(y)) continue;
                
                // Cave noise
                double cave_noise1 = cave_noise_->noise_3d(world_x * 0.02, y * 0.02, world_z * 0.02, 1.0, 3);
                double cave_noise2 = cave_noise_->noise_3d(world_x * 0.02 + 1000, y * 0.02 + 1000, world_z * 0.02 + 1000, 1.0, 3);
                
                // Create cave if both noise values are above threshold
                if (cave_noise1 > 0.6 && cave_noise2 > 0.6) {
                    BlockState current = chunk.get_block(x, y, z);
                    if (current.get_block_type() == BlockType::STONE ||
                        current.get_block_type() == BlockType::DIRT) {
                        
                        BlockState air_state(BlockType::AIR);
                        chunk.set_block(x, y, z, air_state);
                    }
                }
            }
        }
    }
}

void OverworldGenerator::generate_ores(Chunk& chunk, std::int32_t chunk_x, std::int32_t chunk_z) {
    std::mt19937 rng(seed_ ^ (chunk_x * 341873128712 + chunk_z * 132897987541));
    
    // Coal ore (common, higher levels)
    std::uniform_int_distribution<int> coal_attempts(8, 12);
    for (int i = 0; i < coal_attempts(rng); i++) {
        std::uniform_int_distribution<int> x_dist(1, 14);
        std::uniform_int_distribution<int> z_dist(1, 14);
        std::uniform_int_distribution<int> y_dist(40, 100);
        
        std::uint8_t x = static_cast<std::uint8_t>(x_dist(rng));
        std::uint8_t z = static_cast<std::uint8_t>(z_dist(rng));
        std::int32_t y = y_dist(rng);
        
        if (Chunk::is_valid_y(y)) {
            BlockState current = chunk.get_block(x, y, z);
            if (current.get_block_type() == BlockType::STONE) {
                BlockState ore_state(BlockType::COAL_ORE);
                chunk.set_block(x, y, z, ore_state);
            }
        }
    }
    
    // Iron ore (uncommon, medium levels)
    std::uniform_int_distribution<int> iron_attempts(4, 8);
    for (int i = 0; i < iron_attempts(rng); i++) {
        std::uniform_int_distribution<int> x_dist(1, 14);
        std::uniform_int_distribution<int> z_dist(1, 14);
        std::uniform_int_distribution<int> y_dist(20, 60);
        
        std::uint8_t x = static_cast<std::uint8_t>(x_dist(rng));
        std::uint8_t z = static_cast<std::uint8_t>(z_dist(rng));
        std::int32_t y = y_dist(rng);
        
        if (Chunk::is_valid_y(y)) {
            BlockState current = chunk.get_block(x, y, z);
            if (current.get_block_type() == BlockType::STONE) {
                BlockState ore_state(BlockType::IRON_ORE);
                chunk.set_block(x, y, z, ore_state);
            }
        }
    }
    
    // Diamond ore (rare, deep levels)
    std::uniform_int_distribution<int> diamond_attempts(0, 2);
    for (int i = 0; i < diamond_attempts(rng); i++) {
        std::uniform_int_distribution<int> x_dist(1, 14);
        std::uniform_int_distribution<int> z_dist(1, 14);
        std::uniform_int_distribution<int> y_dist(5, 16);
        
        std::uint8_t x = static_cast<std::uint8_t>(x_dist(rng));
        std::uint8_t z = static_cast<std::uint8_t>(z_dist(rng));
        std::int32_t y = y_dist(rng);
        
        if (Chunk::is_valid_y(y)) {
            BlockState current = chunk.get_block(x, y, z);
            if (current.get_block_type() == BlockType::STONE) {
                BlockState ore_state(BlockType::DIAMOND_ORE);
                chunk.set_block(x, y, z, ore_state);
            }
        }
    }
}

void OverworldGenerator::generate_decorations(Chunk& chunk, std::int32_t chunk_x, std::int32_t chunk_z,
                                            const std::array<std::int32_t, 256>& heightmap) {
    std::mt19937 rng(seed_ ^ (chunk_x * 341873128712 + chunk_z * 132897987541 + 999999));
    
    // Generate grass and flowers
    std::uniform_int_distribution<int> decoration_attempts(20, 40);
    for (int i = 0; i < decoration_attempts(rng); i++) {
        std::uniform_int_distribution<int> x_dist(0, 15);
        std::uniform_int_distribution<int> z_dist(0, 15);
        
        std::uint8_t x = static_cast<std::uint8_t>(x_dist(rng));
        std::uint8_t z = static_cast<std::uint8_t>(z_dist(rng));
        
        // Find surface
        for (std::int32_t y = 200; y > 60; y--) {
            if (!Chunk::is_valid_y(y)) continue;
            
            BlockState current = chunk.get_block(x, y, z);
            if (current.get_block_type() == BlockType::GRASS_BLOCK) {
                std::int32_t above_y = y + 1;
                if (Chunk::is_valid_y(above_y)) {
                    BlockState above = chunk.get_block(x, above_y, z);
                    if (above.get_block_type() == BlockType::AIR) {
                        // Place decoration
                        std::uniform_int_distribution<int> decoration_type(0, 10);
                        BlockType decoration = BlockType::AIR;
                        
                        if (decoration_type(rng) < 7) {
                            decoration = BlockType::SHORT_GRASS;
                        } else if (decoration_type(rng) < 9) {
                            decoration = BlockType::DANDELION;
                        } else {
                            decoration = BlockType::POPPY;
                        }
                        
                        if (decoration != BlockType::AIR && BlockRegistry::is_available(decoration)) {
                            BlockState decoration_state(decoration);
                            chunk.set_block(x, above_y, z, decoration_state);
                        }
                    }
                }
                break;
            }
        }
    }
}

const OverworldGenerator::BiomeData& OverworldGenerator::get_biome_data(BiomeType biome) const {
    auto it = biome_data_.find(biome);
    if (it != biome_data_.end()) {
        return it->second;
    }
    
    // Default to plains if biome not found
    return biome_data_.at(BiomeType::PLAINS);
}

// ==================== WORLD GENERATOR FACTORY ====================

std::unique_ptr<WorldGenerator> WorldGeneratorFactory::create_generator(DimensionType dimension, std::uint64_t seed) {
    switch (dimension) {
        case DimensionType::OVERWORLD:
            return std::make_unique<OverworldGenerator>(seed);
        case DimensionType::NETHER:
            return std::make_unique<NetherGenerator>(seed);
        case DimensionType::END:
            return std::make_unique<EndGenerator>(seed);
        default:
            spdlog::warn("Unknown dimension type {}, using Overworld generator", static_cast<int>(dimension));
            return std::make_unique<OverworldGenerator>(seed);
    }
}

// ==================== NETHER GENERATOR ====================

NetherGenerator::NetherGenerator(std::uint64_t seed) : WorldGenerator(seed) {
    terrain_noise_ = std::make_unique<NoiseGenerator>(seed);
    cave_noise_ = std::make_unique<NoiseGenerator>(seed + 1);
    biome_noise_ = std::make_unique<NoiseGenerator>(seed + 2);
}

void NetherGenerator::generate_chunk(Chunk& chunk, std::int32_t chunk_x, std::int32_t chunk_z) {
    generate_nether_terrain(chunk, chunk_x, chunk_z);
    generate_nether_caves(chunk, chunk_x, chunk_z);
    generate_lava_lakes(chunk, chunk_x, chunk_z);
}

BiomeType NetherGenerator::get_biome(std::int32_t x, std::int32_t z) {
    double biome_noise = biome_noise_->noise_2d(x * 0.01, z * 0.01, 1.0, 2);
    
    if (biome_noise < -0.3) {
        return BiomeType::SOUL_SAND_VALLEY;
    } else if (biome_noise < 0.0) {
        return BiomeType::CRIMSON_FOREST;
    } else if (biome_noise < 0.3) {
        return BiomeType::WARPED_FOREST;
    } else if (biome_noise < 0.6) {
        return BiomeType::BASALT_DELTAS;
    } else {
        return BiomeType::NETHER_WASTES;
    }
}

void NetherGenerator::populate_chunk(Chunk& chunk, std::int32_t chunk_x, std::int32_t chunk_z) {
    // Add nether-specific decorations
    // TODO: Implement nether wart, nether trees, etc.
}

std::tuple<std::int32_t, std::int32_t, std::int32_t> NetherGenerator::get_spawn_point() {
    return std::make_tuple(0, 70, 0);
}

void NetherGenerator::generate_nether_terrain(Chunk& chunk, std::int32_t chunk_x, std::int32_t chunk_z) {
    for (std::uint8_t x = 0; x < 16; x++) {
        for (std::uint8_t z = 0; z < 16; z++) {
            std::int32_t world_x = chunk_x * 16 + x;
            std::int32_t world_z = chunk_z * 16 + z;
            
            for (std::int32_t y = 0; y < 128; y++) {
                if (!Chunk::is_valid_y(y)) continue;
                
                BlockType block = BlockType::AIR;
                
                if (y <= 4) {
                    block = BlockType::BEDROCK;
                } else if (y >= 123) {
                    block = BlockType::BEDROCK;
                } else {
                    double terrain_noise = terrain_noise_->noise_3d(world_x * 0.02, y * 0.02, world_z * 0.02, 1.0, 3);
                    if (terrain_noise > 0.3) {
                        block = BlockType::NETHERRACK;
                    }
                }
                
                if (block != BlockType::AIR && BlockRegistry::is_available(block)) {
                    BlockState block_state(block);
                    chunk.set_block(x, y, z, block_state);
                }
            }
        }
    }
}

void NetherGenerator::generate_nether_caves(Chunk& chunk, std::int32_t chunk_x, std::int32_t chunk_z) {
    // Nether has more open caves
    for (std::uint8_t x = 0; x < 16; x++) {
        for (std::uint8_t z = 0; z < 16; z++) {
            std::int32_t world_x = chunk_x * 16 + x;
            std::int32_t world_z = chunk_z * 16 + z;
            
            for (std::int32_t y = 10; y < 118; y++) {
                if (!Chunk::is_valid_y(y)) continue;
                
                double cave_noise = cave_noise_->noise_3d(world_x * 0.03, y * 0.03, world_z * 0.03, 1.0, 2);
                if (cave_noise > 0.4) {
                    BlockState current = chunk.get_block(x, y, z);
                    if (current.get_block_type() == BlockType::NETHERRACK) {
                        BlockState air_state(BlockType::AIR);
                        chunk.set_block(x, y, z, air_state);
                    }
                }
            }
        }
    }
}

void NetherGenerator::generate_lava_lakes(Chunk& chunk, std::int32_t chunk_x, std::int32_t chunk_z) {
    // Fill low areas with lava
    for (std::uint8_t x = 0; x < 16; x++) {
        for (std::uint8_t z = 0; z < 16; z++) {
            for (std::int32_t y = 5; y < 32; y++) {
                if (!Chunk::is_valid_y(y)) continue;
                
                BlockState current = chunk.get_block(x, y, z);
                if (current.get_block_type() == BlockType::AIR) {
                    BlockState lava_state(BlockType::LAVA);
                    chunk.set_block(x, y, z, lava_state);
                }
            }
        }
    }
}

// ==================== END GENERATOR ====================

EndGenerator::EndGenerator(std::uint64_t seed) : WorldGenerator(seed) {
    terrain_noise_ = std::make_unique<NoiseGenerator>(seed);
    island_noise_ = std::make_unique<NoiseGenerator>(seed + 1);
}

void EndGenerator::generate_chunk(Chunk& chunk, std::int32_t chunk_x, std::int32_t chunk_z) {
    generate_end_terrain(chunk, chunk_x, chunk_z);
}

BiomeType EndGenerator::get_biome(std::int32_t x, std::int32_t z) {
    double distance = std::sqrt(x * x + z * z);
    
    if (distance < 1000) {
        return BiomeType::THE_END;
    } else if (distance < 2000) {
        return BiomeType::END_MIDLANDS;
    } else {
        return BiomeType::END_HIGHLANDS;
    }
}

void EndGenerator::populate_chunk(Chunk& chunk, std::int32_t chunk_x, std::int32_t chunk_z) {
    // Add chorus plants and end cities
    // TODO: Implement end-specific decorations
}

std::tuple<std::int32_t, std::int32_t, std::int32_t> EndGenerator::get_spawn_point() {
    return std::make_tuple(0, 65, 0);
}

void EndGenerator::generate_end_terrain(Chunk& chunk, std::int32_t chunk_x, std::int32_t chunk_z) {
    for (std::uint8_t x = 0; x < 16; x++) {
        for (std::uint8_t z = 0; z < 16; z++) {
            std::int32_t world_x = chunk_x * 16 + x;
            std::int32_t world_z = chunk_z * 16 + z;
            
            // Distance from origin affects island generation
            double distance = std::sqrt(world_x * world_x + world_z * world_z);
            double island_noise = island_noise_->noise_2d(world_x * 0.01, world_z * 0.01, 1.0, 3);
            
            // Main island around origin
            if (distance < 1000) {
                for (std::int32_t y = 0; y < 80; y++) {
                    if (!Chunk::is_valid_y(y)) continue;
                    
                    double height_noise = terrain_noise_->noise_2d(world_x * 0.02, world_z * 0.02, 1.0, 2);
                    std::int32_t surface_height = static_cast<std::int32_t>(60 + height_noise * 15);
                    
                    if (y < surface_height) {
                        BlockState block_state(BlockType::END_STONE);
                        chunk.set_block(x, y, z, block_state);
                    }
                }
            }
            // Outer islands
            else if (distance > 1000 && island_noise > 0.5) {
                for (std::int32_t y = 40; y < 100; y++) {
                    if (!Chunk::is_valid_y(y)) continue;
                    
                    double height_noise = terrain_noise_->noise_3d(world_x * 0.02, y * 0.02, world_z * 0.02, 1.0, 2);
                    if (height_noise > 0.3) {
                        BlockState block_state(BlockType::END_STONE);
                        chunk.set_block(x, y, z, block_state);
                    }
                }
            }
        }
    }
}

void EndGenerator::generate_end_cities(Chunk& chunk, std::int32_t chunk_x, std::int32_t chunk_z) {
    // TODO: Implement end city generation
}

} // namespace world
} // namespace parallelstone