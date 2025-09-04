#include "world/chunk_section.hpp"
#include "world/block_state.hpp"
#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace parallelstone {
namespace world {

ChunkSection::ChunkSection() noexcept = default;

BlockState ChunkSection::get_block(std::uint8_t x, std::uint8_t y, std::uint8_t z) const noexcept {
    if (!is_valid_coord(x) || !is_valid_coord(y) || !is_valid_coord(z)) {
        return BlockState::default_state(BlockType::AIR);
    }
    
    if (!blocks_allocated_) {
        return BlockState::default_state(BlockType::AIR);
    }
    
    std::size_t index = block_index(x, y, z);
    std::uint32_t protocol_id = (*blocks_)[index];
    
    auto state = BlockStateRegistry::from_protocol_id(protocol_id);
    return state.value_or(BlockState::default_state(BlockType::AIR));
}

void ChunkSection::set_block(std::uint8_t x, std::uint8_t y, std::uint8_t z, const BlockState& state) {
    if (!is_valid_coord(x) || !is_valid_coord(y) || !is_valid_coord(z)) {
        return;
    }
    
    BlockState old_state = get_block(x, y, z);
    
    // Only allocate storage if we're setting a non-air block
    if (state.type() != BlockType::AIR) {
        ensure_blocks_allocated();
    } else if (!blocks_allocated_) {
        // Setting air on an unallocated section - nothing to do
        return;
    }
    
    if (blocks_allocated_) {
        std::size_t index = block_index(x, y, z);
        (*blocks_)[index] = state.get_protocol_id();
        
        update_non_air_count(old_state, state);
        
        // If section becomes empty, we could deallocate, but for simplicity we keep it allocated
        // This avoids potential performance issues with frequent alloc/dealloc
    }
}

std::uint8_t ChunkSection::get_block_light(std::uint8_t x, std::uint8_t y, std::uint8_t z) const noexcept {
    if (!is_valid_coord(x) || !is_valid_coord(y) || !is_valid_coord(z)) {
        return 0;
    }
    
    if (!lighting_allocated_) {
        return 0; // Default block light is 0
    }
    
    std::size_t index = light_index(x, y, z);
    return get_nibble(block_light_->data(), index);
}

void ChunkSection::set_block_light(std::uint8_t x, std::uint8_t y, std::uint8_t z, std::uint8_t level) {
    if (!is_valid_coord(x) || !is_valid_coord(y) || !is_valid_coord(z) || level > 15) {
        return;
    }
    
    if (level == 0 && !lighting_allocated_) {
        return; // Setting default value on unallocated section
    }
    
    ensure_lighting_allocated();
    std::size_t index = light_index(x, y, z);
    set_nibble(block_light_->data(), index, level);
}

std::uint8_t ChunkSection::get_sky_light(std::uint8_t x, std::uint8_t y, std::uint8_t z) const noexcept {
    if (!is_valid_coord(x) || !is_valid_coord(y) || !is_valid_coord(z)) {
        return 15;
    }
    
    if (!lighting_allocated_) {
        return 15; // Default sky light is 15 (full daylight)
    }
    
    std::size_t index = light_index(x, y, z);
    return get_nibble(sky_light_->data(), index);
}

void ChunkSection::set_sky_light(std::uint8_t x, std::uint8_t y, std::uint8_t z, std::uint8_t level) {
    if (!is_valid_coord(x) || !is_valid_coord(y) || !is_valid_coord(z) || level > 15) {
        return;
    }
    
    if (level == 15 && !lighting_allocated_) {
        return; // Setting default value on unallocated section
    }
    
    ensure_lighting_allocated();
    std::size_t index = light_index(x, y, z);
    set_nibble(sky_light_->data(), index, level);
}

void ChunkSection::fill(const BlockState& state) {
    if (state.type() == BlockType::AIR) {
        clear();
        return;
    }
    
    ensure_blocks_allocated();
    std::uint32_t protocol_id = state.get_protocol_id();
    std::fill(blocks_->begin(), blocks_->end(), protocol_id);
    
    non_air_count_.store(BLOCK_COUNT, std::memory_order_relaxed);
}

void ChunkSection::clear() {
    if (blocks_allocated_) {
        // Fill with air (protocol ID 0)
        std::fill(blocks_->begin(), blocks_->end(), 0);
    }
    
    non_air_count_.store(0, std::memory_order_relaxed);
}

const std::uint32_t* ChunkSection::get_block_data() const noexcept {
    return blocks_allocated_ ? blocks_->data() : nullptr;
}

const std::uint8_t* ChunkSection::get_block_light_data() const noexcept {
    return lighting_allocated_ ? block_light_->data() : nullptr;
}

const std::uint8_t* ChunkSection::get_sky_light_data() const noexcept {
    return lighting_allocated_ ? sky_light_->data() : nullptr;
}

void ChunkSection::set_block_data(const std::uint32_t* data, std::size_t count) {
    if (!data || count != BLOCK_COUNT) {
        return;
    }
    
    ensure_blocks_allocated();
    std::memcpy(blocks_->data(), data, BLOCK_COUNT * sizeof(std::uint32_t));
    
    // Recalculate non-air count
    std::uint16_t count_non_air = 0;
    for (std::size_t i = 0; i < BLOCK_COUNT; ++i) {
        if ((*blocks_)[i] != 0) { // 0 is air
            ++count_non_air;
        }
    }
    non_air_count_.store(count_non_air, std::memory_order_relaxed);
}

void ChunkSection::set_lighting_data(const std::uint8_t* block_light, const std::uint8_t* sky_light) {
    ensure_lighting_allocated();
    
    if (block_light) {
        std::memcpy(block_light_->data(), block_light, LIGHT_COUNT);
    } else {
        // Default block light is 0
        std::memset(block_light_->data(), 0, LIGHT_COUNT);
    }
    
    if (sky_light) {
        std::memcpy(sky_light_->data(), sky_light, LIGHT_COUNT);
    } else {
        // Default sky light is 15 (0xFF for each nibble pair)
        std::memset(sky_light_->data(), 0xFF, LIGHT_COUNT);
    }
}

std::size_t ChunkSection::calculate_data_size() const noexcept {
    std::size_t size = 0;
    
    if (blocks_allocated_) {
        size += BLOCK_COUNT * sizeof(std::uint32_t);
    }
    
    if (lighting_allocated_) {
        size += LIGHT_COUNT * 2; // block light + sky light
    }
    
    return size;
}

void ChunkSection::recalculate_sky_light() {
    if (!lighting_allocated_) {
        return; // No lighting data to recalculate
    }
    
    // Simple top-down sky light calculation
    // In a real implementation, this would be much more sophisticated
    for (std::uint8_t x = 0; x < SECTION_SIZE; ++x) {
        for (std::uint8_t z = 0; z < SECTION_SIZE; ++z) {
            std::uint8_t current_light = 15; // Start with full skylight
            
            for (std::uint8_t y = SECTION_SIZE - 1; y > 0; --y) {
                BlockState state = get_block(x, y, z);
                
                set_sky_light(x, y, z, current_light);
                
                // Reduce light based on block opacity
                if (!state.block_properties().is_transparent) {
                    current_light = 0;
                } else {
                    std::uint8_t filter = state.block_properties().light_filter;
                    if (current_light > filter) {
                        current_light -= filter;
                    } else {
                        current_light = 0;
                    }
                }
            }
            
            // Set bottom layer
            set_sky_light(x, 0, z, current_light);
        }
    }
    
    lighting_dirty_.store(false, std::memory_order_relaxed);
}

void ChunkSection::ensure_blocks_allocated() {
    if (!blocks_allocated_) {
        blocks_ = std::make_unique<std::array<std::uint32_t, BLOCK_COUNT>>();
        // Initialize to air (protocol ID 0)
        std::fill(blocks_->begin(), blocks_->end(), 0);
        blocks_allocated_ = true;
    }
}

void ChunkSection::ensure_lighting_allocated() {
    if (!lighting_allocated_) {
        block_light_ = std::make_unique<std::array<std::uint8_t, LIGHT_COUNT>>();
        sky_light_ = std::make_unique<std::array<std::uint8_t, LIGHT_COUNT>>();
        
        // Initialize block light to 0
        std::memset(block_light_->data(), 0, LIGHT_COUNT);
        
        // Initialize sky light to 15 (0xFF for each nibble pair)
        std::memset(sky_light_->data(), 0xFF, LIGHT_COUNT);
        
        lighting_allocated_ = true;
    }
}

void ChunkSection::update_non_air_count(const BlockState& old_state, const BlockState& new_state) {
    bool old_was_air = (old_state.type() == BlockType::AIR);
    bool new_is_air = (new_state.type() == BlockType::AIR);
    
    if (old_was_air && !new_is_air) {
        non_air_count_.fetch_add(1, std::memory_order_relaxed);
    } else if (!old_was_air && new_is_air) {
        non_air_count_.fetch_sub(1, std::memory_order_relaxed);
    }
    // If both are air or both are non-air, count doesn't change
}

std::uint8_t ChunkSection::get_nibble(const std::uint8_t* array, std::size_t index) const noexcept {
    std::size_t byte_index = index / 2;
    bool high_nibble = (index % 2) == 1;
    
    std::uint8_t byte_value = array[byte_index];
    return high_nibble ? (byte_value >> 4) & 0x0F : byte_value & 0x0F;
}

void ChunkSection::set_nibble(std::uint8_t* array, std::size_t index, std::uint8_t value) const noexcept {
    std::size_t byte_index = index / 2;
    bool high_nibble = (index % 2) == 1;
    
    value &= 0x0F; // Ensure value is 4 bits
    
    if (high_nibble) {
        array[byte_index] = (array[byte_index] & 0x0F) | (value << 4);
    } else {
        array[byte_index] = (array[byte_index] & 0xF0) | value;
    }
}

// Chunk implementation
Chunk::Chunk(std::int32_t chunk_x, std::int32_t chunk_z) noexcept 
    : chunk_x_(chunk_x), chunk_z_(chunk_z) {
    // Initialize heightmap to minimum height
    std::fill(heightmap_.begin(), heightmap_.end(), MIN_SECTION_Y * ChunkSection::SECTION_SIZE);
}

BlockState Chunk::get_block(std::uint8_t x, std::int32_t y, std::uint8_t z) const {
    if (x >= CHUNK_WIDTH || z >= CHUNK_WIDTH || !is_valid_y(y)) {
        return BlockState::default_state(BlockType::AIR);
    }
    
    std::size_t section_index = y_to_section_index(y);
    std::uint8_t section_y = y_to_section_y(y);
    
    ChunkSection* section = get_section(section_index);
    if (!section) {
        return BlockState::default_state(BlockType::AIR);
    }
    
    return section->get_block(x, section_y, z);
}

void Chunk::set_block(std::uint8_t x, std::int32_t y, std::uint8_t z, const BlockState& state) {
    if (x >= CHUNK_WIDTH || z >= CHUNK_WIDTH || !is_valid_y(y)) {
        return;
    }
    
    std::size_t section_index = y_to_section_index(y);
    std::uint8_t section_y = y_to_section_y(y);
    
    ChunkSection& section = get_or_create_section(section_index);
    section.set_block(x, section_y, z, state);
    
    // Update heightmap if necessary
    update_heightmap(x, z);
}

ChunkSection* Chunk::get_section(std::size_t section_index) const noexcept {
    if (section_index >= SECTIONS_COUNT) {
        return nullptr;
    }
    
    return sections_[section_index].get();
}

ChunkSection& Chunk::get_or_create_section(std::size_t section_index) {
    if (section_index >= SECTIONS_COUNT) {
        throw std::out_of_range("Section index out of range");
    }
    
    if (!sections_[section_index]) {
        sections_[section_index] = std::make_unique<ChunkSection>();
    }
    
    return *sections_[section_index];
}

bool Chunk::is_empty() const noexcept {
    for (const auto& section : sections_) {
        if (section && !section->is_empty()) {
            return false;
        }
    }
    return true;
}

void Chunk::recalculate_lighting() {
    // Mark all sections as needing lighting recalculation
    for (auto& section : sections_) {
        if (section) {
            section->mark_lighting_dirty();
        }
    }
    
    // Recalculate from top to bottom
    for (std::size_t i = SECTIONS_COUNT; i > 0; --i) {
        std::size_t section_index = i - 1;
        if (sections_[section_index]) {
            sections_[section_index]->recalculate_sky_light();
        }
    }
}

std::int32_t Chunk::get_height(std::uint8_t x, std::uint8_t z) const noexcept {
    if (x >= CHUNK_WIDTH || z >= CHUNK_WIDTH) {
        return MIN_SECTION_Y * ChunkSection::SECTION_SIZE;
    }
    
    std::size_t index = static_cast<std::size_t>(z) * CHUNK_WIDTH + static_cast<std::size_t>(x);
    return heightmap_[index];
}

void Chunk::update_heightmap(std::uint8_t x, std::uint8_t z) {
    if (x >= CHUNK_WIDTH || z >= CHUNK_WIDTH) {
        return;
    }
    
    // Find the highest non-air block
    std::int32_t height = MIN_SECTION_Y * ChunkSection::SECTION_SIZE;
    
    for (std::int32_t y = (MIN_SECTION_Y + static_cast<std::int32_t>(SECTIONS_COUNT)) * ChunkSection::SECTION_SIZE - 1; 
         y >= MIN_SECTION_Y * ChunkSection::SECTION_SIZE; --y) {
        BlockState state = get_block(x, y, z);
        if (state.type() != BlockType::AIR) {
            height = y;
            break;
        }
    }
    
    std::size_t index = static_cast<std::size_t>(z) * CHUNK_WIDTH + static_cast<std::size_t>(x);
    heightmap_[index] = height;
}

} // namespace world
} // namespace parallelstone