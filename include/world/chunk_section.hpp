#pragma once

#include "compile_time_blocks.hpp"
#include <array>
#include <memory>
#include <atomic>
#include <cstdint>
#include <unordered_map>
#include <string>

namespace parallelstone {
namespace world {

/**
 * @brief A 16x16x16 section of blocks within a chunk
 * 
 * Modern C++20 implementation of Minecraft's chunk section system.
 * Features sparse allocation, efficient storage, and thread-safe access patterns.
 * 
 * Each chunk is divided into 16 sections vertically (Y=0-15, Y=16-31, etc.)
 * This allows for efficient memory usage by only allocating sections that contain non-air blocks.
 */
class ChunkSection {
public:
    static constexpr std::size_t SECTION_SIZE = 16;
    static constexpr std::size_t BLOCK_COUNT = SECTION_SIZE * SECTION_SIZE * SECTION_SIZE; // 4096 blocks
    static constexpr std::size_t LIGHT_COUNT = BLOCK_COUNT / 2; // 2048 nibbles (4 bits per block)
    
    /**
     * @brief Block index calculation for x,y,z coordinates
     */
    static constexpr std::size_t block_index(std::uint8_t x, std::uint8_t y, std::uint8_t z) noexcept {
        return static_cast<std::size_t>(y) * (SECTION_SIZE * SECTION_SIZE) + 
               static_cast<std::size_t>(z) * SECTION_SIZE + 
               static_cast<std::size_t>(x);
    }
    
    /**
     * @brief Light index calculation (2 blocks per byte)
     */
    static constexpr std::size_t light_index(std::uint8_t x, std::uint8_t y, std::uint8_t z) noexcept {
        return block_index(x, y, z) / 2;
    }
    
    /**
     * @brief Check if coordinate is valid within section
     */
    static constexpr bool is_valid_coord(std::uint8_t coord) noexcept {
        return coord < SECTION_SIZE;
    }
    
    /**
     * @brief Default constructor creates empty section
     */
    ChunkSection() noexcept;
    
    /**
     * @brief Destructor
     */
    ~ChunkSection() noexcept = default;
    
    // Non-copyable but movable
    ChunkSection(const ChunkSection&) = delete;
    ChunkSection& operator=(const ChunkSection&) = delete;
    ChunkSection(ChunkSection&&) noexcept = default;
    ChunkSection& operator=(ChunkSection&&) noexcept = default;
    
    /**
     * @brief Get block state at coordinates
     */
    BlockState get_block(std::uint8_t x, std::uint8_t y, std::uint8_t z) const noexcept;
    
    /**
     * @brief Set block state at coordinates
     */
    void set_block(std::uint8_t x, std::uint8_t y, std::uint8_t z, const BlockState& state);
    
    /**
     * @brief Get block light level (0-15)
     */
    std::uint8_t get_block_light(std::uint8_t x, std::uint8_t y, std::uint8_t z) const noexcept;
    
    /**
     * @brief Set block light level (0-15)
     */
    void set_block_light(std::uint8_t x, std::uint8_t y, std::uint8_t z, std::uint8_t level);
    
    /**
     * @brief Get sky light level (0-15)
     */
    std::uint8_t get_sky_light(std::uint8_t x, std::uint8_t y, std::uint8_t z) const noexcept;
    
    /**
     * @brief Set sky light level (0-15)
     */
    void set_sky_light(std::uint8_t x, std::uint8_t y, std::uint8_t z, std::uint8_t level);
    
    /**
     * @brief Check if section is empty (all air blocks)
     */
    bool is_empty() const noexcept {
        return !blocks_allocated_;
    }
    
    /**
     * @brief Check if section has lighting data
     */
    bool has_lighting() const noexcept {
        return lighting_allocated_;
    }
    
    /**
     * @brief Get count of non-air blocks
     */
    std::uint16_t non_air_count() const noexcept {
        return non_air_count_.load(std::memory_order_relaxed);
    }
    
    /**
     * @brief Fill entire section with a single block state
     */
    void fill(const BlockState& state);
    
    /**
     * @brief Clear section to all air blocks
     */
    void clear();
    
    /**
     * @brief Get raw block data for network serialization
     */
    const std::uint32_t* get_block_data() const noexcept;
    
    /**
     * @brief Get raw block light data
     */
    const std::uint8_t* get_block_light_data() const noexcept;
    
    /**
     * @brief Get raw sky light data
     */
    const std::uint8_t* get_sky_light_data() const noexcept;
    
    /**
     * @brief Set raw block data from network
     */
    void set_block_data(const std::uint32_t* data, std::size_t count);
    
    /**
     * @brief Set raw lighting data
     */
    void set_lighting_data(const std::uint8_t* block_light, const std::uint8_t* sky_light);
    
    /**
     * @brief Calculate data size for network transmission
     */
    std::size_t calculate_data_size() const noexcept;
    
    /**
     * @brief Recalculate sky lighting for this section
     */
    void recalculate_sky_light();
    
    /**
     * @brief Mark lighting as needing recalculation
     */
    void mark_lighting_dirty() noexcept {
        lighting_dirty_.store(true, std::memory_order_relaxed);
    }
    
    /**
     * @brief Check if lighting needs recalculation
     */
    bool is_lighting_dirty() const noexcept {
        return lighting_dirty_.load(std::memory_order_relaxed);
    }

private:
    /**
     * @brief Ensure blocks array is allocated
     */
    void ensure_blocks_allocated();
    
    /**
     * @brief Ensure lighting arrays are allocated
     */
    void ensure_lighting_allocated();
    
    /**
     * @brief Update non-air count when block changes
     */
    void update_non_air_count(const BlockState& old_state, const BlockState& new_state);
    
    /**
     * @brief Get or set nibble value in lighting array
     */
    std::uint8_t get_nibble(const std::uint8_t* array, std::size_t index) const noexcept;
    void set_nibble(std::uint8_t* array, std::size_t index, std::uint8_t value) const noexcept;
    
    // Block storage (sparse allocation)
    std::unique_ptr<std::array<std::uint32_t, BLOCK_COUNT>> blocks_;
    bool blocks_allocated_ = false;
    
    // Lighting storage (sparse allocation)
    std::unique_ptr<std::array<std::uint8_t, LIGHT_COUNT>> block_light_;
    std::unique_ptr<std::array<std::uint8_t, LIGHT_COUNT>> sky_light_;
    bool lighting_allocated_ = false;
    
    // Metadata
    std::atomic<std::uint16_t> non_air_count_{0};
    std::atomic<bool> lighting_dirty_{false};
};

/**
 * @brief A complete chunk containing 16 sections vertically
 */
class Chunk {
public:
    static constexpr std::size_t CHUNK_WIDTH = 16;
    static constexpr std::size_t CHUNK_HEIGHT = 384; // 1.18+ extended height
    static constexpr std::size_t SECTIONS_COUNT = CHUNK_HEIGHT / ChunkSection::SECTION_SIZE; // 24 sections
    static constexpr std::int32_t MIN_SECTION_Y = -4; // Y=-64 to Y=319
    
    /**
     * @brief Constructor for chunk at specified coordinates
     */
    Chunk(std::int32_t chunk_x, std::int32_t chunk_z) noexcept;
    
    /**
     * @brief Get block state at world coordinates
     */
    BlockState get_block(std::uint8_t x, std::int32_t y, std::uint8_t z) const;
    
    /**
     * @brief Set block state at world coordinates
     */
    void set_block(std::uint8_t x, std::int32_t y, std::uint8_t z, const BlockState& state);
    
    /**
     * @brief Get chunk coordinates
     */
    std::int32_t chunk_x() const noexcept { return chunk_x_; }
    std::int32_t chunk_z() const noexcept { return chunk_z_; }
    
    /**
     * @brief Convert world Y coordinate to section index
     */
    static constexpr std::size_t y_to_section_index(std::int32_t y) noexcept {
        return static_cast<std::size_t>((y / ChunkSection::SECTION_SIZE) - MIN_SECTION_Y);
    }
    
    /**
     * @brief Convert world Y coordinate to section-relative Y
     */
    static constexpr std::uint8_t y_to_section_y(std::int32_t y) noexcept {
        return static_cast<std::uint8_t>(y & 15); // y % 16
    }
    
    /**
     * @brief Check if Y coordinate is valid
     */
    static constexpr bool is_valid_y(std::int32_t y) noexcept {
        return y >= (MIN_SECTION_Y * ChunkSection::SECTION_SIZE) && 
               y < ((MIN_SECTION_Y + static_cast<std::int32_t>(SECTIONS_COUNT)) * ChunkSection::SECTION_SIZE);
    }
    
    /**
     * @brief Get section at index (may be null for empty sections)
     */
    ChunkSection* get_section(std::size_t section_index) const noexcept;
    
    /**
     * @brief Get or create section at index
     */
    ChunkSection& get_or_create_section(std::size_t section_index);
    
    /**
     * @brief Check if chunk is empty
     */
    bool is_empty() const noexcept;
    
    /**
     * @brief Recalculate all lighting in chunk
     */
    void recalculate_lighting();
    
    /**
     * @brief Get heightmap value at x,z
     */
    std::int32_t get_height(std::uint8_t x, std::uint8_t z) const noexcept;
    
    /**
     * @brief Update heightmap after block change
     */
    void update_heightmap(std::uint8_t x, std::uint8_t z);
    
private:
    std::int32_t chunk_x_;
    std::int32_t chunk_z_;
    
    // Sparse section storage
    std::array<std::unique_ptr<ChunkSection>, SECTIONS_COUNT> sections_;
    
    // Heightmap for surface detection
    std::array<std::int32_t, CHUNK_WIDTH * CHUNK_WIDTH> heightmap_;
};

} // namespace world
} // namespace parallelstone