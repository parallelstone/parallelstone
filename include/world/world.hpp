#pragma once

#include "chunk_section.hpp"
#include "compile_time_blocks.hpp"
#include "block_registry.hpp"
#include "world_generator.hpp"
#include <unordered_map>
#include <memory>
#include <string>
#include <mutex>
#include <thread>
#include <atomic>
#include <filesystem>

namespace parallelstone {
namespace world {

/**
 * @brief World dimension types supported by Minecraft
 */
enum class DimensionType : std::uint8_t {
    OVERWORLD = 0,
    NETHER = 1,
    END = 2
};

/**
 * @brief Chunk coordinate pair for hash map key
 */
struct ChunkCoord {
    std::int32_t x;
    std::int32_t z;
    
    constexpr ChunkCoord(std::int32_t x, std::int32_t z) noexcept : x(x), z(z) {}
    
    constexpr bool operator==(const ChunkCoord& other) const noexcept {
        return x == other.x && z == other.z;
    }
};

/**
 * @brief Hash function for ChunkCoord
 */
struct ChunkCoordHash {
    constexpr std::size_t operator()(const ChunkCoord& coord) const noexcept {
        // Cantor pairing function for unique hash
        auto sum = static_cast<std::uint64_t>(coord.x + coord.z);
        return (sum * (sum + 1) / 2) + static_cast<std::uint64_t>(coord.z);
    }
};

/**
 * @brief World configuration parameters
 */
struct WorldConfig {
    std::string world_name = "world";
    std::filesystem::path world_directory = "worlds";
    DimensionType dimension = DimensionType::OVERWORLD;
    std::uint64_t seed = 0;
    
    // Performance settings
    std::uint32_t max_loaded_chunks = 1024;
    std::uint32_t chunk_view_distance = 16;
    std::uint32_t simulation_distance = 10;
    bool auto_save_enabled = true;
    std::uint32_t auto_save_interval_ms = 30000; // 30 seconds
    
    // Generation settings
    bool generate_structures = true;
    bool generate_decorations = true;
    std::string world_type = "default";
};

/**
 * @brief Statistics for world performance monitoring
 */
struct WorldStats {
    std::atomic<std::uint64_t> chunks_loaded{0};
    std::atomic<std::uint64_t> chunks_generated{0};
    std::atomic<std::uint64_t> chunks_saved{0};
    std::atomic<std::uint64_t> blocks_changed{0};
    std::atomic<std::uint64_t> lighting_updates{0};
    
    // Performance metrics
    std::atomic<std::uint64_t> chunk_load_time_us{0};
    std::atomic<std::uint64_t> chunk_generation_time_us{0};
    std::atomic<std::uint64_t> lighting_calculation_time_us{0};
};

/**
 * @brief Main world management class
 * 
 * Handles chunk loading, unloading, generation, persistence, and provides
 * thread-safe access to world data. Integrates with the ECS system for
 * block and entity management.
 */
class World {
public:
    /**
     * @brief Create or load a world with the given configuration
     */
    explicit World(const WorldConfig& config);
    
    /**
     * @brief Destructor - saves all chunks and shuts down threads
     */
    ~World();
    
    // Non-copyable but movable
    World(const World&) = delete;
    World& operator=(const World&) = delete;
    World(World&&) = default;
    World& operator=(World&&) = default;
    
    /**
     * @brief Get world configuration
     */
    const WorldConfig& config() const noexcept { return config_; }
    
    /**
     * @brief Get world statistics
     */
    const WorldStats& stats() const noexcept { return stats_; }
    
    // ==================== CHUNK MANAGEMENT ====================
    
    /**
     * @brief Load or generate chunk at coordinates
     * @param x Chunk X coordinate
     * @param z Chunk Z coordinate
     * @param generate_if_missing Generate chunk if it doesn't exist
     * @return Shared pointer to chunk, nullptr if not found and generation disabled
     */
    std::shared_ptr<Chunk> get_chunk(std::int32_t x, std::int32_t z, bool generate_if_missing = true);
    
    /**
     * @brief Unload chunk at coordinates
     * @param x Chunk X coordinate
     * @param z Chunk Z coordinate
     * @param save_before_unload Save chunk to disk before unloading
     */
    void unload_chunk(std::int32_t x, std::int32_t z, bool save_before_unload = true);
    
    /**
     * @brief Check if chunk is loaded
     */
    bool is_chunk_loaded(std::int32_t x, std::int32_t z) const;
    
    /**
     * @brief Get all loaded chunk coordinates
     */
    std::vector<ChunkCoord> get_loaded_chunks() const;
    
    /**
     * @brief Load chunks in radius around center
     */
    void load_chunks_around(std::int32_t center_x, std::int32_t center_z, std::uint32_t radius);
    
    /**
     * @brief Unload chunks outside radius from center
     */
    void unload_chunks_outside(std::int32_t center_x, std::int32_t center_z, std::uint32_t radius);
    
    // ==================== BLOCK ACCESS ====================
    
    /**
     * @brief Get block at world coordinates
     */
    BlockType get_block(std::int32_t x, std::int32_t y, std::int32_t z);
    
    /**
     * @brief Set block at world coordinates
     * @param x World X coordinate
     * @param y World Y coordinate (must be valid for chunk)
     * @param z World Z coordinate
     * @param block Block type to set
     * @param update_lighting Whether to update lighting calculations
     * @param notify_neighbors Whether to notify neighboring chunks
     */
    void set_block(std::int32_t x, std::int32_t y, std::int32_t z, BlockType block, 
                   bool update_lighting = true, bool notify_neighbors = true);
    
    /**
     * @brief Get multiple blocks in a region
     */
    void get_blocks(std::int32_t start_x, std::int32_t start_y, std::int32_t start_z,
                   std::int32_t end_x, std::int32_t end_y, std::int32_t end_z,
                   std::vector<BlockType>& blocks);
    
    /**
     * @brief Set multiple blocks in a region
     */
    void set_blocks(std::int32_t start_x, std::int32_t start_y, std::int32_t start_z,
                   const std::vector<BlockType>& blocks, std::int32_t width, std::int32_t height);
    
    /**
     * @brief Get surface height at x,z coordinates
     */
    std::int32_t get_height(std::int32_t x, std::int32_t z);
    
    // ==================== LIGHTING ====================
    
    /**
     * @brief Get block light level at coordinates
     */
    std::uint8_t get_block_light(std::int32_t x, std::int32_t y, std::int32_t z);
    
    /**
     * @brief Get sky light level at coordinates
     */
    std::uint8_t get_sky_light(std::int32_t x, std::int32_t y, std::int32_t z);
    
    /**
     * @brief Update lighting in region
     */
    void update_lighting(std::int32_t start_x, std::int32_t start_y, std::int32_t start_z,
                        std::int32_t end_x, std::int32_t end_y, std::int32_t end_z);
    
    /**
     * @brief Recalculate lighting for entire chunk
     */
    void recalculate_chunk_lighting(std::int32_t chunk_x, std::int32_t chunk_z);
    
    // ==================== PERSISTENCE ====================
    
    /**
     * @brief Save all loaded chunks to disk
     */
    void save_all_chunks();
    
    /**
     * @brief Save specific chunk to disk
     */
    void save_chunk(std::int32_t x, std::int32_t z);
    
    /**
     * @brief Load chunk from disk
     * @return Loaded chunk or nullptr if not found
     */
    std::shared_ptr<Chunk> load_chunk_from_disk(std::int32_t x, std::int32_t z);
    
    /**
     * @brief Check if chunk exists on disk
     */
    bool chunk_exists_on_disk(std::int32_t x, std::int32_t z) const;
    
    /**
     * @brief Get world save directory
     */
    std::filesystem::path get_world_path() const;
    
    // ==================== WORLD MANAGEMENT ====================
    
    /**
     * @brief Start background threads for chunk management
     */
    void start_background_tasks();
    
    /**
     * @brief Stop all background threads
     */
    void stop_background_tasks();
    
    /**
     * @brief Perform maintenance tasks (cleanup, saving, etc.)
     */
    void tick();
    
    /**
     * @brief Get world generator for this dimension
     */
    WorldGenerator* get_generator() const { return generator_.get(); }
    
    // ==================== COORDINATE CONVERSION ====================
    
    /**
     * @brief Convert world coordinates to chunk coordinates
     */
    static constexpr ChunkCoord world_to_chunk(std::int32_t x, std::int32_t z) noexcept {
        return ChunkCoord(x >> 4, z >> 4); // Divide by 16
    }
    
    /**
     * @brief Convert world coordinates to chunk-relative coordinates
     */
    static constexpr std::uint8_t world_to_chunk_relative(std::int32_t coord) noexcept {
        return static_cast<std::uint8_t>(coord & 15); // Modulo 16
    }
    
    /**
     * @brief Convert chunk coordinates to world coordinates (chunk origin)
     */
    static constexpr std::int32_t chunk_to_world(std::int32_t chunk_coord) noexcept {
        return chunk_coord << 4; // Multiply by 16
    }

private:
    /**
     * @brief Background thread function for chunk management
     */
    void chunk_management_thread();
    
    /**
     * @brief Background thread function for auto-save
     */
    void auto_save_thread();
    
    /**
     * @brief Generate new chunk at coordinates
     */
    std::shared_ptr<Chunk> generate_chunk(std::int32_t x, std::int32_t z);
    
    /**
     * @brief Update statistics after operation
     */
    void update_stats(const std::string& operation, std::uint64_t duration_us);
    
    // Configuration and state
    WorldConfig config_;
    WorldStats stats_;
    
    // Chunk storage
    std::unordered_map<ChunkCoord, std::shared_ptr<Chunk>, ChunkCoordHash> loaded_chunks_;
    mutable std::shared_mutex chunks_mutex_;
    
    // World generation
    std::unique_ptr<WorldGenerator> generator_;
    
    // Background threads
    std::atomic<bool> shutdown_requested_{false};
    std::unique_ptr<std::thread> chunk_management_thread_;
    std::unique_ptr<std::thread> auto_save_thread_;
    
    // Thread synchronization
    std::mutex management_mutex_;
    std::condition_variable management_cv_;
};

/**
 * @brief World manager for handling multiple dimensions
 */
class WorldManager {
public:
    /**
     * @brief Initialize world manager with base configuration
     */
    explicit WorldManager(const std::filesystem::path& worlds_directory);
    
    /**
     * @brief Get or create world for dimension
     */
    std::shared_ptr<World> get_world(DimensionType dimension, const std::string& world_name = "world");
    
    /**
     * @brief Create new world with specific configuration
     */
    std::shared_ptr<World> create_world(const WorldConfig& config);
    
    /**
     * @brief Save all worlds
     */
    void save_all_worlds();
    
    /**
     * @brief Shutdown all worlds and stop threads
     */
    void shutdown();
    
    /**
     * @brief Get world statistics across all dimensions
     */
    WorldStats get_aggregate_stats() const;

private:
    std::filesystem::path worlds_directory_;
    std::unordered_map<DimensionType, std::shared_ptr<World>> worlds_;
    mutable std::shared_mutex worlds_mutex_;
};

} // namespace world
} // namespace parallelstone