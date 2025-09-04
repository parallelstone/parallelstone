#include "world/world.hpp"
#include "world/world_generator.hpp"
#include <chrono>
#include <fstream>
#include <algorithm>
#include <spdlog/spdlog.h>

namespace parallelstone {
namespace world {

// ==================== WORLD IMPLEMENTATION ====================

World::World(const WorldConfig& config) 
    : config_(config), generator_(WorldGeneratorFactory::create_generator(config.dimension, config.seed)) {
    
    // Create world directory if it doesn't exist
    auto world_path = get_world_path();
    if (!std::filesystem::exists(world_path)) {
        std::filesystem::create_directories(world_path);
        spdlog::info("Created world directory: {}", world_path.string());
    }
    
    spdlog::info("Initialized world '{}' (dimension {}, seed {})", 
                 config_.world_name, static_cast<int>(config_.dimension), config_.seed);
}

World::~World() {
    stop_background_tasks();
    save_all_chunks();
    spdlog::info("World '{}' shutdown complete", config_.world_name);
}

std::shared_ptr<Chunk> World::get_chunk(std::int32_t x, std::int32_t z, bool generate_if_missing) {
    ChunkCoord coord(x, z);
    
    // Try to find existing chunk first (read lock)
    {
        std::shared_lock lock(chunks_mutex_);
        auto it = loaded_chunks_.find(coord);
        if (it != loaded_chunks_.end()) {
            return it->second;
        }
    }
    
    // Chunk not loaded - acquire write lock
    std::unique_lock lock(chunks_mutex_);
    
    // Double-check pattern
    auto it = loaded_chunks_.find(coord);
    if (it != loaded_chunks_.end()) {
        return it->second;
    }
    
    // Try loading from disk first
    auto chunk = load_chunk_from_disk(x, z);
    if (chunk) {
        loaded_chunks_[coord] = chunk;
        stats_.chunks_loaded++;
        spdlog::debug("Loaded chunk ({}, {}) from disk", x, z);
        return chunk;
    }
    
    // Generate new chunk if requested
    if (generate_if_missing) {
        chunk = generate_chunk(x, z);
        loaded_chunks_[coord] = chunk;
        stats_.chunks_generated++;
        spdlog::debug("Generated new chunk ({}, {})", x, z);
        return chunk;
    }
    
    return nullptr;
}

void World::unload_chunk(std::int32_t x, std::int32_t z, bool save_before_unload) {
    ChunkCoord coord(x, z);
    std::unique_lock lock(chunks_mutex_);
    
    auto it = loaded_chunks_.find(coord);
    if (it == loaded_chunks_.end()) {
        return;
    }
    
    if (save_before_unload) {
        save_chunk(x, z);
    }
    
    loaded_chunks_.erase(it);
    spdlog::debug("Unloaded chunk ({}, {})", x, z);
}

bool World::is_chunk_loaded(std::int32_t x, std::int32_t z) const {
    ChunkCoord coord(x, z);
    std::shared_lock lock(chunks_mutex_);
    return loaded_chunks_.find(coord) != loaded_chunks_.end();
}

std::vector<ChunkCoord> World::get_loaded_chunks() const {
    std::shared_lock lock(chunks_mutex_);
    std::vector<ChunkCoord> chunks;
    chunks.reserve(loaded_chunks_.size());
    
    for (const auto& [coord, chunk] : loaded_chunks_) {
        chunks.push_back(coord);
    }
    
    return chunks;
}

void World::load_chunks_around(std::int32_t center_x, std::int32_t center_z, std::uint32_t radius) {
    auto start = std::chrono::high_resolution_clock::now();
    
    std::vector<std::future<void>> futures;
    
    for (std::int32_t dx = -static_cast<std::int32_t>(radius); dx <= static_cast<std::int32_t>(radius); ++dx) {
        for (std::int32_t dz = -static_cast<std::int32_t>(radius); dz <= static_cast<std::int32_t>(radius); ++dz) {
            // Skip chunks outside circular radius
            if (dx * dx + dz * dz > static_cast<std::int32_t>(radius * radius)) {
                continue;
            }
            
            std::int32_t chunk_x = center_x + dx;
            std::int32_t chunk_z = center_z + dz;
            
            // Load chunk asynchronously if not already loaded
            if (!is_chunk_loaded(chunk_x, chunk_z)) {
                auto future = std::async(std::launch::async, [this, chunk_x, chunk_z]() {
                    get_chunk(chunk_x, chunk_z, true);
                });
                futures.push_back(std::move(future));
            }
        }
    }
    
    // Wait for all chunks to load
    for (auto& future : futures) {
        future.wait();
    }
    
    auto duration = std::chrono::high_resolution_clock::now() - start;
    auto duration_us = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
    
    spdlog::info("Loaded chunks in {}x{} area around ({}, {}) in {} us", 
                 radius * 2 + 1, radius * 2 + 1, center_x, center_z, duration_us);
}

void World::unload_chunks_outside(std::int32_t center_x, std::int32_t center_z, std::uint32_t radius) {
    auto chunks_to_unload = get_loaded_chunks();
    std::uint32_t unloaded_count = 0;
    
    for (const auto& coord : chunks_to_unload) {
        std::int32_t dx = coord.x - center_x;
        std::int32_t dz = coord.z - center_z;
        
        if (dx * dx + dz * dz > static_cast<std::int32_t>(radius * radius)) {
            unload_chunk(coord.x, coord.z, true);
            unloaded_count++;
        }
    }
    
    if (unloaded_count > 0) {
        spdlog::info("Unloaded {} chunks outside radius {} from ({}, {})", 
                     unloaded_count, radius, center_x, center_z);
    }
}

BlockType World::get_block(std::int32_t x, std::int32_t y, std::int32_t z) {
    auto chunk_coord = world_to_chunk(x, z);
    auto chunk = get_chunk(chunk_coord.x, chunk_coord.z, false);
    
    if (!chunk) {
        return BlockType::AIR;
    }
    
    auto chunk_x = world_to_chunk_relative(x);
    auto chunk_z = world_to_chunk_relative(z);
    
    if (!Chunk::is_valid_y(y)) {
        return BlockType::AIR;
    }
    
    auto block_state = chunk->get_block(chunk_x, y, chunk_z);
    return block_state.get_block_type();
}

void World::set_block(std::int32_t x, std::int32_t y, std::int32_t z, BlockType block, 
                     bool update_lighting, bool notify_neighbors) {
    auto chunk_coord = world_to_chunk(x, z);
    auto chunk = get_chunk(chunk_coord.x, chunk_coord.z, true);
    
    if (!chunk || !Chunk::is_valid_y(y)) {
        return;
    }
    
    auto chunk_x = world_to_chunk_relative(x);
    auto chunk_z = world_to_chunk_relative(z);
    
    // Create block state from block type
    BlockState new_state(block);
    chunk->set_block(chunk_x, y, chunk_z, new_state);
    
    stats_.blocks_changed++;
    
    if (update_lighting) {
        // Update lighting in a small area around the changed block
        update_lighting(x - 1, y - 1, z - 1, x + 1, y + 1, z + 1);
    }
    
    if (notify_neighbors) {
        // Notify neighboring chunks if block is on chunk boundary
        if (chunk_x == 0 && is_chunk_loaded(chunk_coord.x - 1, chunk_coord.z)) {
            // Left neighbor
        }
        if (chunk_x == 15 && is_chunk_loaded(chunk_coord.x + 1, chunk_coord.z)) {
            // Right neighbor
        }
        if (chunk_z == 0 && is_chunk_loaded(chunk_coord.x, chunk_coord.z - 1)) {
            // Front neighbor
        }
        if (chunk_z == 15 && is_chunk_loaded(chunk_coord.x, chunk_coord.z + 1)) {
            // Back neighbor
        }
    }
}

std::int32_t World::get_height(std::int32_t x, std::int32_t z) {
    auto chunk_coord = world_to_chunk(x, z);
    auto chunk = get_chunk(chunk_coord.x, chunk_coord.z, false);
    
    if (!chunk) {
        return 64; // Default sea level
    }
    
    auto chunk_x = world_to_chunk_relative(x);
    auto chunk_z = world_to_chunk_relative(z);
    
    return chunk->get_height(chunk_x, chunk_z);
}

void World::save_all_chunks() {
    auto chunks = get_loaded_chunks();
    spdlog::info("Saving {} chunks to disk", chunks.size());
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (const auto& coord : chunks) {
        save_chunk(coord.x, coord.z);
    }
    
    auto duration = std::chrono::high_resolution_clock::now() - start;
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    
    spdlog::info("Saved all chunks in {} ms", duration_ms);
}

void World::save_chunk(std::int32_t x, std::int32_t z) {
    ChunkCoord coord(x, z);
    std::shared_lock lock(chunks_mutex_);
    
    auto it = loaded_chunks_.find(coord);
    if (it == loaded_chunks_.end()) {
        return;
    }
    
    auto chunk = it->second;
    lock.unlock();
    
    // Create chunk file path
    auto chunk_path = get_world_path() / "region" / 
                     fmt::format("r.{}.{}.dat", x >> 5, z >> 5); // 32x32 region files
    
    // Ensure region directory exists
    std::filesystem::create_directories(chunk_path.parent_path());
    
    // TODO: Implement actual chunk serialization format (NBT or custom binary)
    // For now, just create empty file to indicate chunk exists
    std::ofstream file(chunk_path, std::ios::binary | std::ios::app);
    if (file.is_open()) {
        stats_.chunks_saved++;
        spdlog::debug("Saved chunk ({}, {}) to {}", x, z, chunk_path.string());
    } else {
        spdlog::warn("Failed to save chunk ({}, {}) to {}", x, z, chunk_path.string());
    }
}

std::shared_ptr<Chunk> World::load_chunk_from_disk(std::int32_t x, std::int32_t z) {
    if (!chunk_exists_on_disk(x, z)) {
        return nullptr;
    }
    
    // TODO: Implement actual chunk deserialization
    // For now, return nullptr to force generation
    return nullptr;
}

bool World::chunk_exists_on_disk(std::int32_t x, std::int32_t z) const {
    auto chunk_path = get_world_path() / "region" / 
                     fmt::format("r.{}.{}.dat", x >> 5, z >> 5);
    return std::filesystem::exists(chunk_path);
}

std::filesystem::path World::get_world_path() const {
    return config_.world_directory / config_.world_name / 
           fmt::format("DIM{}", static_cast<int>(config_.dimension));
}

std::shared_ptr<Chunk> World::generate_chunk(std::int32_t x, std::int32_t z) {
    auto start = std::chrono::high_resolution_clock::now();
    
    auto chunk = std::make_shared<Chunk>(x, z);
    generator_->generate_chunk(*chunk, x, z);
    generator_->populate_chunk(*chunk, x, z);
    
    auto duration = std::chrono::high_resolution_clock::now() - start;
    auto duration_us = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
    
    stats_.chunk_generation_time_us += duration_us;
    
    return chunk;
}

void World::start_background_tasks() {
    if (chunk_management_thread_ || auto_save_thread_) {
        return; // Already started
    }
    
    shutdown_requested_ = false;
    
    chunk_management_thread_ = std::make_unique<std::thread>([this]() {
        chunk_management_thread();
    });
    
    if (config_.auto_save_enabled) {
        auto_save_thread_ = std::make_unique<std::thread>([this]() {
            auto_save_thread();
        });
    }
    
    spdlog::info("Started background tasks for world '{}'", config_.world_name);
}

void World::stop_background_tasks() {
    shutdown_requested_ = true;
    management_cv_.notify_all();
    
    if (chunk_management_thread_ && chunk_management_thread_->joinable()) {
        chunk_management_thread_->join();
        chunk_management_thread_.reset();
    }
    
    if (auto_save_thread_ && auto_save_thread_->joinable()) {
        auto_save_thread_->join();
        auto_save_thread_.reset();
    }
    
    spdlog::info("Stopped background tasks for world '{}'", config_.world_name);
}

void World::chunk_management_thread() {
    while (!shutdown_requested_) {
        std::unique_lock lock(management_mutex_);
        
        // Wait for work or shutdown signal
        if (management_cv_.wait_for(lock, std::chrono::seconds(1)) == std::cv_status::timeout) {
            // Perform periodic maintenance
            tick();
        }
    }
}

void World::auto_save_thread() {
    while (!shutdown_requested_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(config_.auto_save_interval_ms));
        
        if (!shutdown_requested_) {
            save_all_chunks();
        }
    }
}

void World::tick() {
    // Cleanup empty chunks that haven't been accessed recently
    auto chunks = get_loaded_chunks();
    if (chunks.size() > config_.max_loaded_chunks) {
        // TODO: Implement LRU-based chunk unloading
        spdlog::debug("World has {} loaded chunks (max: {})", chunks.size(), config_.max_loaded_chunks);
    }
    
    // Update lighting for dirty chunks
    // TODO: Implement lighting update queue
}

// ==================== WORLD MANAGER IMPLEMENTATION ====================

WorldManager::WorldManager(const std::filesystem::path& worlds_directory) 
    : worlds_directory_(worlds_directory) {
    
    if (!std::filesystem::exists(worlds_directory_)) {
        std::filesystem::create_directories(worlds_directory_);
        spdlog::info("Created worlds directory: {}", worlds_directory_.string());
    }
}

std::shared_ptr<World> WorldManager::get_world(DimensionType dimension, const std::string& world_name) {
    std::shared_lock lock(worlds_mutex_);
    
    auto it = worlds_.find(dimension);
    if (it != worlds_.end()) {
        return it->second;
    }
    
    lock.unlock();
    
    // Create world with default configuration
    WorldConfig config;
    config.world_name = world_name;
    config.world_directory = worlds_directory_;
    config.dimension = dimension;
    config.seed = std::random_device{}(); // Random seed
    
    return create_world(config);
}

std::shared_ptr<World> WorldManager::create_world(const WorldConfig& config) {
    std::unique_lock lock(worlds_mutex_);
    
    auto world = std::make_shared<World>(config);
    worlds_[config.dimension] = world;
    
    // Start background tasks
    world->start_background_tasks();
    
    spdlog::info("Created world '{}' for dimension {}", config.world_name, static_cast<int>(config.dimension));
    
    return world;
}

void WorldManager::save_all_worlds() {
    std::shared_lock lock(worlds_mutex_);
    
    for (const auto& [dimension, world] : worlds_) {
        world->save_all_chunks();
    }
}

void WorldManager::shutdown() {
    std::unique_lock lock(worlds_mutex_);
    
    for (const auto& [dimension, world] : worlds_) {
        world->stop_background_tasks();
        world->save_all_chunks();
    }
    
    worlds_.clear();
    spdlog::info("World manager shutdown complete");
}

WorldStats WorldManager::get_aggregate_stats() const {
    std::shared_lock lock(worlds_mutex_);
    
    WorldStats aggregate;
    for (const auto& [dimension, world] : worlds_) {
        const auto& stats = world->stats();
        aggregate.chunks_loaded += stats.chunks_loaded.load();
        aggregate.chunks_generated += stats.chunks_generated.load();
        aggregate.chunks_saved += stats.chunks_saved.load();
        aggregate.blocks_changed += stats.blocks_changed.load();
        aggregate.lighting_updates += stats.lighting_updates.load();
    }
    
    return aggregate;
}

} // namespace world
} // namespace parallelstone