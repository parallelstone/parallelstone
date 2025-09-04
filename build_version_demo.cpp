#include "world/block_registry.hpp"
#include <iostream>
#include <iomanip>

/**
 * @brief Demonstration of compile-time Minecraft version selection
 * 
 * This program shows how ParallelStone's block system adapts to different
 * Minecraft versions at compile time without runtime overhead.
 */

using namespace parallelstone::world;

int main() {
    std::cout << "=== ParallelStone Compile-Time Version Demo ===\n\n";
    
    // Display current version info
    std::cout << "Compiled for Minecraft version: " << BlockRegistry::get_version_string() << "\n";
    std::cout << "Protocol version: " << BlockRegistry::get_version() << "\n";
    std::cout << "Available blocks: " << BlockRegistry::get_block_count() << "\n\n";
    
    // Test version-specific blocks
    std::cout << "=== Version-Specific Block Availability ===\n";
    
    struct TestBlock {
        BlockType type;
        const char* name;
        const char* introduced;
    };
    
    TestBlock test_blocks[] = {
        {BlockType::STONE, "Stone", "Classic"},
        {BlockType::DEEPSLATE, "Deepslate", "1.17+"},
        {BlockType::CHERRY_PLANKS, "Cherry Planks", "1.20+"},
        {BlockType::CRAFTER, "Crafter", "1.21+"},
        {BlockType::TRIAL_SPAWNER, "Trial Spawner", "1.21+"},
        {BlockType::HEAVY_CORE, "Heavy Core", "1.21+"}
    };
    
    for (const auto& test : test_blocks) {
        bool available = BlockRegistry::is_available(test.type);
        std::cout << std::setw(20) << test.name << " (" << test.introduced << "): ";
        std::cout << (available ? "✓ Available" : "✗ Not available") << "\n";
        
        if (available) {
            auto props = BlockRegistry::get_properties(test.type);
            std::cout << std::setw(35) << " └─ Hardness: " << props.hardness 
                      << ", Light: " << static_cast<int>(props.light_emission) << "\n";
        }
    }
    
    std::cout << "\n=== Block Categories ===\n";
    
    // Test utility functions
    std::cout << "Block categorization:\n";
    std::cout << "  Logs: " << (block_utils::is_log(BlockType::OAK_LOG) ? "✓" : "✗") << "\n";
    std::cout << "  Ores: " << (block_utils::is_ore(BlockType::DIAMOND_ORE) ? "✓" : "✗") << "\n";
    std::cout << "  Liquids: " << (block_utils::is_liquid(BlockType::WATER) ? "✓" : "✗") << "\n";
    std::cout << "  Air: " << (block_utils::is_air(BlockType::AIR) ? "✓" : "✗") << "\n";
    
    if (BlockRegistry::is_available(BlockType::COPPER_BLOCK)) {
        std::cout << "  Copper: " << (block_utils::is_copper(BlockType::COPPER_BLOCK) ? "✓" : "✗") << "\n";
    }
    
    std::cout << "\n=== Protocol Integration ===\n";
    
    // Test protocol ID conversion
    auto test_block = BlockType::STONE;
    auto protocol_id = BlockRegistry::get_protocol_id(test_block);
    auto converted_back = BlockRegistry::from_protocol_id(protocol_id);
    
    std::cout << "Protocol ID conversion test:\n";
    std::cout << "  Original: " << BlockRegistry::get_name(test_block) << "\n";
    std::cout << "  Protocol ID: " << protocol_id << "\n";
    std::cout << "  Converted back: " << BlockRegistry::get_name(converted_back) << "\n";
    std::cout << "  Success: " << (test_block == converted_back ? "✓" : "✗") << "\n";
    
    std::cout << "\n=== Compile Instructions ===\n";
    std::cout << "To build for different Minecraft versions:\n\n";
    std::cout << "# For Minecraft 1.20.1:\n";
    std::cout << "cmake -B build -DMINECRAFT_VERSION=120100 .\n";
    std::cout << "cmake --build build --config Release\n\n";
    std::cout << "# For Minecraft 1.21.7 (default):\n";
    std::cout << "cmake -B build -DMINECRAFT_VERSION=121700 .\n";
    std::cout << "cmake --build build --config Release\n\n";
    std::cout << "# For Minecraft 1.21.3:\n";
    std::cout << "cmake -B build -DMINECRAFT_VERSION=121300 .\n";
    std::cout << "cmake --build build --config Release\n\n";
    
    return 0;
}