#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "world/world.hpp"
#include "world/world_ecs_integration.hpp"
#include "world/world_performance.hpp"
#include <filesystem>
#include <thread>
#include <chrono>

namespace parallelstone {
namespace world {
namespace test {

class WorldTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test world directory
        test_world_dir = std::filesystem::temp_directory_path() / "parallelstone_test_worlds";
        std::filesystem::create_directories(test_world_dir);
        
        // Configure test world
        config.world_name = "test_world";
        config.world_directory = test_world_dir;
        config.dimension = DimensionType::OVERWORLD;
        config.seed = 12345;
        config.max_loaded_chunks = 100;
        config.chunk_view_distance = 8;
        config.auto_save_enabled = false; // Disable for tests
        
        world = std::make_shared<World>(config);
    }
    
    void TearDown() override {
        world.reset();
        // Clean up test directories
        if (std::filesystem::exists(test_world_dir)) {
            std::filesystem::remove_all(test_world_dir);
        }
    }
    
    std::filesystem::path test_world_dir;
    WorldConfig config;
    std::shared_ptr<World> world;
};

// ==================== BASIC WORLD TESTS ====================

TEST_F(WorldTest, WorldInitialization) {
    ASSERT_NE(world, nullptr);
    EXPECT_EQ(world->config().world_name, "test_world");
    EXPECT_EQ(world->config().dimension, DimensionType::OVERWORLD);
    EXPECT_EQ(world->config().seed, 12345);
}

TEST_F(WorldTest, ChunkGeneration) {
    // Generate a chunk
    auto chunk = world->get_chunk(0, 0, true);
    
    ASSERT_NE(chunk, nullptr);
    EXPECT_EQ(chunk->chunk_x(), 0);
    EXPECT_EQ(chunk->chunk_z(), 0);
    EXPECT_FALSE(chunk->is_empty());
    
    // Verify chunk is loaded
    EXPECT_TRUE(world->is_chunk_loaded(0, 0));
}

TEST_F(WorldTest, BlockOperations) {
    // Generate chunk first
    world->get_chunk(0, 0, true);
    
    // Test block setting and getting
    world->set_block(5, 70, 5, BlockType::STONE);
    BlockType retrieved_block = world->get_block(5, 70, 5);
    
    EXPECT_EQ(retrieved_block, BlockType::STONE);
}

TEST_F(WorldTest, ChunkLoading) {
    // Load chunks in a 3x3 area
    world->load_chunks_around(0, 0, 1);
    
    // Verify all chunks are loaded
    for (int dx = -1; dx <= 1; dx++) {
        for (int dz = -1; dz <= 1; dz++) {
            EXPECT_TRUE(world->is_chunk_loaded(dx, dz));
        }
    }
    
    auto loaded_chunks = world->get_loaded_chunks();
    EXPECT_EQ(loaded_chunks.size(), 9);
}

TEST_F(WorldTest, ChunkUnloading) {
    // Load chunks
    world->load_chunks_around(0, 0, 2);
    
    // Unload distant chunks
    world->unload_chunks_outside(0, 0, 1);
    
    // Verify close chunks are still loaded
    EXPECT_TRUE(world->is_chunk_loaded(0, 0));
    EXPECT_TRUE(world->is_chunk_loaded(1, 0));
    
    // Verify distant chunks are unloaded
    EXPECT_FALSE(world->is_chunk_loaded(2, 2));
}

TEST_F(WorldTest, HeightCalculation) {
    // Generate chunk
    world->get_chunk(0, 0, true);
    
    // Test height calculation
    std::int32_t height = world->get_height(8, 8);
    
    // Height should be reasonable (between bedrock and sky)
    EXPECT_GT(height, 5);
    EXPECT_LT(height, 200);
}

// ==================== COORDINATE CONVERSION TESTS ====================

TEST_F(WorldTest, CoordinateConversion) {
    // Test world to chunk coordinate conversion
    auto chunk_coord = World::world_to_chunk(17, 33);
    EXPECT_EQ(chunk_coord.x, 1);
    EXPECT_EQ(chunk_coord.z, 2);
    
    // Test chunk to world coordinate conversion
    auto world_coord = World::chunk_to_world(1);
    EXPECT_EQ(world_coord, 16);
    
    // Test chunk-relative coordinates
    auto rel_coord = World::world_to_chunk_relative(17);
    EXPECT_EQ(rel_coord, 1);
}

// ==================== PERFORMANCE TESTS ====================

TEST_F(WorldTest, ChunkGenerationPerformance) {
    const int num_chunks = 25; // 5x5 area
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Generate chunks in parallel
    world->load_chunks_around(0, 0, 2);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Should generate 25 chunks in reasonable time (less than 5 seconds)
    EXPECT_LT(duration.count(), 5000);
    
    // Verify all chunks were generated
    auto loaded_chunks = world->get_loaded_chunks();
    EXPECT_EQ(loaded_chunks.size(), num_chunks);
}

TEST_F(WorldTest, BlockUpdatePerformance) {
    // Generate chunk
    world->get_chunk(0, 0, true);
    
    const int num_updates = 1000;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Perform many block updates
    for (int i = 0; i < num_updates; i++) {
        int x = i % 16;
        int z = (i / 16) % 16;
        world->set_block(x, 70 + (i % 10), z, BlockType::STONE);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Should handle 1000 block updates quickly (less than 100ms)
    EXPECT_LT(duration.count(), 100000);
}

// ==================== ECS INTEGRATION TESTS ====================

class WorldECSTest : public WorldTest {
protected:
    void SetUp() override {
        WorldTest::SetUp();
        world_integration = std::make_shared<WorldECSIntegration>(world);
    }
    
    std::shared_ptr<WorldECSIntegration> world_integration;
};

TEST_F(WorldECSTest, ECSInitialization) {
    ASSERT_NE(world_integration, nullptr);
    EXPECT_NE(&world_integration->get_registry(), nullptr);
    EXPECT_EQ(world_integration->get_world(), world);
}

TEST_F(WorldECSTest, BlockEntityCreation) {
    // Create a block entity
    auto entity = world_integration->create_block_entity(10, 70, 10, BlockType::DIAMOND_BLOCK);
    
    ASSERT_NE(entity, ecs::NULL_ENTITY);
    
    // Verify entity has correct components
    auto& registry = world_integration->get_registry();
    EXPECT_TRUE(registry.has<ecs::Block>(entity));
    EXPECT_TRUE(registry.has<ecs::Position>(entity));
    
    // Verify block component data
    const auto& block = registry.get<ecs::Block>(entity);
    EXPECT_EQ(block.universal_id, static_cast<std::uint16_t>(BlockType::DIAMOND_BLOCK));
    
    // Verify position component data
    const auto& position = registry.get<ecs::Position>(entity);
    EXPECT_EQ(position.x, 10);
    EXPECT_EQ(position.y, 70);
    EXPECT_EQ(position.z, 10);
}

TEST_F(WorldECSTest, PlayerEntityCreation) {
    // Create a player entity
    auto entity = world_integration->create_player("TestPlayer", 0, 70, 0);
    
    ASSERT_NE(entity, ecs::NULL_ENTITY);
    
    // Verify entity has correct components
    auto& registry = world_integration->get_registry();
    EXPECT_TRUE(registry.has<ecs::Player>(entity));
    EXPECT_TRUE(registry.has<ecs::Position>(entity));
    EXPECT_TRUE(registry.has<ecs::Velocity>(entity));
    EXPECT_TRUE(registry.has<ecs::Inventory>(entity));
    
    // Verify player component data
    const auto& player = registry.get<ecs::Player>(entity);
    EXPECT_EQ(player.username, "TestPlayer");
    EXPECT_EQ(player.health, 20.0f);
}

TEST_F(WorldECSTest, ChunkSynchronization) {
    // Generate chunk with some blocks
    world->get_chunk(0, 0, true);
    world->set_block(5, 70, 5, BlockType::GOLD_BLOCK);
    world->set_block(10, 75, 10, BlockType::IRON_BLOCK);
    
    // Sync chunk with ECS
    world_integration->sync_chunk_blocks(0, 0);
    
    // Verify block entities were created
    auto gold_entity = world_integration->get_block_entity(5, 70, 5);
    auto iron_entity = world_integration->get_block_entity(10, 75, 10);
    
    EXPECT_TRUE(gold_entity.has_value());
    EXPECT_TRUE(iron_entity.has_value());
}

TEST_F(WorldECSTest, EntityRadiusQuery) {
    // Create several entities at different positions
    world_integration->create_player("Player1", 0, 70, 0);
    world_integration->create_player("Player2", 5, 70, 5);
    world_integration->create_player("Player3", 20, 70, 20);
    world_integration->create_mob("zombie", 2, 70, 2);
    
    // Query entities within radius
    auto entities = world_integration->get_entities_in_radius(0, 70, 0, 10.0);
    
    // Should find 3 entities (Player1, Player2, zombie) within 10 blocks
    EXPECT_GE(entities.size(), 3);
    
    // Query with smaller radius
    auto close_entities = world_integration->get_entities_in_radius(0, 70, 0, 3.0);
    
    // Should find fewer entities
    EXPECT_LT(close_entities.size(), entities.size());
}

// ==================== PERFORMANCE MONITORING TESTS ====================

class WorldPerformanceTest : public WorldECSTest {
protected:
    void SetUp() override {
        WorldECSTest::SetUp();
        monitor = std::make_shared<WorldPerformanceMonitor>(world_integration);
    }
    
    std::shared_ptr<WorldPerformanceMonitor> monitor;
};

TEST_F(WorldPerformanceTest, PerformanceMonitorInitialization) {
    ASSERT_NE(monitor, nullptr);
    
    // Start monitoring
    monitor->start_monitoring();
    
    // Give it some time to collect data
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Stop monitoring
    monitor->stop_monitoring();
    
    // Should have some metrics
    const auto& metrics = monitor->get_metrics();
    EXPECT_GE(metrics.average_fps.load(), 0.0f);
}

TEST_F(WorldPerformanceTest, TimingRecording) {
    // Record some timing data
    monitor->record_timing("test_operation", 1000); // 1ms
    monitor->record_timing("test_operation", 2000); // 2ms
    monitor->record_timing("test_operation", 1500); // 1.5ms
    
    // Record throughput data
    monitor->record_throughput("test_metric", 100);
    
    // Generate performance report
    std::string report = monitor->generate_performance_report();
    EXPECT_FALSE(report.empty());
    EXPECT_NE(report.find("test_operation"), std::string::npos);
}

TEST_F(WorldPerformanceTest, OptimizationRecommendations) {
    // Simulate some performance data
    monitor->record_timing("chunk_generation", 50000); // Slow chunk generation
    monitor->record_throughput("chunks_per_second", 5); // Low throughput
    
    auto recommendations = monitor->get_optimization_recommendations();
    EXPECT_FALSE(recommendations.empty());
}

// ==================== WORLD GENERATOR TESTS ====================

class WorldGeneratorTest : public ::testing::Test {
protected:
    void SetUp() override {
        generator = std::make_unique<OverworldGenerator>(54321);
    }
    
    std::unique_ptr<WorldGenerator> generator;
};

TEST_F(WorldGeneratorTest, BiomeGeneration) {
    // Test biome generation at various coordinates
    auto biome1 = generator->get_biome(0, 0);
    auto biome2 = generator->get_biome(100, 100);
    auto biome3 = generator->get_biome(-50, 75);
    
    // Should generate valid biomes
    EXPECT_NE(biome1, static_cast<BiomeType>(0));
    EXPECT_NE(biome2, static_cast<BiomeType>(0));
    EXPECT_NE(biome3, static_cast<BiomeType>(0));
    
    // Same coordinates should give same biome
    auto biome1_again = generator->get_biome(0, 0);
    EXPECT_EQ(biome1, biome1_again);
}

TEST_F(WorldGeneratorTest, ChunkGeneration) {
    Chunk test_chunk(0, 0);
    
    // Generate terrain
    generator->generate_chunk(test_chunk, 0, 0);
    
    // Verify chunk has some terrain
    bool found_non_air = false;
    for (int y = 0; y < 100 && !found_non_air; y++) {
        auto block_state = test_chunk.get_block(8, y, 8);
        if (block_state.get_block_type() != BlockType::AIR) {
            found_non_air = true;
        }
    }
    
    EXPECT_TRUE(found_non_air);
    
    // Should have bedrock at bottom
    auto bottom_block = test_chunk.get_block(8, 5, 8);
    EXPECT_EQ(bottom_block.get_block_type(), BlockType::BEDROCK);
}

TEST_F(WorldGeneratorTest, SpawnPointGeneration) {
    auto [spawn_x, spawn_y, spawn_z] = generator->get_spawn_point();
    
    // Spawn point should be reasonable
    EXPECT_GT(spawn_y, 60); // Above sea level
    EXPECT_LT(spawn_y, 100); // Not too high
    EXPECT_GE(spawn_x, -100); // Within reasonable range
    EXPECT_LE(spawn_x, 100);
    EXPECT_GE(spawn_z, -100);
    EXPECT_LE(spawn_z, 100);
}

// ==================== INTEGRATION TESTS ====================

class WorldIntegrationTest : public WorldECSTest {
protected:
    void SetUp() override {
        WorldECSTest::SetUp();
        // Start world background tasks for full integration testing
        world->start_background_tasks();
    }
    
    void TearDown() override {
        world->stop_background_tasks();
        WorldECSTest::TearDown();
    }
};

TEST_F(WorldIntegrationTest, PlayerWorldInteraction) {
    // Create a player
    auto player = world_integration->create_player("IntegrationTestPlayer", 0, 70, 0);
    
    // Generate chunk around player
    world->load_chunks_around(0, 0, 2);
    
    // Player should be able to modify blocks
    world_integration->set_block(1, 70, 1, BlockType::DIAMOND_BLOCK, true);
    
    // Verify block was set in both world and ECS
    EXPECT_EQ(world->get_block(1, 70, 1), BlockType::DIAMOND_BLOCK);
    
    auto block_entity = world_integration->get_block_entity(1, 70, 1);
    EXPECT_TRUE(block_entity.has_value());
}

TEST_F(WorldIntegrationTest, MultiPlayerInteraction) {
    // Create multiple players
    auto player1 = world_integration->create_player("Player1", 0, 70, 0);
    auto player2 = world_integration->create_player("Player2", 10, 70, 10);
    
    // Load chunks for both players
    world->load_chunks_around(0, 0, 2);
    world->load_chunks_around(0, 0, 2);
    
    // Both players should be in the system
    auto entities_near_p1 = world_integration->get_entities_in_radius(0, 70, 0, 20.0);
    EXPECT_GE(entities_near_p1.size(), 2); // At least both players
    
    // Update systems to process entity interactions
    world_integration->update_systems(0.016f); // 60 FPS
}

// ==================== STRESS TESTS ====================

TEST_F(WorldIntegrationTest, LargeChunkLoading) {
    // Load a large number of chunks
    const int radius = 5;
    world->load_chunks_around(0, 0, radius);
    
    auto loaded_chunks = world->get_loaded_chunks();
    int expected_chunks = (2 * radius + 1) * (2 * radius + 1);
    
    EXPECT_EQ(loaded_chunks.size(), expected_chunks);
    
    // Verify all chunks are accessible
    for (const auto& coord : loaded_chunks) {
        EXPECT_TRUE(world->is_chunk_loaded(coord.x, coord.z));
    }
}

TEST_F(WorldIntegrationTest, MassiveBlockUpdates) {
    // Generate a chunk
    world->get_chunk(0, 0, true);
    
    const int num_updates = 10000;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Perform massive block updates
    for (int i = 0; i < num_updates; i++) {
        int x = (i % 16);
        int z = ((i / 16) % 16);
        int y = 70 + (i % 20);
        
        BlockType block_type = (i % 2 == 0) ? BlockType::STONE : BlockType::DIRT;
        world_integration->set_block(x, y, z, block_type, true);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Should handle massive updates reasonably quickly
    EXPECT_LT(duration.count(), 10000); // Less than 10 seconds
    
    // Verify some blocks were actually set
    EXPECT_NE(world->get_block(5, 75, 5), BlockType::AIR);
    EXPECT_NE(world->get_block(10, 80, 10), BlockType::AIR);
}

} // namespace test
} // namespace world
} // namespace parallelstone