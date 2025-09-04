#include <benchmark/benchmark.h>
#include "world/world.hpp"
#include "world/world_ecs_integration.hpp"
#include "world/world_performance.hpp"
#include <memory>
#include <random>

namespace parallelstone {
namespace world {
namespace benchmark {

// Test fixture for world benchmarks
class WorldBenchmarkFixture : public ::benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        // Configure test world
        config.world_name = "benchmark_world";
        config.world_directory = std::filesystem::temp_directory_path() / "parallelstone_benchmarks";
        config.dimension = DimensionType::OVERWORLD;
        config.seed = 98765;
        config.max_loaded_chunks = 500;
        config.auto_save_enabled = false;
        
        world = std::make_shared<World>(config);
        world_integration = std::make_shared<WorldECSIntegration>(world);
        
        // Pre-generate some chunks for tests that need them
        world->load_chunks_around(0, 0, 3);
    }
    
    void TearDown(const ::benchmark::State& state) override {
        world_integration.reset();
        world.reset();
        
        // Clean up
        if (std::filesystem::exists(config.world_directory)) {
            std::filesystem::remove_all(config.world_directory);
        }
    }

protected:
    WorldConfig config;
    std::shared_ptr<World> world;
    std::shared_ptr<WorldECSIntegration> world_integration;
};

// ==================== CHUNK GENERATION BENCHMARKS ====================

BENCHMARK_F(WorldBenchmarkFixture, ChunkGeneration_Single)(benchmark::State& state) {
    int chunk_counter = 0;
    
    for (auto _ : state) {
        int chunk_x = (chunk_counter % 100) - 50;
        int chunk_z = ((chunk_counter / 100) % 100) - 50;
        
        auto chunk = world->get_chunk(chunk_x, chunk_z, true);
        benchmark::DoNotOptimize(chunk);
        
        chunk_counter++;
    }
    
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_F(WorldBenchmarkFixture, ChunkGeneration_Batch)(benchmark::State& state) {
    const int batch_size = state.range(0);
    
    for (auto _ : state) {
        int start_x = (state.iterations() * batch_size) % 100 - 50;
        int start_z = ((state.iterations() * batch_size) / 100) % 100 - 50;
        
        for (int i = 0; i < batch_size; i++) {
            int chunk_x = start_x + (i % 10);
            int chunk_z = start_z + (i / 10);
            
            auto chunk = world->get_chunk(chunk_x, chunk_z, true);
            benchmark::DoNotOptimize(chunk);
        }
    }
    
    state.SetItemsProcessed(state.iterations() * batch_size);
}
BENCHMARK_REGISTER_F(WorldBenchmarkFixture, ChunkGeneration_Batch)
    ->RangeMultiplier(4)->Range(4, 64);

// ==================== BLOCK OPERATION BENCHMARKS ====================

BENCHMARK_F(WorldBenchmarkFixture, BlockGet_Sequential)(benchmark::State& state) {
    for (auto _ : state) {
        for (int i = 0; i < 1000; i++) {
            int x = i % 16;
            int z = (i / 16) % 16;
            int y = 70 + (i % 10);
            
            auto block = world->get_block(x, y, z);
            benchmark::DoNotOptimize(block);
        }
    }
    
    state.SetItemsProcessed(state.iterations() * 1000);
}

BENCHMARK_F(WorldBenchmarkFixture, BlockGet_Random)(benchmark::State& state) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> coord_dist(-48, 47); // 6x6 chunk area
    std::uniform_int_distribution<int> y_dist(60, 90);
    
    for (auto _ : state) {
        for (int i = 0; i < 1000; i++) {
            int x = coord_dist(gen);
            int y = y_dist(gen);
            int z = coord_dist(gen);
            
            auto block = world->get_block(x, y, z);
            benchmark::DoNotOptimize(block);
        }
    }
    
    state.SetItemsProcessed(state.iterations() * 1000);
}

BENCHMARK_F(WorldBenchmarkFixture, BlockSet_Sequential)(benchmark::State& state) {
    const std::vector<BlockType> test_blocks = {
        BlockType::STONE, BlockType::DIRT, BlockType::GRASS_BLOCK, 
        BlockType::COBBLESTONE, BlockType::OAK_PLANKS
    };
    
    for (auto _ : state) {
        for (int i = 0; i < 1000; i++) {
            int x = i % 16;
            int z = (i / 16) % 16;
            int y = 70 + (i % 10);
            
            BlockType block_type = test_blocks[i % test_blocks.size()];
            world->set_block(x, y, z, block_type, false); // Skip lighting updates for performance
        }
    }
    
    state.SetItemsProcessed(state.iterations() * 1000);
}

BENCHMARK_F(WorldBenchmarkFixture, BlockSet_WithLighting)(benchmark::State& state) {
    const std::vector<BlockType> test_blocks = {
        BlockType::STONE, BlockType::DIRT, BlockType::GRASS_BLOCK
    };
    
    for (auto _ : state) {
        for (int i = 0; i < 100; i++) { // Fewer iterations due to lighting cost
            int x = i % 16;
            int z = (i / 16) % 16;
            int y = 70 + (i % 5);
            
            BlockType block_type = test_blocks[i % test_blocks.size()];
            world->set_block(x, y, z, block_type, true); // Include lighting updates
        }
    }
    
    state.SetItemsProcessed(state.iterations() * 100);
}

// ==================== ECS BENCHMARKS ====================

BENCHMARK_F(WorldBenchmarkFixture, ECS_BlockEntityCreation)(benchmark::State& state) {
    const std::vector<BlockType> test_blocks = {
        BlockType::DIAMOND_BLOCK, BlockType::GOLD_BLOCK, BlockType::IRON_BLOCK,
        BlockType::EMERALD_BLOCK, BlockType::LAPIS_BLOCK
    };
    
    int entity_counter = 0;
    
    for (auto _ : state) {
        int x = (entity_counter % 16);
        int z = ((entity_counter / 16) % 16);
        int y = 70 + (entity_counter % 20);
        
        BlockType block_type = test_blocks[entity_counter % test_blocks.size()];
        auto entity = world_integration->create_block_entity(x, y, z, block_type);
        benchmark::DoNotOptimize(entity);
        
        entity_counter++;
    }
    
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_F(WorldBenchmarkFixture, ECS_PlayerEntityCreation)(benchmark::State& state) {
    int player_counter = 0;
    
    for (auto _ : state) {
        std::string username = "Player" + std::to_string(player_counter);
        int x = (player_counter % 20) - 10;
        int z = ((player_counter / 20) % 20) - 10;
        
        auto entity = world_integration->create_player(username, x, 70, z);
        benchmark::DoNotOptimize(entity);
        
        player_counter++;
    }
    
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_F(WorldBenchmarkFixture, ECS_EntityRadiusQuery)(benchmark::State& state) {
    // Pre-create entities
    for (int i = 0; i < 1000; i++) {
        std::string username = "Player" + std::to_string(i);
        int x = (i % 40) - 20;
        int z = ((i / 40) % 40) - 20;
        world_integration->create_player(username, x, 70, z);
    }
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> coord_dist(-15, 15);
    std::uniform_real_distribution<double> radius_dist(5.0, 25.0);
    
    for (auto _ : state) {
        int center_x = coord_dist(gen);
        int center_z = coord_dist(gen);
        double radius = radius_dist(gen);
        
        auto entities = world_integration->get_entities_in_radius(center_x, 70, center_z, radius);
        benchmark::DoNotOptimize(entities);
    }
    
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_F(WorldBenchmarkFixture, ECS_SystemUpdate)(benchmark::State& state) {
    // Pre-create entities for systems to process
    for (int i = 0; i < 500; i++) {
        std::string username = "Player" + std::to_string(i);
        int x = (i % 30) - 15;
        int z = ((i / 30) % 30) - 15;
        world_integration->create_player(username, x, 70, z);
    }
    
    for (int i = 0; i < 1000; i++) {
        int x = (i % 16);
        int z = ((i / 16) % 16);
        int y = 70 + (i % 10);
        world_integration->create_block_entity(x, y, z, BlockType::STONE);
    }
    
    const float delta_time = 0.016f; // 60 FPS
    
    for (auto _ : state) {
        world_integration->update_systems(delta_time);
    }
    
    state.SetItemsProcessed(state.iterations());
}

// ==================== CHUNK LOADING BENCHMARKS ====================

BENCHMARK_F(WorldBenchmarkFixture, ChunkLoading_Area)(benchmark::State& state) {
    const int radius = state.range(0);
    int center_offset = 0;
    
    for (auto _ : state) {
        // Use different centers to avoid caching effects
        int center_x = (center_offset % 10) - 5;
        int center_z = ((center_offset / 10) % 10) - 5;
        
        world->load_chunks_around(center_x * 20, center_z * 20, radius);
        
        center_offset++;
    }
    
    int chunks_per_iteration = (2 * radius + 1) * (2 * radius + 1);
    state.SetItemsProcessed(state.iterations() * chunks_per_iteration);
}
BENCHMARK_REGISTER_F(WorldBenchmarkFixture, ChunkLoading_Area)
    ->RangeMultiplier(2)->Range(1, 8);

BENCHMARK_F(WorldBenchmarkFixture, ChunkUnloading_Area)(benchmark::State& state) {
    const int load_radius = 10;
    const int unload_radius = state.range(0);
    
    for (auto _ : state) {
        // Load a large area first
        world->load_chunks_around(0, 0, load_radius);
        
        // Then unload outside smaller radius
        world->unload_chunks_outside(0, 0, unload_radius);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK_REGISTER_F(WorldBenchmarkFixture, ChunkUnloading_Area)
    ->RangeMultiplier(2)->Range(1, 6);

// ==================== MEMORY PERFORMANCE BENCHMARKS ====================

BENCHMARK_F(WorldBenchmarkFixture, Memory_ChunkAllocation)(benchmark::State& state) {
    std::vector<std::shared_ptr<Chunk>> chunks;
    chunks.reserve(state.iterations());
    
    for (auto _ : state) {
        auto chunk = std::make_shared<Chunk>(rand() % 1000 - 500, rand() % 1000 - 500);
        chunks.push_back(chunk);
        benchmark::DoNotOptimize(chunk);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * sizeof(Chunk));
}

BENCHMARK_F(WorldBenchmarkFixture, Memory_BlockStateCreation)(benchmark::State& state) {
    const std::vector<BlockType> test_blocks = {
        BlockType::STONE, BlockType::DIRT, BlockType::GRASS_BLOCK,
        BlockType::WATER, BlockType::LAVA, BlockType::BEDROCK
    };
    
    for (auto _ : state) {
        for (int i = 0; i < 1000; i++) {
            BlockType block_type = test_blocks[i % test_blocks.size()];
            BlockState state_obj(block_type);
            benchmark::DoNotOptimize(state_obj);
        }
    }
    
    state.SetItemsProcessed(state.iterations() * 1000);
}

// ==================== PERFORMANCE MONITORING BENCHMARKS ====================

BENCHMARK_F(WorldBenchmarkFixture, Performance_MonitorOverhead)(benchmark::State& state) {
    auto monitor = std::make_shared<WorldPerformanceMonitor>(world_integration);
    monitor->start_monitoring();
    
    for (auto _ : state) {
        // Simulate normal world operations
        world->get_block(rand() % 16, 70, rand() % 16);
        world->set_block(rand() % 16, 70, rand() % 16, BlockType::STONE);
        
        // Record some metrics
        monitor->record_timing("test_operation", rand() % 1000);
        monitor->record_throughput("test_metric", rand() % 100);
    }
    
    monitor->stop_monitoring();
    state.SetItemsProcessed(state.iterations());
}

// ==================== COMPREHENSIVE BENCHMARKS ====================

BENCHMARK_F(WorldBenchmarkFixture, Comprehensive_WorldSimulation)(benchmark::State& state) {
    // Create a realistic world scenario
    const int num_players = state.range(0);
    
    // Create players
    std::vector<ecs::Entity> players;
    for (int i = 0; i < num_players; i++) {
        std::string username = "Player" + std::to_string(i);
        int x = (i % 10) * 20 - 100;
        int z = (i / 10) * 20 - 100;
        auto player = world_integration->create_player(username, x, 70, z);
        players.push_back(player);
    }
    
    const float delta_time = 0.05f; // 20 TPS server tick
    
    for (auto _ : state) {
        // Simulate one server tick
        
        // Update player positions (simulate movement)
        for (auto player : players) {
            if (world_integration->get_registry().has<ecs::Position>(player)) {
                auto& pos = world_integration->get_registry().get<ecs::Position>(player);
                pos.x += (rand() % 3 - 1) * 0.1f; // Small random movement
                pos.z += (rand() % 3 - 1) * 0.1f;
                
                // Load chunks around player
                int chunk_x = static_cast<int>(pos.x) >> 4;
                int chunk_z = static_cast<int>(pos.z) >> 4;
                world->load_chunks_around(chunk_x, chunk_z, 3);
            }
        }
        
        // Simulate some block changes
        for (int i = 0; i < 10; i++) {
            int x = rand() % 32 - 16;
            int y = 70 + (rand() % 10);
            int z = rand() % 32 - 16;
            
            BlockType block = (rand() % 2 == 0) ? BlockType::STONE : BlockType::DIRT;
            world->set_block(x, y, z, block, false);
        }
        
        // Update ECS systems
        world_integration->update_systems(delta_time);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK_REGISTER_F(WorldBenchmarkFixture, Comprehensive_WorldSimulation)
    ->RangeMultiplier(2)->Range(1, 16);

} // namespace benchmark
} // namespace world
} // namespace parallelstone

// Register all benchmarks
BENCHMARK_MAIN();