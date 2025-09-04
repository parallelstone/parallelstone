#include "world/biome_system.hpp"
#include "world/chunk_section.hpp" 
#include "world/block_state.hpp"
#include "utils/noise.hpp"
#include <algorithm>
#include <random>
#include <spdlog/spdlog.h>

namespace parallelstone {
namespace world {

// ==================== BIOME TERRAIN GENERATOR IMPLEMENTATION ====================

BiomeTerrainGenerator::BiomeTerrainGenerator(std::shared_ptr<BiomeGenerator> biome_gen) 
    : biome_generator_(biome_gen) {
    
    // Initialize terrain-specific noise generators
    std::uint64_t base_seed = 12345; // Should come from world seed
    height_noise_ = std::make_unique<utils::PerlinNoise>(base_seed);
    surface_noise_ = std::make_unique<utils::PerlinNoise>(base_seed + 1000);
    cave_noise_ = std::make_unique<utils::PerlinNoise>(base_seed + 2000);
    ore_noise_ = std::make_unique<utils::PerlinNoise>(base_seed + 3000);
    
    spdlog::info("Initialized biome-aware terrain generator");
}

void BiomeTerrainGenerator::generate_terrain(Chunk& chunk, std::int32_t chunk_x, std::int32_t chunk_z, DimensionType dimension) {
    // Generate biome map for this chunk
    auto biome_map = biome_generator_->generate_chunk_biomes(chunk_x, chunk_z, dimension);
    
    switch (dimension) {
        case DimensionType::OVERWORLD:
            generate_overworld_terrain(chunk, biome_map, chunk_x, chunk_z);
            break;
        case DimensionType::NETHER:
            generate_nether_terrain(chunk, chunk_x, chunk_z);
            break;
        case DimensionType::END:
            generate_end_terrain(chunk, chunk_x, chunk_z);
            break;
    }
    
    // Generate biome-specific features
    generate_biome_features(chunk, biome_map, chunk_x, chunk_z);
}

void BiomeTerrainGenerator::generate_overworld_terrain(Chunk& chunk, const std::vector<std::vector<BiomeType>>& biome_map,
                                                     std::int32_t chunk_x, std::int32_t chunk_z) {
    
    for (std::uint8_t x = 0; x < 16; x++) {
        for (std::uint8_t z = 0; z < 16; z++) {
            std::int32_t world_x = chunk_x * 16 + x;
            std::int32_t world_z = chunk_z * 16 + z;
            BiomeType biome = biome_map[x][z];
            
            // Calculate terrain height based on biome characteristics
            std::int32_t surface_height = calculate_terrain_height(biome, world_x, world_z);
            surface_height = std::clamp(surface_height, -64, 319); // World height limits
            
            // Generate basic terrain layers
            generate_basic_terrain_column(chunk, x, z, surface_height, biome);
            
            // Generate surface layer specific to biome
            generate_surface_layer(chunk, biome, x, z, surface_height);
            
            // Generate vegetation
            generate_vegetation(chunk, biome, x, surface_height + 1, z);
            
            // Update chunk heightmap
            chunk.set_height(x, z, surface_height);
        }
    }
    
    // Generate caves and underground features
    generate_caves(chunk, biome_map, chunk_x, chunk_z);
    
    // Generate ore distributions based on biomes
    for (std::uint8_t x = 0; x < 16; x++) {
        for (std::uint8_t z = 0; z < 16; z++) {
            generate_ore_veins(chunk, biome_map[x][z]);
        }
    }
}

std::int32_t BiomeTerrainGenerator::calculate_terrain_height(BiomeType biome, std::int32_t x, std::int32_t z) const {
    const BiomeData& biome_data = biome_generator_->get_biome_data(biome);
    
    // Multi-octave height generation
    float height = 0.0f;
    
    // Large-scale continental features
    height += height_noise_->sample(x * 0.0001f, z * 0.0001f) * 50.0f;
    
    // Medium-scale terrain features (hills, valleys)  
    height += height_noise_->sample(x * 0.0008f, z * 0.0008f) * 25.0f;
    
    // Small-scale terrain variation
    height += height_noise_->sample(x * 0.003f, z * 0.003f) * 10.0f;
    
    // Apply biome-specific height modifications
    float biome_base = biome_data.base_height * 40.0f; // Scale to reasonable heights
    float biome_variation = biome_data.height_variation * height * biome_data.terrain_scale;
    
    height = biome_base + biome_variation;
    
    // Sea level adjustment (y=63)
    height += 63.0f;
    
    // Special biome height adjustments
    switch (biome) {
        case BiomeType::MOUNTAINS:
        case BiomeType::JAGGED_PEAKS:
        case BiomeType::FROZEN_PEAKS:
        case BiomeType::STONY_PEAKS:
            // Add extra height for mountain peaks
            height += height_noise_->sample(x * 0.002f, z * 0.002f) * 100.0f;
            height = std::max(height, 120.0f); // Minimum mountain height
            break;
            
        case BiomeType::DEEP_OCEAN:
        case BiomeType::DEEP_COLD_OCEAN:
        case BiomeType::DEEP_FROZEN_OCEAN:
        case BiomeType::DEEP_LUKEWARM_OCEAN:
            // Deep ocean floors
            height = std::min(height, 30.0f);
            break;
            
        case BiomeType::OCEAN:
        case BiomeType::WARM_OCEAN:
        case BiomeType::LUKEWARM_OCEAN:
        case BiomeType::COLD_OCEAN:
        case BiomeType::FROZEN_OCEAN:
            // Regular ocean depth
            height = std::min(height, 45.0f);
            break;
            
        case BiomeType::SWAMP:
        case BiomeType::MANGROVE_SWAMP:
            // Swamps are typically at or below sea level
            height = std::min(height, 62.0f);
            break;
    }
    
    return static_cast<std::int32_t>(height);
}

void BiomeTerrainGenerator::generate_basic_terrain_column(Chunk& chunk, std::uint8_t x, std::uint8_t z, 
                                                        std::int32_t surface_height, BiomeType biome) {
    const BiomeData& biome_data = biome_generator_->get_biome_data(biome);
    
    // Fill from bedrock to surface
    for (std::int32_t y = -64; y <= surface_height && y < 320; y++) {
        BlockType block_type = BlockType::AIR;
        
        if (y <= -60) {
            // Bedrock layer
            block_type = BlockType::BEDROCK;
        } else if (y <= surface_height - 4) {
            // Deep stone layer
            block_type = biome_data.stone_block;
        } else if (y < surface_height) {
            // Subsurface layer
            block_type = biome_data.subsurface_block;
        } else if (y == surface_height) {
            // Surface block
            block_type = biome_data.surface_block;
        }
        
        // Handle water/lava filling for ocean and below-sea-level areas
        if (block_type == BlockType::AIR && y <= 62) {
            block_type = biome_data.fluid_block;
        }
        
        if (block_type != BlockType::AIR) {
            chunk.set_block(x, y, z, BlockState(block_type));
        }
    }
}

void BiomeTerrainGenerator::generate_surface_layer(Chunk& chunk, BiomeType biome, 
                                                  std::uint8_t x, std::uint8_t z, std::int32_t surface_y) {
    const BiomeData& biome_data = biome_generator_->get_biome_data(biome);
    
    // Add surface variations based on biome
    switch (biome) {
        case BiomeType::DESERT: {
            // Sand dunes and varied sand depth
            float sand_depth_noise = surface_noise_->sample((chunk_x * 16 + x) * 0.1f, (chunk_z * 16 + z) * 0.1f);
            int sand_depth = 2 + static_cast<int>(sand_depth_noise * 4);
            
            for (int i = 0; i < sand_depth && (surface_y - i) >= -64; i++) {
                chunk.set_block(x, surface_y - i, z, BlockState(BlockType::SAND));
            }
            break;
        }
        
        case BiomeType::SNOWY_PLAINS:
        case BiomeType::SNOWY_TAIGA:
        case BiomeType::ICE_SPIKES: {
            // Snow layer on top
            if (surface_y + 1 < 320) {
                chunk.set_block(x, surface_y + 1, z, BlockState(BlockType::SNOW));
            }
            break;
        }
        
        case BiomeType::MUSHROOM_FIELDS: {
            // Mycelium surface with varied depth
            chunk.set_block(x, surface_y, z, BlockState(BlockType::MYCELIUM));
            if (surface_y - 1 >= -64) {
                chunk.set_block(x, surface_y - 1, z, BlockState(BlockType::DIRT));
            }
            break;
        }
        
        case BiomeType::BADLANDS:
        case BiomeType::WOODED_BADLANDS:
        case BiomeType::ERODED_BADLANDS: {
            // Layered terracotta
            std::vector<BlockType> terracotta_layers = {
                BlockType::RED_TERRACOTTA, BlockType::ORANGE_TERRACOTTA, 
                BlockType::YELLOW_TERRACOTTA, BlockType::WHITE_TERRACOTTA,
                BlockType::LIGHT_GRAY_TERRACOTTA, BlockType::BROWN_TERRACOTTA
            };
            
            for (int i = 0; i < 8 && (surface_y - i) >= -64; i++) {
                BlockType layer_block = terracotta_layers[i % terracotta_layers.size()];
                chunk.set_block(x, surface_y - i, z, BlockState(layer_block));
            }
            break;
        }
        
        case BiomeType::STONY_SHORE: {
            // Mix of stone and gravel
            float stone_noise = surface_noise_->sample((chunk_x * 16 + x) * 0.2f, (chunk_z * 16 + z) * 0.2f);
            BlockType surface_block = (stone_noise > 0.0f) ? BlockType::STONE : BlockType::GRAVEL;
            chunk.set_block(x, surface_y, z, BlockState(surface_block));
            break;
        }
        
        case BiomeType::SOUL_SAND_VALLEY: {
            // Soul sand with soul soil underneath
            chunk.set_block(x, surface_y, z, BlockState(BlockType::SOUL_SAND));
            if (surface_y - 1 >= -64) {
                chunk.set_block(x, surface_y - 1, z, BlockState(BlockType::SOUL_SOIL));
            }
            break;
        }
    }
}

void BiomeTerrainGenerator::generate_vegetation(Chunk& chunk, BiomeType biome, 
                                              std::uint8_t x, std::int32_t y, std::uint8_t z) {
    if (y >= 320 || y < -64) return;
    
    const BiomeData& biome_data = biome_generator_->get_biome_data(biome);
    
    // Check if this position should have vegetation
    std::int32_t world_x = chunk_x * 16 + x;
    std::int32_t world_z = chunk_z * 16 + z;
    float vegetation_noise = surface_noise_->sample(world_x * 0.1f, world_z * 0.1f);
    
    if (vegetation_noise < biome_data.vegetation_density) {
        // Place vegetation blocks
        if (!biome_data.vegetation_blocks.empty()) {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, biome_data.vegetation_blocks.size() - 1);
            
            BlockType vegetation = biome_data.vegetation_blocks[dis(gen)];
            chunk.set_block(x, y, z, BlockState(vegetation));
        }
    }
    
    // Tree generation
    float tree_noise = surface_noise_->sample(world_x * 0.05f, world_z * 0.05f);
    if (tree_noise < biome_data.tree_density) {
        place_trees(chunk, biome, x, y, z);
    }
    
    // Grass and flower generation  
    place_grass_and_flowers(chunk, biome, x, y, z);
}

void BiomeTerrainGenerator::place_trees(Chunk& chunk, BiomeType biome, std::uint8_t x, std::int32_t y, std::uint8_t z) {
    if (y + 10 >= 320) return; // Not enough space for tree
    
    // Simple tree placement - in a full implementation, this would be much more sophisticated
    switch (biome) {
        case BiomeType::FOREST:
        case BiomeType::PLAINS: {
            // Oak tree
            place_simple_tree(chunk, x, y, z, BlockType::OAK_LOG, BlockType::OAK_LEAVES, 4 + (rand() % 3));
            break;
        }
        
        case BiomeType::BIRCH_FOREST: {
            // Birch tree
            place_simple_tree(chunk, x, y, z, BlockType::BIRCH_LOG, BlockType::BIRCH_LEAVES, 5 + (rand() % 2));
            break;
        }
        
        case BiomeType::TAIGA:
        case BiomeType::SNOWY_TAIGA: {
            // Spruce tree
            place_simple_tree(chunk, x, y, z, BlockType::SPRUCE_LOG, BlockType::SPRUCE_LEAVES, 6 + (rand() % 4));
            break;
        }
        
        case BiomeType::JUNGLE:
        case BiomeType::BAMBOO_JUNGLE: {
            // Jungle tree
            place_simple_tree(chunk, x, y, z, BlockType::JUNGLE_LOG, BlockType::JUNGLE_LEAVES, 8 + (rand() % 6));
            // Add cocoa beans occasionally
            if (rand() % 4 == 0 && y + 2 < 320) {
                chunk.set_block(x, y + 2, z, BlockState(BlockType::COCOA));
            }
            break;
        }
        
        case BiomeType::DARK_FOREST: {
            // Dark oak tree
            place_simple_tree(chunk, x, y, z, BlockType::DARK_OAK_LOG, BlockType::DARK_OAK_LEAVES, 6 + (rand() % 3));
            break;
        }
        
        case BiomeType::CRIMSON_FOREST: {
            // Crimson fungus
            place_nether_fungus(chunk, x, y, z, BlockType::CRIMSON_STEM, BlockType::NETHER_WART_BLOCK, 5 + (rand() % 4));
            break;
        }
        
        case BiomeType::WARPED_FOREST: {
            // Warped fungus
            place_nether_fungus(chunk, x, y, z, BlockType::WARPED_STEM, BlockType::WARPED_WART_BLOCK, 5 + (rand() % 4));
            break;
        }
    }
}

void BiomeTerrainGenerator::place_simple_tree(Chunk& chunk, std::uint8_t x, std::int32_t y, std::uint8_t z,
                                            BlockType log_type, BlockType leaf_type, int height) {
    // Place trunk
    for (int i = 0; i < height; i++) {
        if (y + i < 320) {
            chunk.set_block(x, y + i, z, BlockState(log_type));
        }
    }
    
    // Place leaves (simple sphere shape)
    int leaf_y = y + height - 1;
    for (int dx = -2; dx <= 2; dx++) {
        for (int dz = -2; dz <= 2; dz++) {
            for (int dy = -1; dy <= 2; dy++) {
                if (x + dx >= 0 && x + dx < 16 && z + dz >= 0 && z + dz < 16) {
                    int leaf_pos_y = leaf_y + dy;
                    if (leaf_pos_y >= -64 && leaf_pos_y < 320) {
                        // Skip center column (trunk) and corners
                        if ((dx == 0 && dz == 0) || (abs(dx) == 2 && abs(dz) == 2)) {
                            continue;
                        }
                        
                        // Random leaf placement for natural look
                        if (rand() % 4 != 0) { // 75% chance
                            chunk.set_block(x + dx, leaf_pos_y, z + dz, BlockState(leaf_type));
                        }
                    }
                }
            }
        }
    }
}

void BiomeTerrainGenerator::place_nether_fungus(Chunk& chunk, std::uint8_t x, std::int32_t y, std::uint8_t z,
                                               BlockType stem_type, BlockType wart_type, int height) {
    // Place stem
    for (int i = 0; i < height; i++) {
        if (y + i < 320) {
            chunk.set_block(x, y + i, z, BlockState(stem_type));
        }
    }
    
    // Place wart blocks at top
    int top_y = y + height - 1;
    for (int dx = -1; dx <= 1; dx++) {
        for (int dz = -1; dz <= 1; dz++) {
            if (x + dx >= 0 && x + dx < 16 && z + dz >= 0 && z + dz < 16) {
                if (top_y >= -64 && top_y < 320) {
                    chunk.set_block(x + dx, top_y, z + dz, BlockState(wart_type));
                }
            }
        }
    }
}

void BiomeTerrainGenerator::place_grass_and_flowers(Chunk& chunk, BiomeType biome, std::uint8_t x, std::int32_t y, std::uint8_t z) {
    if (y >= 320 || y < -64) return;
    
    std::int32_t world_x = chunk_x * 16 + x;
    std::int32_t world_z = chunk_z * 16 + z;
    float grass_noise = surface_noise_->sample(world_x * 0.3f, world_z * 0.3f);
    
    if (grass_noise < 0.3f) { // 30% chance for grass/flowers
        switch (biome) {
            case BiomeType::PLAINS:
            case BiomeType::SUNFLOWER_PLAINS:
                if (rand() % 10 < 7) {
                    chunk.set_block(x, y, z, BlockState(BlockType::SHORT_GRASS));
                } else {
                    BlockType flower = (rand() % 2 == 0) ? BlockType::DANDELION : BlockType::POPPY;
                    chunk.set_block(x, y, z, BlockState(flower));
                }
                break;
                
            case BiomeType::FOREST:
            case BiomeType::BIRCH_FOREST:
            case BiomeType::DARK_FOREST:
                if (rand() % 3 == 0) {
                    chunk.set_block(x, y, z, BlockState(BlockType::FERN));
                } else {
                    chunk.set_block(x, y, z, BlockState(BlockType::SHORT_GRASS));
                }
                break;
                
            case BiomeType::FLOWER_FOREST:
                // Higher flower density
                if (rand() % 5 < 3) {
                    std::vector<BlockType> flowers = {
                        BlockType::DANDELION, BlockType::POPPY, BlockType::BLUE_ORCHID,
                        BlockType::ALLIUM, BlockType::AZURE_BLUET, BlockType::OXEYE_DAISY
                    };
                    BlockType flower = flowers[rand() % flowers.size()];
                    chunk.set_block(x, y, z, BlockState(flower));
                } else {
                    chunk.set_block(x, y, z, BlockState(BlockType::SHORT_GRASS));
                }
                break;
                
            case BiomeType::DESERT:
                if (rand() % 20 == 0) { // Rare in desert
                    chunk.set_block(x, y, z, BlockState(BlockType::DEAD_BUSH));
                }
                break;
                
            case BiomeType::SWAMP:
                if (rand() % 8 == 0) {
                    chunk.set_block(x, y, z, BlockState(BlockType::BLUE_ORCHID));
                } else if (rand() % 4 == 0) {
                    chunk.set_block(x, y, z, BlockState(BlockType::FERN));
                }
                break;
        }
    }
}

void BiomeTerrainGenerator::generate_caves(Chunk& chunk, const std::vector<std::vector<BiomeType>>& biome_map,
                                         std::int32_t chunk_x, std::int32_t chunk_z) {
    for (std::uint8_t x = 0; x < 16; x++) {
        for (std::uint8_t z = 0; z < 16; z++) {
            std::int32_t world_x = chunk_x * 16 + x;
            std::int32_t world_z = chunk_z * 16 + z;
            
            for (std::int32_t y = -50; y < 100; y++) { // Cave generation range
                // 3D cave noise
                float cave_noise = cave_noise_->sample(world_x * 0.02f, y * 0.02f, world_z * 0.02f);
                float cave_noise2 = cave_noise_->sample(world_x * 0.03f, y * 0.03f, world_z * 0.03f);
                
                // Combine noises for cave tunnels
                if (cave_noise > 0.6f && cave_noise2 > 0.5f) {
                    auto current_block = chunk.get_block(x, y, z);
                    if (current_block.get_block_type() == BlockType::STONE ||
                        current_block.get_block_type() == BlockType::DIRT ||
                        current_block.get_block_type() == BlockType::DEEPSLATE) {
                        
                        // Create cave air
                        chunk.set_block(x, y, z, BlockState(BlockType::AIR));
                        
                        // Add water in lower caves
                        if (y < 10 && rand() % 20 == 0) {
                            chunk.set_block(x, y, z, BlockState(BlockType::WATER));
                        }
                    }
                }
            }
        }
    }
}

void BiomeTerrainGenerator::generate_ore_veins(Chunk& chunk, BiomeType biome) {
    // Generate ore based on depth and biome
    for (std::uint8_t x = 0; x < 16; x++) {
        for (std::uint8_t z = 0; z < 16; z++) {
            for (std::int32_t y = -64; y < 100; y++) {
                auto current_block = chunk.get_block(x, y, z);
                if (current_block.get_block_type() == BlockType::STONE ||
                    current_block.get_block_type() == BlockType::DEEPSLATE) {
                    
                    std::int32_t world_x = chunk_x * 16 + x;
                    std::int32_t world_z = chunk_z * 16 + z;
                    float ore_noise = ore_noise_->sample(world_x * 0.1f, y * 0.1f, world_z * 0.1f);
                    
                    // Coal (common, all levels)
                    if (ore_noise > 0.7f && y > -30) {
                        chunk.set_block(x, y, z, BlockState(BlockType::COAL_ORE));
                    }
                    // Iron (moderate depth)
                    else if (ore_noise > 0.75f && y < 50 && y > -40) {
                        chunk.set_block(x, y, z, BlockState(BlockType::IRON_ORE));
                    }
                    // Gold (deeper)
                    else if (ore_noise > 0.8f && y < 20 && y > -50) {
                        chunk.set_block(x, y, z, BlockState(BlockType::GOLD_ORE));
                    }
                    // Diamond (very deep)
                    else if (ore_noise > 0.85f && y < -10 && y > -60) {
                        chunk.set_block(x, y, z, BlockState(BlockType::DIAMOND_ORE));
                    }
                    // Emerald (mountains only)
                    else if (ore_noise > 0.88f && y > 50 && 
                            (biome == BiomeType::MOUNTAINS || biome == BiomeType::WINDSWEPT_HILLS)) {
                        chunk.set_block(x, y, z, BlockState(BlockType::EMERALD_ORE));
                    }
                }
            }
        }
    }
}

void BiomeTerrainGenerator::generate_biome_features(Chunk& chunk, const std::vector<std::vector<BiomeType>>& biome_map,
                                                   std::int32_t chunk_x, std::int32_t chunk_z) {
    // Generate special biome features
    for (std::uint8_t x = 0; x < 16; x++) {
        for (std::uint8_t z = 0; z < 16; z++) {
            BiomeType biome = biome_map[x][z];
            std::int32_t surface_y = chunk.get_height(x, z);
            
            switch (biome) {
                case BiomeType::ICE_SPIKES: {
                    // Generate ice spikes
                    if (rand() % 100 == 0) { // 1% chance
                        generate_ice_spike(chunk, x, surface_y + 1, z, 5 + (rand() % 10));
                    }
                    break;
                }
                
                case BiomeType::DESERT: {
                    // Generate cacti
                    if (rand() % 50 == 0) { // 2% chance
                        generate_cactus(chunk, x, surface_y + 1, z, 2 + (rand() % 3));
                    }
                    break;
                }
                
                case BiomeType::MUSHROOM_FIELDS: {
                    // Generate giant mushrooms
                    if (rand() % 30 == 0) { // 3.3% chance
                        generate_giant_mushroom(chunk, x, surface_y + 1, z, 
                                              (rand() % 2 == 0) ? BlockType::RED_MUSHROOM_BLOCK : BlockType::BROWN_MUSHROOM_BLOCK);
                    }
                    break;
                }
                
                case BiomeType::BAMBOO_JUNGLE: {
                    // Generate bamboo groves
                    if (rand() % 20 == 0) { // 5% chance
                        generate_bamboo_grove(chunk, x, surface_y + 1, z);
                    }
                    break;
                }
            }
        }
    }
}

void BiomeTerrainGenerator::generate_ice_spike(Chunk& chunk, std::uint8_t x, std::int32_t y, std::uint8_t z, int height) {
    for (int i = 0; i < height && y + i < 320; i++) {
        // Taper the spike as it gets taller
        int spike_size = std::max(1, 3 - (i / 3));
        
        for (int dx = -spike_size + 1; dx < spike_size; dx++) {
            for (int dz = -spike_size + 1; dz < spike_size; dz++) {
                if (x + dx >= 0 && x + dx < 16 && z + dz >= 0 && z + dz < 16) {
                    chunk.set_block(x + dx, y + i, z + dz, BlockState(BlockType::PACKED_ICE));
                }
            }
        }
    }
}

void BiomeTerrainGenerator::generate_cactus(Chunk& chunk, std::uint8_t x, std::int32_t y, std::uint8_t z, int height) {
    for (int i = 0; i < height && y + i < 320; i++) {
        chunk.set_block(x, y + i, z, BlockState(BlockType::CACTUS));
    }
}

void BiomeTerrainGenerator::generate_giant_mushroom(Chunk& chunk, std::uint8_t x, std::int32_t y, std::uint8_t z, BlockType mushroom_type) {
    int height = 4 + (rand() % 3);
    
    // Stem
    for (int i = 0; i < height; i++) {
        if (y + i < 320) {
            chunk.set_block(x, y + i, z, BlockState(BlockType::MUSHROOM_STEM));
        }
    }
    
    // Cap
    int cap_y = y + height;
    for (int dx = -2; dx <= 2; dx++) {
        for (int dz = -2; dz <= 2; dz++) {
            if (x + dx >= 0 && x + dx < 16 && z + dz >= 0 && z + dz < 16 && cap_y < 320) {
                if (abs(dx) <= 1 || abs(dz) <= 1) { // Diamond shape
                    chunk.set_block(x + dx, cap_y, z + dz, BlockState(mushroom_type));
                }
            }
        }
    }
}

void BiomeTerrainGenerator::generate_bamboo_grove(Chunk& chunk, std::uint8_t x, std::int32_t y, std::uint8_t z) {
    // Generate cluster of bamboo
    for (int dx = -2; dx <= 2; dx++) {
        for (int dz = -2; dz <= 2; dz++) {
            if (x + dx >= 0 && x + dx < 16 && z + dz >= 0 && z + dz < 16) {
                if (rand() % 3 == 0) { // 33% chance for each position
                    int bamboo_height = 8 + (rand() % 12);
                    for (int i = 0; i < bamboo_height && y + i < 320; i++) {
                        chunk.set_block(x + dx, y + i, z + dz, BlockState(BlockType::BAMBOO));
                    }
                }
            }
        }
    }
}

void BiomeTerrainGenerator::generate_nether_terrain(Chunk& chunk, std::int32_t chunk_x, std::int32_t chunk_z) {
    // Nether terrain generation with ceiling and floor
    for (std::uint8_t x = 0; x < 16; x++) {
        for (std::uint8_t z = 0; z < 16; z++) {
            std::int32_t world_x = chunk_x * 16 + x;
            std::int32_t world_z = chunk_z * 16 + z;
            
            BiomeType biome = biome_generator_->generate_biome(world_x, world_z, DimensionType::NETHER);
            const BiomeData& biome_data = biome_generator_->get_biome_data(biome);
            
            for (std::int32_t y = 0; y < 128; y++) { // Nether height range
                // Generate ceiling and floor
                if (y <= 4 || y >= 123) {
                    chunk.set_block(x, y, z, BlockState(BlockType::BEDROCK));
                }
                // Generate biome-specific terrain
                else {
                    float terrain_noise = height_noise_->sample(world_x * 0.05f, y * 0.05f, world_z * 0.05f);
                    
                    if (terrain_noise > 0.3f) {
                        chunk.set_block(x, y, z, BlockState(biome_data.surface_block));
                    } else if (y < 32) {
                        // Lava seas in lower nether
                        chunk.set_block(x, y, z, BlockState(BlockType::LAVA));
                    }
                }
            }
        }
    }
}

void BiomeTerrainGenerator::generate_end_terrain(Chunk& chunk, std::int32_t chunk_x, std::int32_t chunk_z) {
    // End terrain generation - floating islands
    for (std::uint8_t x = 0; x < 16; x++) {
        for (std::uint8_t z = 0; z < 16; z++) {
            std::int32_t world_x = chunk_x * 16 + x;
            std::int32_t world_z = chunk_z * 16 + z;
            
            // Distance from origin determines island type
            float distance = std::sqrt(static_cast<float>(world_x * world_x + world_z * world_z));
            
            if (distance < 1000.0f) {
                // Main end island
                generate_main_end_island(chunk, x, z, world_x, world_z);
            } else {
                // Outer end islands
                generate_outer_end_islands(chunk, x, z, world_x, world_z);
            }
        }
    }
}

void BiomeTerrainGenerator::generate_main_end_island(Chunk& chunk, std::uint8_t x, std::uint8_t z, 
                                                   std::int32_t world_x, std::int32_t world_z) {
    float distance = std::sqrt(static_cast<float>(world_x * world_x + world_z * world_z));
    float height_factor = std::max(0.0f, 1.0f - (distance / 1000.0f));
    
    int base_height = 64;
    int island_height = static_cast<int>(base_height + height_factor * 20);
    
    for (std::int32_t y = 50; y <= island_height; y++) {
        chunk.set_block(x, y, z, BlockState(BlockType::END_STONE));
    }
    
    // Generate obsidian pillars occasionally
    if (distance > 200.0f && distance < 800.0f && rand() % 200 == 0) {
        int pillar_height = 30 + (rand() % 40);
        for (int i = 0; i < pillar_height; i++) {
            if (island_height + i < 320) {
                chunk.set_block(x, island_height + i, z, BlockState(BlockType::OBSIDIAN));
            }
        }
    }
}

void BiomeTerrainGenerator::generate_outer_end_islands(Chunk& chunk, std::uint8_t x, std::uint8_t z,
                                                     std::int32_t world_x, std::int32_t world_z) {
    float island_noise = height_noise_->sample(world_x * 0.001f, world_z * 0.001f);
    
    if (island_noise > 0.4f) { // 40% chance for island presence
        int island_height = 40 + static_cast<int>(island_noise * 60);
        int thickness = 5 + static_cast<int>(island_noise * 15);
        
        for (std::int32_t y = island_height - thickness; y <= island_height; y++) {
            chunk.set_block(x, y, z, BlockState(BlockType::END_STONE));
        }
        
        // Add chorus plants on outer islands
        if (rand() % 10 == 0 && island_height + 1 < 320) {
            int chorus_height = 3 + (rand() % 8);
            for (int i = 0; i < chorus_height; i++) {
                if (island_height + 1 + i < 320) {
                    chunk.set_block(x, island_height + 1 + i, z, BlockState(BlockType::CHORUS_PLANT));
                }
            }
        }
    }
}

} // namespace world
} // namespace parallelstone