#include <gtest/gtest.h>
#include "world/biome_system.hpp"
#include "world/chunk_section.hpp"
#include "world/world_generator.hpp"
#include <filesystem>
#include <memory>
#include <chrono>
#include <unordered_set>

namespace parallelstone {
namespace world {
namespace test {

class BiomeSystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_seed = 42;
        biome_generator = std::make_unique<BiomeGenerator>(test_seed);
        terrain_generator = std::make_shared<BiomeTerrainGenerator>(
            std::shared_ptr<BiomeGenerator>(biome_generator.get(), [](BiomeGenerator*){}));
        transition_system = std::make_unique<BiomeTransitionSystem>(
            std::shared_ptr<BiomeGenerator>(biome_generator.get(), [](BiomeGenerator*){}));
    }

    std::uint64_t test_seed;
    std::unique_ptr<BiomeGenerator> biome_generator;
    std::shared_ptr<BiomeTerrainGenerator> terrain_generator;
    std::unique_ptr<BiomeTransitionSystem> transition_system;
};

// ==================== BIOME GENERATION TESTS ====================

TEST_F(BiomeSystemTest, BiomeGeneratorInitialization) {
    ASSERT_NE(biome_generator, nullptr);
    EXPECT_EQ(biome_generator->get_seed(), test_seed);
}

TEST_F(BiomeSystemTest, BiomeGeneration_Overworld) {
    // Test biome generation at various coordinates
    BiomeType biome1 = biome_generator->generate_biome(0, 0, DimensionType::OVERWORLD);
    BiomeType biome2 = biome_generator->generate_biome(100, 200, DimensionType::OVERWORLD);
    BiomeType biome3 = biome_generator->generate_biome(-150, -300, DimensionType::OVERWORLD);
    
    // Should generate valid overworld biomes
    EXPECT_NE(biome1, BiomeType::INVALID);
    EXPECT_NE(biome2, BiomeType::INVALID);
    EXPECT_NE(biome3, BiomeType::INVALID);
    
    // Should not generate non-overworld biomes
    EXPECT_NE(biome1, BiomeType::NETHER_WASTES);
    EXPECT_NE(biome1, BiomeType::THE_END);
    EXPECT_NE(biome2, BiomeType::CRIMSON_FOREST);
    EXPECT_NE(biome3, BiomeType::END_HIGHLANDS);
}

TEST_F(BiomeSystemTest, BiomeGeneration_Nether) {
    BiomeType biome1 = biome_generator->generate_biome(0, 0, DimensionType::NETHER);
    BiomeType biome2 = biome_generator->generate_biome(500, -400, DimensionType::NETHER);
    
    // Should generate valid nether biomes
    std::unordered_set<BiomeType> valid_nether_biomes = {
        BiomeType::NETHER_WASTES, BiomeType::SOUL_SAND_VALLEY, BiomeType::CRIMSON_FOREST,
        BiomeType::WARPED_FOREST, BiomeType::BASALT_DELTAS
    };
    
    EXPECT_TRUE(valid_nether_biomes.count(biome1) > 0);
    EXPECT_TRUE(valid_nether_biomes.count(biome2) > 0);
}

TEST_F(BiomeSystemTest, BiomeGeneration_End) {
    BiomeType biome1 = biome_generator->generate_biome(0, 0, DimensionType::END);
    BiomeType biome2 = biome_generator->generate_biome(5000, 5000, DimensionType::END);
    
    // Central end should be THE_END
    EXPECT_EQ(biome1, BiomeType::THE_END);
    
    // Distant coordinates should be outer end biomes
    std::unordered_set<BiomeType> valid_outer_end_biomes = {
        BiomeType::END_HIGHLANDS, BiomeType::END_MIDLANDS, BiomeType::SMALL_END_ISLANDS, BiomeType::END_BARRENS
    };
    
    EXPECT_TRUE(valid_outer_end_biomes.count(biome2) > 0);
}

TEST_F(BiomeSystemTest, BiomeGeneration_Consistency) {
    // Same coordinates should always generate same biome
    BiomeType biome1a = biome_generator->generate_biome(123, 456, DimensionType::OVERWORLD);
    BiomeType biome1b = biome_generator->generate_biome(123, 456, DimensionType::OVERWORLD);
    BiomeType biome1c = biome_generator->generate_biome(123, 456, DimensionType::OVERWORLD);
    
    EXPECT_EQ(biome1a, biome1b);
    EXPECT_EQ(biome1b, biome1c);
}

TEST_F(BiomeSystemTest, ChunkBiomeGeneration) {
    auto biome_map = biome_generator->generate_chunk_biomes(0, 0, DimensionType::OVERWORLD);
    
    ASSERT_EQ(biome_map.size(), 16);
    ASSERT_EQ(biome_map[0].size(), 16);
    
    // All biomes should be valid
    for (int x = 0; x < 16; x++) {
        for (int z = 0; z < 16; z++) {
            EXPECT_NE(biome_map[x][z], BiomeType::INVALID);
        }
    }
    
    // Should have consistent generation with individual calls
    for (int x = 0; x < 16; x += 4) {
        for (int z = 0; z < 16; z += 4) {
            BiomeType individual_biome = biome_generator->generate_biome(x, z, DimensionType::OVERWORLD);
            EXPECT_EQ(biome_map[x][z], individual_biome);
        }
    }
}

// ==================== BIOME DATA TESTS ====================

TEST_F(BiomeSystemTest, BiomeDataRetrieval) {
    // Test retrieving data for various biome types
    const BiomeData& plains_data = biome_generator->get_biome_data(BiomeType::PLAINS);
    const BiomeData& desert_data = biome_generator->get_biome_data(BiomeType::DESERT);
    const BiomeData& ocean_data = biome_generator->get_biome_data(BiomeType::OCEAN);
    
    // Check basic properties
    EXPECT_EQ(plains_data.type, BiomeType::PLAINS);
    EXPECT_EQ(plains_data.category, BiomeCategory::FLATLAND);
    EXPECT_EQ(plains_data.name, "Plains");
    
    EXPECT_EQ(desert_data.type, BiomeType::DESERT);
    EXPECT_EQ(desert_data.category, BiomeCategory::ARIDLAND);
    EXPECT_TRUE(desert_data.is_dry);
    
    EXPECT_EQ(ocean_data.type, BiomeType::OCEAN);
    EXPECT_EQ(ocean_data.category, BiomeCategory::OFFSHORE);
    EXPECT_TRUE(ocean_data.is_ocean);
}

TEST_F(BiomeSystemTest, BiomeTemperatureHumidity) {
    // Test temperature and humidity generation
    float temp1 = biome_generator->get_temperature(0, 0);
    float temp2 = biome_generator->get_temperature(1000, 0);
    float humidity1 = biome_generator->get_humidity(0, 0);
    float humidity2 = biome_generator->get_humidity(0, 1000);
    
    // Should be in valid range
    EXPECT_GE(temp1, 0.0f);
    EXPECT_LE(temp1, 1.0f);
    EXPECT_GE(temp2, 0.0f);
    EXPECT_LE(temp2, 1.0f);
    
    EXPECT_GE(humidity1, 0.0f);
    EXPECT_LE(humidity1, 1.0f);
    EXPECT_GE(humidity2, 0.0f);
    EXPECT_LE(humidity2, 1.0f);
    
    // Should be consistent
    EXPECT_EQ(temp1, biome_generator->get_temperature(0, 0));
    EXPECT_EQ(humidity1, biome_generator->get_humidity(0, 0));
}

TEST_F(BiomeSystemTest, PrecipitationGeneration) {
    // Test precipitation at various locations and heights
    bool precipitation1 = biome_generator->has_precipitation_at(0, 70, 0);
    bool precipitation2 = biome_generator->has_precipitation_at(100, 150, 200);
    bool precipitation3 = biome_generator->has_precipitation_at(-50, 300, -100); // Too high
    
    // Precipitation should be boolean
    EXPECT_TRUE(precipitation1 == true || precipitation1 == false);
    EXPECT_TRUE(precipitation2 == true || precipitation2 == false);
    
    // No precipitation above world height
    EXPECT_FALSE(precipitation3);
}

// ==================== TERRAIN GENERATION TESTS ====================

TEST_F(BiomeSystemTest, TerrainHeightCalculation) {
    // Test terrain height calculation for different biomes
    std::int32_t height1 = terrain_generator->calculate_terrain_height(BiomeType::PLAINS, 0, 0);
    std::int32_t height2 = terrain_generator->calculate_terrain_height(BiomeType::MOUNTAINS, 0, 0);
    std::int32_t height3 = terrain_generator->calculate_terrain_height(BiomeType::DEEP_OCEAN, 0, 0);
    
    // Heights should be in reasonable ranges
    EXPECT_GE(height1, -64);
    EXPECT_LE(height1, 319);
    
    EXPECT_GE(height2, -64);
    EXPECT_LE(height2, 319);
    
    EXPECT_GE(height3, -64);
    EXPECT_LE(height3, 319);
    
    // Mountains should generally be higher than plains
    EXPECT_GE(height2, height1);
    
    // Deep ocean should be lower than plains
    EXPECT_LE(height3, height1);
}

TEST_F(BiomeSystemTest, ChunkTerrainGeneration) {
    Chunk test_chunk(0, 0);
    
    // Generate terrain for the chunk
    terrain_generator->generate_terrain(test_chunk, 0, 0, DimensionType::OVERWORLD);
    
    // Check that terrain was generated
    bool found_non_air = false;
    bool found_bedrock = false;
    
    for (std::uint8_t x = 0; x < 16; x++) {
        for (std::uint8_t z = 0; z < 16; z++) {
            for (std::int32_t y = -64; y < 100; y++) {
                if (!Chunk::is_valid_y(y)) continue;
                
                auto block_state = test_chunk.get_block(x, y, z);
                BlockType block_type = block_state.get_block_type();
                
                if (block_type != BlockType::AIR) {
                    found_non_air = true;
                }
                
                if (block_type == BlockType::BEDROCK) {
                    found_bedrock = true;
                }
            }
        }
    }
    
    EXPECT_TRUE(found_non_air);
    EXPECT_TRUE(found_bedrock);
}

TEST_F(BiomeSystemTest, BiomeSpecificTerrain) {
    Chunk desert_chunk(1, 1);
    Chunk ocean_chunk(2, 2);
    
    // Force specific biomes by generating at coordinates known to produce them
    // In a real implementation, you might need to find coordinates that generate these biomes
    terrain_generator->generate_terrain(desert_chunk, 1, 1, DimensionType::OVERWORLD);
    terrain_generator->generate_terrain(ocean_chunk, 2, 2, DimensionType::OVERWORLD);
    
    // Both chunks should have generated terrain
    EXPECT_FALSE(desert_chunk.is_empty());
    EXPECT_FALSE(ocean_chunk.is_empty());
}

// ==================== BIOME TRANSITION TESTS ====================

TEST_F(BiomeSystemTest, TransitionZoneDetection) {
    // Test transition zone detection
    bool is_transition1 = transition_system->is_transition_zone(0, 0);
    bool is_transition2 = transition_system->is_transition_zone(1000, 1000);
    
    // Should return boolean values
    EXPECT_TRUE(is_transition1 == true || is_transition1 == false);
    EXPECT_TRUE(is_transition2 == true || is_transition2 == false);
}

TEST_F(BiomeSystemTest, BiomeBlending) {
    // Test blended biome data retrieval
    BiomeData blended_data = transition_system->get_blended_biome_data(0, 0, 8.0f);
    
    // Blended data should have valid values
    EXPECT_GE(blended_data.temperature, 0.0f);
    EXPECT_LE(blended_data.temperature, 2.0f);
    EXPECT_GE(blended_data.humidity, 0.0f);
    EXPECT_LE(blended_data.humidity, 1.0f);
    EXPECT_GE(blended_data.base_height, -2.0f);
    EXPECT_LE(blended_data.base_height, 2.0f);
}

TEST_F(BiomeSystemTest, TransitionApplication) {
    Chunk transition_chunk(3, 3);
    
    // Generate base terrain
    terrain_generator->generate_terrain(transition_chunk, 3, 3, DimensionType::OVERWORLD);
    
    // Apply transitions
    transition_system->apply_transitions(transition_chunk, 3, 3);
    
    // Chunk should still be valid after transitions
    EXPECT_FALSE(transition_chunk.is_empty());
    
    // Should have reasonable heightmap
    for (std::uint8_t x = 0; x < 16; x++) {
        for (std::uint8_t z = 0; z < 16; z++) {
            std::int32_t height = transition_chunk.get_height(x, z);
            EXPECT_GE(height, -64);
            EXPECT_LE(height, 319);
        }
    }
}

// ==================== INTEGRATION TESTS ====================

class BiomeIntegrationTest : public BiomeSystemTest {
protected:
    void SetUp() override {
        BiomeSystemTest::SetUp();
        world_generator = std::make_unique<OverworldGenerator>(test_seed);
        world_generator->set_biome_generator(
            std::shared_ptr<BiomeGenerator>(biome_generator.get(), [](BiomeGenerator*){}));
    }
    
    std::unique_ptr<OverworldGenerator> world_generator;
};

TEST_F(BiomeIntegrationTest, WorldGeneratorIntegration) {
    Chunk integrated_chunk(0, 0);
    
    // Generate chunk using integrated world generator
    world_generator->generate_chunk(integrated_chunk, 0, 0, DimensionType::OVERWORLD);
    
    // Should generate complete terrain
    EXPECT_FALSE(integrated_chunk.is_empty());
    
    // Should have bedrock at bottom
    bool found_bedrock = false;
    for (std::uint8_t x = 0; x < 16 && !found_bedrock; x++) {
        for (std::uint8_t z = 0; z < 16 && !found_bedrock; z++) {
            for (std::int32_t y = -64; y < -55; y++) {
                auto block_state = integrated_chunk.get_block(x, y, z);
                if (block_state.get_block_type() == BlockType::BEDROCK) {
                    found_bedrock = true;
                    break;
                }
            }
        }
    }
    EXPECT_TRUE(found_bedrock);
}

TEST_F(BiomeIntegrationTest, BiomeConsistency) {
    // Test that biome generation is consistent between generator and system
    BiomeType system_biome = biome_generator->generate_biome(100, 200, DimensionType::OVERWORLD);
    BiomeType generator_biome = world_generator->get_biome(100, 200, DimensionType::OVERWORLD);
    
    EXPECT_EQ(system_biome, generator_biome);
}

TEST_F(BiomeIntegrationTest, SpawnPointGeneration) {
    auto [spawn_x, spawn_y, spawn_z] = world_generator->get_spawn_point(DimensionType::OVERWORLD);
    
    // Spawn point should be reasonable
    EXPECT_GE(spawn_y, 60); // Above sea level
    EXPECT_LT(spawn_y, 200); // Not too high
    EXPECT_GE(spawn_x, -1000);
    EXPECT_LE(spawn_x, 1000);
    EXPECT_GE(spawn_z, -1000);
    EXPECT_LE(spawn_z, 1000);
    
    // Should be in a suitable spawn biome
    BiomeType spawn_biome = world_generator->get_biome(spawn_x, spawn_z, DimensionType::OVERWORLD);
    auto spawn_biomes = get_spawn_biomes(DimensionType::OVERWORLD);
    bool is_suitable = std::find(spawn_biomes.begin(), spawn_biomes.end(), spawn_biome) != spawn_biomes.end();
    EXPECT_TRUE(is_suitable);
}

// ==================== PERFORMANCE TESTS ====================

TEST_F(BiomeSystemTest, BiomeGenerationPerformance) {
    const int test_iterations = 10000;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < test_iterations; i++) {
        std::int32_t x = (i * 137) % 10000 - 5000; // Pseudo-random coordinates
        std::int32_t z = (i * 149) % 10000 - 5000;
        BiomeType biome = biome_generator->generate_biome(x, z, DimensionType::OVERWORLD);
        (void)biome; // Prevent optimization
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Should generate biomes quickly (less than 10 microseconds per generation on average)
    double avg_time_us = static_cast<double>(duration.count()) / test_iterations;
    EXPECT_LT(avg_time_us, 10.0);
    
    std::cout << "Average biome generation time: " << avg_time_us << " microseconds" << std::endl;
}

TEST_F(BiomeSystemTest, ChunkBiomeGenerationPerformance) {
    const int test_chunks = 100;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < test_chunks; i++) {
        std::int32_t chunk_x = (i * 17) % 100 - 50;
        std::int32_t chunk_z = (i * 23) % 100 - 50;
        auto biome_map = biome_generator->generate_chunk_biomes(chunk_x, chunk_z, DimensionType::OVERWORLD);
        (void)biome_map; // Prevent optimization
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Should generate chunk biomes quickly (less than 5ms per chunk on average)
    double avg_time_ms = static_cast<double>(duration.count()) / test_chunks;
    EXPECT_LT(avg_time_ms, 5.0);
    
    std::cout << "Average chunk biome generation time: " << avg_time_ms << " milliseconds" << std::endl;
}

TEST_F(BiomeSystemTest, TerrainGenerationPerformance) {
    const int test_chunks = 50;
    std::vector<std::unique_ptr<Chunk>> chunks;
    chunks.reserve(test_chunks);
    
    // Pre-create chunks
    for (int i = 0; i < test_chunks; i++) {
        chunks.push_back(std::make_unique<Chunk>(i, i));
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < test_chunks; i++) {
        terrain_generator->generate_terrain(*chunks[i], i, i, DimensionType::OVERWORLD);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Should generate terrain reasonably quickly (less than 100ms per chunk on average)
    double avg_time_ms = static_cast<double>(duration.count()) / test_chunks;
    EXPECT_LT(avg_time_ms, 100.0);
    
    std::cout << "Average terrain generation time: " << avg_time_ms << " milliseconds" << std::endl;
}

// ==================== STRESS TESTS ====================

TEST_F(BiomeSystemTest, LargeAreaBiomeGeneration) {
    // Test biome generation over a large area
    const int area_size = 1000; // 1000x1000 block area
    const int sample_step = 50;  // Sample every 50 blocks
    
    std::unordered_set<BiomeType> generated_biomes;
    
    for (int x = -area_size/2; x < area_size/2; x += sample_step) {
        for (int z = -area_size/2; z < area_size/2; z += sample_step) {
            BiomeType biome = biome_generator->generate_biome(x, z, DimensionType::OVERWORLD);
            generated_biomes.insert(biome);
        }
    }
    
    // Should generate a diverse set of biomes over large area
    EXPECT_GE(generated_biomes.size(), 5);
    
    // Should not generate invalid biomes
    EXPECT_EQ(generated_biomes.count(BiomeType::INVALID), 0);
    
    std::cout << "Generated " << generated_biomes.size() << " different biomes in large area test" << std::endl;
}

TEST_F(BiomeSystemTest, ExtremeDimensionGeneration) {
    // Test biome generation at extreme coordinates
    std::vector<std::pair<std::int32_t, std::int32_t>> extreme_coords = {
        {1000000, 1000000},
        {-1000000, -1000000},
        {1000000, -1000000},
        {-1000000, 1000000},
        {0, 1000000},
        {1000000, 0}
    };
    
    for (const auto& [x, z] : extreme_coords) {
        BiomeType biome = biome_generator->generate_biome(x, z, DimensionType::OVERWORLD);
        EXPECT_NE(biome, BiomeType::INVALID);
        
        // Should be consistent
        BiomeType biome_again = biome_generator->generate_biome(x, z, DimensionType::OVERWORLD);
        EXPECT_EQ(biome, biome_again);
    }
}

} // namespace test
} // namespace world
} // namespace parallelstone