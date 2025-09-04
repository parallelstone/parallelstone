#include "world/biome_system.hpp"
#include "world/chunk_section.hpp"
#include <algorithm>
#include <cmath>
#include <spdlog/spdlog.h>

namespace parallelstone {
namespace world {

// ==================== BIOME TRANSITION SYSTEM IMPLEMENTATION ====================

BiomeTransitionSystem::BiomeTransitionSystem(std::shared_ptr<BiomeGenerator> biome_gen) 
    : biome_generator_(biome_gen) {
    initialize_transition_rules();
    spdlog::info("Initialized biome transition system with {} transition rules", transition_rules_.size());
}

void BiomeTransitionSystem::initialize_transition_rules() {
    // Define biome transition rules based on Minecraft patterns
    
    // Ocean transitions
    transition_rules_.push_back({
        BiomeType::OCEAN, BiomeType::PLAINS, BiomeType::BEACH, 8.0f, 0.8f
    });
    transition_rules_.push_back({
        BiomeType::OCEAN, BiomeType::DESERT, BiomeType::BEACH, 6.0f, 0.9f  
    });
    transition_rules_.push_back({
        BiomeType::OCEAN, BiomeType::FOREST, BiomeType::BEACH, 8.0f, 0.8f
    });
    transition_rules_.push_back({
        BiomeType::OCEAN, BiomeType::JUNGLE, BiomeType::BEACH, 8.0f, 0.8f
    });
    transition_rules_.push_back({
        BiomeType::OCEAN, BiomeType::TAIGA, BiomeType::BEACH, 8.0f, 0.8f
    });
    transition_rules_.push_back({
        BiomeType::FROZEN_OCEAN, BiomeType::SNOWY_PLAINS, BiomeType::SNOWY_BEACH, 8.0f, 0.8f
    });
    transition_rules_.push_back({
        BiomeType::FROZEN_OCEAN, BiomeType::SNOWY_TAIGA, BiomeType::SNOWY_BEACH, 8.0f, 0.8f
    });
    
    // Mountain transitions
    transition_rules_.push_back({
        BiomeType::PLAINS, BiomeType::MOUNTAINS, BiomeType::WINDSWEPT_HILLS, 12.0f, 0.6f
    });
    transition_rules_.push_back({
        BiomeType::FOREST, BiomeType::MOUNTAINS, BiomeType::WINDSWEPT_FOREST, 12.0f, 0.6f
    });
    transition_rules_.push_back({
        BiomeType::SAVANNA, BiomeType::MOUNTAINS, BiomeType::WINDSWEPT_SAVANNA, 12.0f, 0.6f
    });
    
    // Desert transitions
    transition_rules_.push_back({
        BiomeType::PLAINS, BiomeType::DESERT, BiomeType::SAVANNA, 16.0f, 0.5f
    });
    transition_rules_.push_back({
        BiomeType::FOREST, BiomeType::DESERT, BiomeType::SAVANNA, 20.0f, 0.4f
    });
    
    // Forest transitions
    transition_rules_.push_back({
        BiomeType::PLAINS, BiomeType::DARK_FOREST, BiomeType::FOREST, 10.0f, 0.7f
    });
    transition_rules_.push_back({
        BiomeType::BIRCH_FOREST, BiomeType::DARK_FOREST, BiomeType::FOREST, 8.0f, 0.8f
    });
    
    // Taiga transitions
    transition_rules_.push_back({
        BiomeType::PLAINS, BiomeType::TAIGA, BiomeType::FOREST, 12.0f, 0.6f
    });
    transition_rules_.push_back({
        BiomeType::FOREST, BiomeType::SNOWY_TAIGA, BiomeType::TAIGA, 10.0f, 0.7f
    });
    
    // Swamp transitions
    transition_rules_.push_back({
        BiomeType::PLAINS, BiomeType::SWAMP, BiomeType::FOREST, 8.0f, 0.8f
    });
    transition_rules_.push_back({
        BiomeType::FOREST, BiomeType::SWAMP, BiomeType::FOREST, 6.0f, 0.9f
    });
    
    // Jungle transitions  
    transition_rules_.push_back({
        BiomeType::FOREST, BiomeType::JUNGLE, BiomeType::SPARSE_JUNGLE, 10.0f, 0.7f
    });
    transition_rules_.push_back({
        BiomeType::PLAINS, BiomeType::JUNGLE, BiomeType::SPARSE_JUNGLE, 14.0f, 0.6f
    });
    
    // Snowy biome transitions
    transition_rules_.push_back({
        BiomeType::PLAINS, BiomeType::SNOWY_PLAINS, BiomeType::TAIGA, 12.0f, 0.6f
    });
    transition_rules_.push_back({
        BiomeType::TAIGA, BiomeType::SNOWY_TAIGA, BiomeType::TAIGA, 6.0f, 0.9f
    });
    
    // River transitions (rivers cut through most biomes)
    transition_rules_.push_back({
        BiomeType::PLAINS, BiomeType::RIVER, BiomeType::RIVER, 4.0f, 1.0f
    });
    transition_rules_.push_back({
        BiomeType::FOREST, BiomeType::RIVER, BiomeType::RIVER, 4.0f, 1.0f
    });
    transition_rules_.push_back({
        BiomeType::DESERT, BiomeType::RIVER, BiomeType::RIVER, 4.0f, 1.0f
    });
    transition_rules_.push_back({
        BiomeType::SNOWY_PLAINS, BiomeType::FROZEN_RIVER, BiomeType::FROZEN_RIVER, 4.0f, 1.0f
    });
    transition_rules_.push_back({
        BiomeType::SNOWY_TAIGA, BiomeType::FROZEN_RIVER, BiomeType::FROZEN_RIVER, 4.0f, 1.0f
    });
    
    spdlog::debug("Initialized {} biome transition rules", transition_rules_.size());
}

void BiomeTransitionSystem::apply_transitions(Chunk& chunk, std::int32_t chunk_x, std::int32_t chunk_z) {
    // Sample surrounding area for biome transitions
    constexpr int sample_radius = 2; // Check 2 chunks in each direction
    constexpr int sample_size = (sample_radius * 2 + 1) * 16; // Total sampling area
    
    // Create extended biome map including surrounding areas
    std::vector<std::vector<BiomeType>> extended_biome_map(sample_size, std::vector<BiomeType>(sample_size));
    
    // Sample biomes in extended area
    for (int x = 0; x < sample_size; x++) {
        for (int z = 0; z < sample_size; z++) {
            std::int32_t world_x = (chunk_x - sample_radius) * 16 + x;
            std::int32_t world_z = (chunk_z - sample_radius) * 16 + z;
            extended_biome_map[x][z] = biome_generator_->generate_biome(world_x, world_z);
        }
    }
    
    // Apply transitions to chunk blocks
    for (std::uint8_t x = 0; x < 16; x++) {
        for (std::uint8_t z = 0; z < 16; z++) {
            std::int32_t world_x = chunk_x * 16 + x;
            std::int32_t world_z = chunk_z * 16 + z;
            std::int32_t extended_x = sample_radius * 16 + x;
            std::int32_t extended_z = sample_radius * 16 + z;
            
            BiomeType current_biome = extended_biome_map[extended_x][extended_z];
            
            // Check if this position needs transition
            if (is_transition_zone(world_x, world_z)) {
                BiomeType transition_biome = determine_transition_biome(
                    extended_biome_map, extended_x, extended_z, current_biome);
                
                if (transition_biome != current_biome) {
                    apply_biome_transition_to_column(chunk, x, z, current_biome, transition_biome);
                }
            }
            
            // Apply smooth blending for terrain features
            apply_terrain_blending(chunk, x, z, extended_biome_map, extended_x, extended_z);
        }
    }
}

BiomeType BiomeTransitionSystem::determine_transition_biome(
    const std::vector<std::vector<BiomeType>>& biome_map, 
    int center_x, int center_z, BiomeType current_biome) {
    
    constexpr int check_radius = 8; // Check 8-block radius for transitions
    std::unordered_map<BiomeType, int> biome_counts;
    
    // Sample surrounding biomes
    for (int dx = -check_radius; dx <= check_radius; dx++) {
        for (int dz = -check_radius; dz <= check_radius; dz++) {
            int sample_x = center_x + dx;
            int sample_z = center_z + dz;
            
            if (sample_x >= 0 && sample_x < biome_map.size() &&
                sample_z >= 0 && sample_z < biome_map[0].size()) {
                
                BiomeType sample_biome = biome_map[sample_x][sample_z];
                biome_counts[sample_biome]++;
            }
        }
    }
    
    // Find the most common adjacent biome that's different from current
    BiomeType most_common_different = current_biome;
    int max_count = 0;
    
    for (const auto& [biome, count] : biome_counts) {
        if (biome != current_biome && count > max_count) {
            most_common_different = biome;
            max_count = count;
        }
    }
    
    // Look for applicable transition rule
    for (const auto& rule : transition_rules_) {
        if ((rule.from_biome == current_biome && rule.to_biome == most_common_different) ||
            (rule.from_biome == most_common_different && rule.to_biome == current_biome)) {
            
            float distance = std::sqrt(static_cast<float>(check_radius * check_radius));
            if (distance <= rule.min_distance) {
                return rule.transition_biome;
            }
        }
    }
    
    return current_biome; // No transition needed
}

void BiomeTransitionSystem::apply_biome_transition_to_column(Chunk& chunk, std::uint8_t x, std::uint8_t z,
                                                           BiomeType from_biome, BiomeType to_biome) {
    const BiomeData& from_data = biome_generator_->get_biome_data(from_biome);
    const BiomeData& to_data = biome_generator_->get_biome_data(to_biome);
    
    std::int32_t surface_y = chunk.get_height(x, z);
    
    // Apply transition to surface blocks
    BlockType transition_surface = blend_surface_blocks(from_data.surface_block, to_data.surface_block);
    if (transition_surface != from_data.surface_block) {
        chunk.set_block(x, surface_y, z, BlockState(transition_surface));
    }
    
    // Apply transition to subsurface blocks
    if (surface_y - 1 >= -64) {
        BlockType transition_subsurface = blend_surface_blocks(from_data.subsurface_block, to_data.subsurface_block);
        if (transition_subsurface != from_data.subsurface_block) {
            chunk.set_block(x, surface_y - 1, z, BlockState(transition_subsurface));
        }
    }
    
    // Handle special transition cases
    handle_special_transitions(chunk, x, z, surface_y, from_biome, to_biome);
}

BlockType BiomeTransitionSystem::blend_surface_blocks(BlockType block1, BlockType block2) {
    // Define block blending rules
    std::unordered_map<std::pair<BlockType, BlockType>, BlockType, PairHash> blend_rules = {
        // Grass/Sand transitions
        {{BlockType::GRASS_BLOCK, BlockType::SAND}, BlockType::COARSE_DIRT},
        {{BlockType::SAND, BlockType::GRASS_BLOCK}, BlockType::COARSE_DIRT},
        
        // Forest transitions
        {{BlockType::GRASS_BLOCK, BlockType::PODZOL}, BlockType::COARSE_DIRT},
        {{BlockType::PODZOL, BlockType::GRASS_BLOCK}, BlockType::COARSE_DIRT},
        
        // Snow transitions
        {{BlockType::GRASS_BLOCK, BlockType::SNOW_BLOCK}, BlockType::GRASS_BLOCK}, // Keep grass, add snow layer separately
        {{BlockType::SNOW_BLOCK, BlockType::GRASS_BLOCK}, BlockType::GRASS_BLOCK},
        
        // Swamp transitions
        {{BlockType::GRASS_BLOCK, BlockType::MUD}, BlockType::COARSE_DIRT},
        {{BlockType::MUD, BlockType::GRASS_BLOCK}, BlockType::COARSE_DIRT},
        
        // Desert/Savanna transitions
        {{BlockType::SAND, BlockType::COARSE_DIRT}, BlockType::COARSE_DIRT},
        {{BlockType::COARSE_DIRT, BlockType::SAND}, BlockType::COARSE_DIRT},
    };
    
    auto it = blend_rules.find({block1, block2});
    if (it != blend_rules.end()) {
        return it->second;
    }
    
    // Default: return the first block if no rule exists
    return block1;
}

void BiomeTransitionSystem::handle_special_transitions(Chunk& chunk, std::uint8_t x, std::uint8_t z, 
                                                     std::int32_t surface_y, BiomeType from_biome, BiomeType to_biome) {
    // Handle beach transitions
    if ((from_biome == BiomeType::OCEAN && to_biome == BiomeType::PLAINS) ||
        (from_biome == BiomeType::PLAINS && to_biome == BiomeType::OCEAN)) {
        
        // Create beach
        chunk.set_block(x, surface_y, z, BlockState(BlockType::SAND));
        if (surface_y - 1 >= -64) {
            chunk.set_block(x, surface_y - 1, z, BlockState(BlockType::SAND));
        }
        if (surface_y - 2 >= -64) {
            chunk.set_block(x, surface_y - 2, z, BlockState(BlockType::SANDSTONE));
        }
    }
    
    // Handle snowy transitions
    if (to_biome == BiomeType::SNOWY_PLAINS || to_biome == BiomeType::SNOWY_TAIGA ||
        to_biome == BiomeType::ICE_SPIKES) {
        
        if (surface_y + 1 < 320) {
            chunk.set_block(x, surface_y + 1, z, BlockState(BlockType::SNOW));
        }
    }
    
    // Handle river transitions
    if (to_biome == BiomeType::RIVER || to_biome == BiomeType::FROZEN_RIVER) {
        // Lower the terrain slightly for river
        if (surface_y > 55) { // Only lower if above minimum river level
            chunk.set_block(x, surface_y, z, BlockState(BlockType::AIR));
            chunk.set_block(x, surface_y - 1, z, BlockState(BlockType::GRAVEL));
            chunk.set_block(x, surface_y - 2, z, BlockState(BlockType::SAND));
            
            // Fill with water or ice
            BlockType fluid = (to_biome == BiomeType::FROZEN_RIVER) ? BlockType::ICE : BlockType::WATER;
            for (std::int32_t y = surface_y - 1; y >= std::max(-64, surface_y - 4); y--) {
                chunk.set_block(x, y, z, BlockState(fluid));
            }
            
            chunk.set_height(x, z, surface_y - 2);
        }
    }
    
    // Handle mountain transitions
    if (to_biome == BiomeType::MOUNTAINS || to_biome == BiomeType::WINDSWEPT_HILLS) {
        // Add some stone exposure
        if (surface_y > 80 && rand() % 4 == 0) {
            chunk.set_block(x, surface_y, z, BlockState(BlockType::STONE));
        }
    }
}

void BiomeTransitionSystem::apply_terrain_blending(Chunk& chunk, std::uint8_t x, std::uint8_t z,
                                                  const std::vector<std::vector<BiomeType>>& biome_map,
                                                  int center_x, int center_z) {
    BiomeType center_biome = biome_map[center_x][center_z];
    
    // Sample height variations in surrounding area
    constexpr int blend_radius = 4;
    float height_sum = 0.0f;
    int sample_count = 0;
    
    for (int dx = -blend_radius; dx <= blend_radius; dx++) {
        for (int dz = -blend_radius; dz <= blend_radius; dz++) {
            int sample_x = center_x + dx;
            int sample_z = center_z + dz;
            
            if (sample_x >= 0 && sample_x < biome_map.size() &&
                sample_z >= 0 && sample_z < biome_map[0].size()) {
                
                BiomeType sample_biome = biome_map[sample_x][sample_z];
                const BiomeData& biome_data = biome_generator_->get_biome_data(sample_biome);
                
                float distance = std::sqrt(static_cast<float>(dx * dx + dz * dz));
                float weight = std::max(0.0f, 1.0f - (distance / blend_radius));
                
                height_sum += biome_data.base_height * weight;
                sample_count++;
            }
        }
    }
    
    if (sample_count > 0) {
        float average_height = height_sum / sample_count;
        const BiomeData& center_data = biome_generator_->get_biome_data(center_biome);
        
        // If height difference is significant, apply gentle terrain blending
        float height_diff = std::abs(average_height - center_data.base_height);
        if (height_diff > 0.2f) {
            apply_height_blending(chunk, x, z, height_diff);
        }
    }
}

void BiomeTransitionSystem::apply_height_blending(Chunk& chunk, std::uint8_t x, std::uint8_t z, float blend_factor) {
    std::int32_t surface_y = chunk.get_height(x, z);
    
    // Apply gentle height adjustment (max 2 blocks)
    int height_adjustment = static_cast<int>(blend_factor * 4.0f - 2.0f);
    height_adjustment = std::clamp(height_adjustment, -2, 2);
    
    if (height_adjustment != 0) {
        std::int32_t new_surface = surface_y + height_adjustment;
        new_surface = std::clamp(new_surface, -64, 319);
        
        if (height_adjustment > 0) {
            // Add terrain
            for (std::int32_t y = surface_y + 1; y <= new_surface; y++) {
                BlockType add_block = (y == new_surface) ? BlockType::GRASS_BLOCK : BlockType::DIRT;
                chunk.set_block(x, y, z, BlockState(add_block));
            }
        } else {
            // Remove terrain
            for (std::int32_t y = new_surface + 1; y <= surface_y; y++) {
                chunk.set_block(x, y, z, BlockState(BlockType::AIR));
            }
        }
        
        chunk.set_height(x, z, new_surface);
    }
}

BiomeData BiomeTransitionSystem::get_blended_biome_data(std::int32_t x, std::int32_t z, float blend_radius) const {
    BiomeType center_biome = biome_generator_->generate_biome(x, z);
    BiomeData blended_data = biome_generator_->get_biome_data(center_biome);
    
    // Sample surrounding biomes for blending
    constexpr int sample_count = 8; // Sample 8 points around center
    float total_weight = 1.0f; // Center has weight of 1.0
    
    for (int i = 0; i < sample_count; i++) {
        float angle = (i * 2.0f * M_PI) / sample_count;
        std::int32_t sample_x = x + static_cast<std::int32_t>(std::cos(angle) * blend_radius);
        std::int32_t sample_z = z + static_cast<std::int32_t>(std::sin(angle) * blend_radius);
        
        BiomeType sample_biome = biome_generator_->generate_biome(sample_x, sample_z);
        if (sample_biome != center_biome) {
            const BiomeData& sample_data = biome_generator_->get_biome_data(sample_biome);
            
            float weight = 0.125f; // Each sample has 1/8 weight
            total_weight += weight;
            
            // Blend numerical properties
            blended_data.temperature = (blended_data.temperature * (total_weight - weight) + 
                                      sample_data.temperature * weight) / total_weight;
            blended_data.humidity = (blended_data.humidity * (total_weight - weight) + 
                                   sample_data.humidity * weight) / total_weight;
            blended_data.base_height = (blended_data.base_height * (total_weight - weight) + 
                                      sample_data.base_height * weight) / total_weight;
        }
    }
    
    return blended_data;
}

bool BiomeTransitionSystem::is_transition_zone(std::int32_t x, std::int32_t z) const {
    BiomeType center_biome = biome_generator_->generate_biome(x, z);
    
    // Check if any nearby positions have different biomes
    constexpr int check_distance = 4;
    for (int dx = -check_distance; dx <= check_distance; dx += 2) {
        for (int dz = -check_distance; dz <= check_distance; dz += 2) {
            if (dx == 0 && dz == 0) continue;
            
            BiomeType sample_biome = biome_generator_->generate_biome(x + dx, z + dz);
            if (sample_biome != center_biome) {
                return true;
            }
        }
    }
    
    return false;
}

float BiomeTransitionSystem::calculate_biome_influence(BiomeType biome, std::int32_t center_x, std::int32_t center_z,
                                                     std::int32_t sample_x, std::int32_t sample_z, float radius) const {
    float distance = std::sqrt(static_cast<float>((sample_x - center_x) * (sample_x - center_x) + 
                                                 (sample_z - center_z) * (sample_z - center_z)));
    
    if (distance > radius) {
        return 0.0f;
    }
    
    BiomeType sample_biome = biome_generator_->generate_biome(sample_x, sample_z);
    if (sample_biome != biome) {
        return 0.0f;
    }
    
    // Linear falloff with distance
    return std::max(0.0f, 1.0f - (distance / radius));
}

// Helper struct for pair hashing
struct BiomeTransitionSystem::PairHash {
    std::size_t operator()(const std::pair<BlockType, BlockType>& p) const {
        return std::hash<int>{}(static_cast<int>(p.first)) ^ 
               (std::hash<int>{}(static_cast<int>(p.second)) << 1);
    }
};

} // namespace world
} // namespace parallelstone