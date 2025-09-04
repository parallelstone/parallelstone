#pragma once

#include "world.hpp"
#include "world_ecs_integration.hpp"
#include <memory>
#include <chrono>
#include <atomic>
#include <thread>
#include <queue>
#include <mutex>

namespace parallelstone {
namespace world {

/**
 * @brief Performance monitoring and optimization for world systems
 */
class WorldPerformanceMonitor {
public:
    struct PerformanceMetrics {
        // Timing metrics (microseconds)
        std::atomic<std::uint64_t> chunk_generation_time{0};
        std::atomic<std::uint64_t> chunk_loading_time{0};
        std::atomic<std::uint64_t> lighting_calculation_time{0};
        std::atomic<std::uint64_t> block_update_time{0};
        std::atomic<std::uint64_t> entity_update_time{0};
        std::atomic<std::uint64_t> network_sync_time{0};
        
        // Throughput metrics
        std::atomic<std::uint64_t> chunks_generated_per_second{0};
        std::atomic<std::uint64_t> blocks_updated_per_second{0};
        std::atomic<std::uint64_t> entities_processed_per_second{0};
        
        // Memory metrics
        std::atomic<std::uint64_t> memory_usage_bytes{0};
        std::atomic<std::uint32_t> loaded_chunks{0};
        std::atomic<std::uint32_t> active_entities{0};
        
        // Quality metrics
        std::atomic<float> average_fps{60.0f};
        std::atomic<float> tick_time_ms{50.0f};
        std::atomic<std::uint32_t> dropped_frames{0};
    };
    
    /**
     * @brief Initialize performance monitor
     */
    explicit WorldPerformanceMonitor(std::shared_ptr<WorldECSIntegration> world_integration);
    
    /**
     * @brief Start performance monitoring
     */
    void start_monitoring();
    
    /**
     * @brief Stop performance monitoring
     */
    void stop_monitoring();
    
    /**
     * @brief Get current performance metrics
     */
    const PerformanceMetrics& get_metrics() const { return metrics_; }
    
    /**
     * @brief Record operation timing
     */
    void record_timing(const std::string& operation, std::uint64_t duration_us);
    
    /**
     * @brief Record throughput metric
     */
    void record_throughput(const std::string& metric, std::uint64_t count);
    
    /**
     * @brief Get performance report
     */
    std::string generate_performance_report() const;
    
    /**
     * @brief Get optimization recommendations
     */
    std::vector<std::string> get_optimization_recommendations() const;

private:
    void monitoring_thread();
    void update_metrics();
    
    std::shared_ptr<WorldECSIntegration> world_integration_;
    PerformanceMetrics metrics_;
    
    std::atomic<bool> monitoring_active_{false};
    std::unique_ptr<std::thread> monitoring_thread_;
    
    // Timing history for moving averages
    std::mutex timing_mutex_;
    std::unordered_map<std::string, std::queue<std::uint64_t>> timing_history_;
};

/**
 * @brief Automatic performance optimization system
 */
class WorldPerformanceOptimizer {
public:
    enum class OptimizationLevel {
        CONSERVATIVE = 0,  // Minimal optimizations
        BALANCED = 1,      // Balanced performance/quality
        AGGRESSIVE = 2     // Maximum performance
    };
    
    struct OptimizationSettings {
        OptimizationLevel level = OptimizationLevel::BALANCED;
        
        // Chunk management settings
        std::uint32_t max_chunks_per_tick = 4;
        std::uint32_t chunk_generation_batch_size = 2;
        bool enable_chunk_compression = true;
        bool enable_sparse_chunk_storage = true;
        
        // Lighting settings
        bool enable_light_batching = true;
        std::uint32_t max_light_updates_per_tick = 64;
        bool enable_light_caching = true;
        
        // Entity settings
        std::uint32_t max_entities_per_chunk = 200;
        bool enable_entity_culling = true;
        double entity_culling_distance = 128.0;
        
        // Network settings
        std::uint32_t network_batch_size = 16;
        bool enable_chunk_delta_compression = true;
        bool enable_entity_interpolation = true;
        
        // Memory settings
        std::uint64_t max_memory_usage_mb = 2048;
        bool enable_memory_pooling = true;
        bool enable_garbage_collection = true;
    };
    
    /**
     * @brief Initialize optimizer
     */
    explicit WorldPerformanceOptimizer(std::shared_ptr<WorldECSIntegration> world_integration,
                                      std::shared_ptr<WorldPerformanceMonitor> monitor);
    
    /**
     * @brief Apply optimization settings
     */
    void apply_optimizations(const OptimizationSettings& settings);
    
    /**
     * @brief Auto-optimize based on current performance
     */
    void auto_optimize();
    
    /**
     * @brief Get current optimization settings
     */
    const OptimizationSettings& get_settings() const { return settings_; }
    
    // ==================== SPECIFIC OPTIMIZATIONS ====================
    
    /**
     * @brief Optimize chunk loading/unloading
     */
    void optimize_chunk_management();
    
    /**
     * @brief Optimize lighting calculations
     */
    void optimize_lighting_system();
    
    /**
     * @brief Optimize entity processing
     */
    void optimize_entity_systems();
    
    /**
     * @brief Optimize network synchronization
     */
    void optimize_network_sync();
    
    /**
     * @brief Optimize memory usage
     */
    void optimize_memory_usage();

private:
    std::shared_ptr<WorldECSIntegration> world_integration_;
    std::shared_ptr<WorldPerformanceMonitor> monitor_;
    OptimizationSettings settings_;
    
    // Optimization state
    std::chrono::steady_clock::time_point last_optimization_;
    std::uint32_t optimization_cycle_ = 0;
};

/**
 * @brief Chunk loading optimization with predictive algorithms
 */
class ChunkLoadingOptimizer {
public:
    /**
     * @brief Initialize chunk loading optimizer
     */
    explicit ChunkLoadingOptimizer(std::shared_ptr<World> world);
    
    /**
     * @brief Predict chunks that will be needed
     */
    std::vector<ChunkCoord> predict_chunk_needs(const std::vector<ecs::Entity>& players,
                                               const ecs::Registry& registry) const;
    
    /**
     * @brief Preload chunks based on player movement patterns
     */
    void preload_chunks(const std::vector<ChunkCoord>& chunks);
    
    /**
     * @brief Optimize chunk priority based on player proximity
     */
    void update_chunk_priorities(const std::vector<ecs::Entity>& players,
                               const ecs::Registry& registry);
    
    /**
     * @brief Enable/disable adaptive loading
     */
    void set_adaptive_loading(bool enabled) { adaptive_loading_enabled_ = enabled; }

private:
    struct ChunkPriority {
        ChunkCoord coord;
        float priority;
        std::chrono::steady_clock::time_point last_accessed;
    };
    
    std::shared_ptr<World> world_;
    bool adaptive_loading_enabled_ = true;
    
    std::vector<ChunkPriority> chunk_priorities_;
    std::mutex priority_mutex_;
    
    // Player movement tracking for prediction
    std::unordered_map<ecs::Entity, std::vector<std::pair<ChunkCoord, std::chrono::steady_clock::time_point>>> movement_history_;
};

/**
 * @brief Memory pool for efficient chunk and entity allocation
 */
class WorldMemoryPool {
public:
    /**
     * @brief Initialize memory pool
     */
    explicit WorldMemoryPool(std::size_t initial_size_mb = 512);
    
    /**
     * @brief Allocate chunk memory
     */
    std::unique_ptr<Chunk> allocate_chunk(std::int32_t chunk_x, std::int32_t chunk_z);
    
    /**
     * @brief Deallocate chunk memory
     */
    void deallocate_chunk(std::unique_ptr<Chunk> chunk);
    
    /**
     * @brief Allocate chunk section memory
     */
    std::unique_ptr<ChunkSection> allocate_chunk_section();
    
    /**
     * @brief Deallocate chunk section memory
     */
    void deallocate_chunk_section(std::unique_ptr<ChunkSection> section);
    
    /**
     * @brief Get memory usage statistics
     */
    struct MemoryStats {
        std::size_t total_allocated_bytes;
        std::size_t chunks_allocated;
        std::size_t sections_allocated;
        std::size_t pool_efficiency_percent;
    };
    
    MemoryStats get_memory_stats() const;
    
    /**
     * @brief Compact memory pool
     */
    void compact();

private:
    std::size_t pool_size_bytes_;
    std::unique_ptr<std::uint8_t[]> memory_pool_;
    
    std::mutex allocation_mutex_;
    std::vector<void*> free_chunks_;
    std::vector<void*> free_sections_;
    
    // Statistics
    mutable std::atomic<std::size_t> allocated_bytes_{0};
    mutable std::atomic<std::size_t> allocated_chunks_{0};
    mutable std::atomic<std::size_t> allocated_sections_{0};
};

/**
 * @brief Batch processing system for world operations
 */
class WorldBatchProcessor {
public:
    /**
     * @brief Initialize batch processor
     */
    explicit WorldBatchProcessor(std::shared_ptr<WorldECSIntegration> world_integration);
    
    /**
     * @brief Queue block update for batching
     */
    void queue_block_update(std::int32_t x, std::int32_t y, std::int32_t z, BlockType block_type);
    
    /**
     * @brief Queue lighting update for batching
     */
    void queue_lighting_update(std::int32_t x, std::int32_t y, std::int32_t z);
    
    /**
     * @brief Queue entity update for batching
     */
    void queue_entity_update(ecs::Entity entity);
    
    /**
     * @brief Process all queued operations
     */
    void process_batches();
    
    /**
     * @brief Set batch processing parameters
     */
    void set_batch_size(std::uint32_t block_batch_size, std::uint32_t lighting_batch_size);
    
    /**
     * @brief Enable/disable automatic batch processing
     */
    void set_auto_processing(bool enabled, std::uint32_t interval_ms = 50);

private:
    struct BlockUpdate {
        std::int32_t x, y, z;
        BlockType block_type;
    };
    
    struct LightingUpdate {
        std::int32_t x, y, z;
    };
    
    std::shared_ptr<WorldECSIntegration> world_integration_;
    
    std::mutex batch_mutex_;
    std::vector<BlockUpdate> queued_block_updates_;
    std::vector<LightingUpdate> queued_lighting_updates_;
    std::vector<ecs::Entity> queued_entity_updates_;
    
    std::uint32_t block_batch_size_ = 64;
    std::uint32_t lighting_batch_size_ = 32;
    
    // Auto-processing
    std::atomic<bool> auto_processing_enabled_{false};
    std::unique_ptr<std::thread> processing_thread_;
    
    void processing_thread_func();
};

/**
 * @brief Performance profiler for detailed analysis
 */
class WorldProfiler {
public:
    struct ProfileData {
        std::string operation_name;
        std::uint64_t total_time_us;
        std::uint64_t call_count;
        std::uint64_t min_time_us;
        std::uint64_t max_time_us;
        std::uint64_t avg_time_us;
    };
    
    /**
     * @brief Start profiling session
     */
    void start_profiling();
    
    /**
     * @brief Stop profiling session
     */
    void stop_profiling();
    
    /**
     * @brief Profile operation timing
     */
    class ScopedTimer {
    public:
        ScopedTimer(WorldProfiler* profiler, const std::string& operation_name);
        ~ScopedTimer();
        
    private:
        WorldProfiler* profiler_;
        std::string operation_name_;
        std::chrono::high_resolution_clock::time_point start_time_;
    };
    
    /**
     * @brief Get profiling results
     */
    std::vector<ProfileData> get_profile_data() const;
    
    /**
     * @brief Generate profiling report
     */
    std::string generate_profile_report() const;
    
    /**
     * @brief Clear profiling data
     */
    void clear_profile_data();

private:
    friend class ScopedTimer;
    
    void record_timing(const std::string& operation, std::uint64_t duration_us);
    
    std::atomic<bool> profiling_active_{false};
    mutable std::mutex profile_mutex_;
    std::unordered_map<std::string, ProfileData> profile_data_;
};

// Convenience macro for profiling
#define PROFILE_SCOPE(profiler, operation) \
    WorldProfiler::ScopedTimer _timer(profiler, operation)

} // namespace world
} // namespace parallelstone