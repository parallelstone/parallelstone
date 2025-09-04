/**
 * @file version_aware_demo.cpp
 * @brief Demonstration of ParallelStone's version-aware block system
 * 
 * This demo shows how the server handles different Minecraft protocol versions:
 * - Dynamic block registration based on version
 * - Automatic fallback mapping for unsupported blocks
 * - Protocol ID conversion between versions
 * - Backward compatibility across Minecraft versions
 */

#include "world/version_aware_blocks.hpp"
#include "ecs/core.hpp"
#include "ecs/world_ecs.hpp"
#include <iostream>
#include <iomanip>

using namespace parallelstone::world;
using namespace parallelstone::ecs;

void demonstrate_basic_registry() {
    std::cout << "=== Basic Version-Aware Registry ===\n";
    
    auto& registry = get_block_registry();
    
    // Load all block definitions
    registry.load_definitions();
    
    std::cout << "Loaded block registry with version support\n";
    
    // Test basic block lookup
    auto stone_block = registry.get_universal_block("minecraft:stone");
    auto air_block = registry.get_universal_block("minecraft:air");
    auto cherry_log = registry.get_universal_block("minecraft:cherry_log");
    
    if (stone_block) {
        std::cout << "Stone block (ID " << stone_block->id << "): " << stone_block->display_name << "\n";
        std::cout << "  Available from version " << stone_block->availability.min_version << "\n";
    }
    
    if (air_block) {
        std::cout << "Air block (ID " << air_block->id << "): " << air_block->display_name << "\n";
    }
    
    if (cherry_log) {
        std::cout << "Cherry Log (ID " << cherry_log->id << "): " << cherry_log->display_name << "\n";
        std::cout << "  Available from version " << cherry_log->availability.min_version << "\n";
    }
    
    std::cout << "\n";
}

void demonstrate_version_compatibility() {
    std::cout << "=== Version Compatibility Demonstration ===\n";
    
    auto& registry = get_block_registry();
    
    // Test different protocol versions
    std::vector<int> test_versions = {
        ProtocolVersion::MC_1_12_2,
        ProtocolVersion::MC_1_16_5,
        ProtocolVersion::MC_1_20_1,
        ProtocolVersion::MC_1_21_7
    };
    
    for (int version : test_versions) {
        std::cout << "--- Protocol Version " << version << " ---\n";
        
        auto available_blocks = registry.get_blocks_for_version(version);
        std::cout << "Available blocks: " << available_blocks.size() << "\n";
        
        // Test specific blocks
        std::vector<std::string> test_blocks = {
            "minecraft:stone",
            "minecraft:ancient_debris",    // 1.16+
            "minecraft:deepslate",        // 1.17+  
            "minecraft:cherry_log",       // 1.20+
            "minecraft:bamboo_planks"     // 1.20+
        };
        
        for (const auto& block_name : test_blocks) {
            auto block = registry.get_universal_block(block_name);
            if (!block) {
                std::cout << "  " << block_name << ": Not registered\n";
                continue;
            }
            
            if (block->is_available_in(version)) {
                std::cout << "  " << block_name << ": Available\n";
            } else {
                auto fallback_id = block->get_fallback_for(version);
                auto fallback_block = registry.get_universal_block(fallback_id);
                std::cout << "  " << block_name << ": Not available, fallback to ";
                if (fallback_block) {
                    std::cout << fallback_block->name << "\n";
                } else {
                    std::cout << "air\n";
                }
            }
        }
        std::cout << "\n";
    }
}

void demonstrate_protocol_conversion() {
    std::cout << "=== Protocol ID Conversion ===\n";
    
    auto& registry = get_block_registry();
    
    // Load protocol mappings for different versions
    registry.load_protocol_mappings(ProtocolVersion::MC_1_12_2);
    registry.load_protocol_mappings(ProtocolVersion::MC_1_16_5);
    registry.load_protocol_mappings(ProtocolVersion::MC_1_21_7);
    
    // Test protocol conversion for different versions
    std::vector<std::string> test_blocks = {
        "minecraft:air",
        "minecraft:stone", 
        "minecraft:grass_block",
        "minecraft:ancient_debris",
        "minecraft:cherry_log"
    };
    
    std::cout << std::setw(20) << "Block Name" 
              << std::setw(12) << "1.12.2 ID"
              << std::setw(12) << "1.16.5 ID" 
              << std::setw(12) << "1.21.7 ID" << "\n";
    std::cout << std::string(56, '-') << "\n";
    
    for (const auto& block_name : test_blocks) {
        auto block = registry.get_universal_block(block_name);
        if (!block) continue;
        
        std::cout << std::setw(20) << block_name;
        
        // 1.12.2
        auto id_1_12_2 = registry.universal_to_protocol(block->id, ProtocolVersion::MC_1_12_2);
        if (id_1_12_2) {
            std::cout << std::setw(12) << *id_1_12_2;
        } else {
            std::cout << std::setw(12) << "N/A";
        }
        
        // 1.16.5
        auto id_1_16_5 = registry.universal_to_protocol(block->id, ProtocolVersion::MC_1_16_5);
        if (id_1_16_5) {
            std::cout << std::setw(12) << *id_1_16_5;
        } else {
            std::cout << std::setw(12) << "N/A";
        }
        
        // 1.21.7
        auto id_1_21_7 = registry.universal_to_protocol(block->id, ProtocolVersion::MC_1_21_7);
        if (id_1_21_7) {
            std::cout << std::setw(12) << *id_1_21_7;
        } else {
            std::cout << std::setw(12) << "N/A";
        }
        
        std::cout << "\n";
    }
    
    std::cout << "\n";
}

void demonstrate_version_properties() {
    std::cout << "=== Version-Specific Properties ===\n";
    
    auto& registry = get_block_registry();
    
    // Test how block properties might change between versions
    auto stone_block = registry.get_universal_block("minecraft:stone");
    if (!stone_block) {
        std::cout << "Stone block not found!\n";
        return;
    }
    
    std::vector<int> versions = {
        ProtocolVersion::MC_1_12_2,
        ProtocolVersion::MC_1_16_5,
        ProtocolVersion::MC_1_21_7
    };
    
    std::cout << "Stone block properties across versions:\n";
    for (int version : versions) {
        auto properties = registry.get_properties(stone_block->id, version);
        
        std::cout << "  Version " << version << ":\n";
        std::cout << "    Hardness: " << properties.hardness << "\n";
        std::cout << "    Blast Resistance: " << properties.blast_resistance << "\n";
        std::cout << "    Requires Tool: " << (properties.requires_tool ? "Yes" : "No") << "\n";
    }
    
    std::cout << "\n";
}

void demonstrate_ecs_integration() {
    std::cout << "=== ECS Integration with Version-Aware Blocks ===\n";
    
    auto& block_registry = get_block_registry();
    
    // Set current protocol version
    block_registry.set_protocol_version(ProtocolVersion::MC_1_20_1);
    std::cout << "Set server protocol version to 1.20.1\n";
    
    // Create ECS registry
    Registry ecs_registry;
    ecs_registry.register_component<Position>();
    ecs_registry.register_component<Block>();
    ecs_registry.register_component<Physics>();
    
    auto& block_system = ecs_registry.add_system<BlockSystem>();
    
    std::cout << "Created ECS registry with block system\n";
    
    // Create blocks using universal IDs
    auto stone_block = block_registry.get_universal_block("minecraft:stone");
    auto cherry_log_block = block_registry.get_universal_block("minecraft:cherry_log");
    auto ancient_debris_block = block_registry.get_universal_block("minecraft:ancient_debris");
    
    if (stone_block) {
        Position pos(10, 64, 10);
        Block block_comp(stone_block->id);
        auto entity = block_system.create_block(ecs_registry, pos, block_comp);
        
        std::cout << "Created stone block entity: " << entity << "\n";
        
        // Get properties through the block component
        auto properties = block_comp.get_properties();
        std::cout << "  Hardness: " << properties.hardness << "\n";
    }
    
    if (cherry_log_block) {
        Position pos(11, 64, 10);
        Block block_comp(cherry_log_block->id);
        auto entity = block_system.create_block(ecs_registry, pos, block_comp);
        
        std::cout << "Created cherry log block entity: " << entity << "\n";
        std::cout << "  Available in 1.20.1: " << cherry_log_block->is_available_in(ProtocolVersion::MC_1_20_1) << "\n";
    }
    
    if (ancient_debris_block) {
        Position pos(12, 64, 10);
        Block block_comp(ancient_debris_block->id);
        auto entity = block_system.create_block(ecs_registry, pos, block_comp);
        
        std::cout << "Created ancient debris block entity: " << entity << "\n";
        std::cout << "  Available in 1.20.1: " << ancient_debris_block->is_available_in(ProtocolVersion::MC_1_20_1) << "\n";
    }
    
    // Test version switching
    std::cout << "\nSwitching server to 1.12.2...\n";
    block_registry.set_protocol_version(ProtocolVersion::MC_1_12_2);
    
    // Check how blocks would appear to 1.12.2 clients
    if (cherry_log_block) {
        auto fallback_id = cherry_log_block->get_fallback_for(ProtocolVersion::MC_1_12_2);
        auto fallback_block = block_registry.get_universal_block(fallback_id);
        
        std::cout << "Cherry log fallback for 1.12.2: ";
        if (fallback_block) {
            std::cout << fallback_block->name << "\n";
        } else {
            std::cout << "air\n";
        }
    }
    
    std::cout << "\n";
}

void demonstrate_version_statistics() {
    std::cout << "=== Version Statistics ===\n";
    
    auto& registry = get_block_registry();
    
    std::vector<int> versions = {
        ProtocolVersion::MC_1_12_2,
        ProtocolVersion::MC_1_16_5,
        ProtocolVersion::MC_1_20_1,
        ProtocolVersion::MC_1_21_7
    };
    
    std::cout << std::setw(10) << "Version" 
              << std::setw(12) << "Blocks"
              << std::setw(12) << "States" << "\n";
    std::cout << std::string(34, '-') << "\n";
    
    for (int version : versions) {
        auto stats = registry.get_version_stats(version);
        
        std::cout << std::setw(10) << version
                  << std::setw(12) << stats.block_count
                  << std::setw(12) << stats.state_count << "\n";
    }
    
    std::cout << "\n";
}

void demonstrate_multi_client_support() {
    std::cout << "=== Multi-Client Version Support ===\n";
    
    auto& registry = get_block_registry();
    
    // Simulate different clients connecting with different versions
    struct Client {
        int protocol_version;
        std::string name;
    };
    
    std::vector<Client> clients = {
        {ProtocolVersion::MC_1_12_2, "OldClient"},
        {ProtocolVersion::MC_1_16_5, "NetherClient"},
        {ProtocolVersion::MC_1_21_7, "ModernClient"}
    };
    
    // World contains modern blocks
    std::vector<std::string> world_blocks = {
        "minecraft:stone",
        "minecraft:grass_block", 
        "minecraft:ancient_debris",   // 1.16+
        "minecraft:deepslate",       // 1.17+
        "minecraft:cherry_log",      // 1.20+
        "minecraft:bamboo_planks"    // 1.20+
    };
    
    std::cout << "World contains " << world_blocks.size() << " different block types\n\n";
    
    for (const auto& client : clients) {
        std::cout << "Client: " << client.name << " (Protocol " << client.protocol_version << ")\n";
        std::cout << "Sees these blocks:\n";
        
        for (const auto& block_name : world_blocks) {
            auto block = registry.get_universal_block(block_name);
            if (!block) continue;
            
            if (block->is_available_in(client.protocol_version)) {
                // Client can see the actual block
                auto protocol_id = registry.universal_to_protocol(block->id, client.protocol_version);
                std::cout << "  " << block_name << " (Protocol ID: ";
                if (protocol_id) {
                    std::cout << *protocol_id << ")\n";
                } else {
                    std::cout << "unmapped)\n";
                }
            } else {
                // Client sees fallback block
                auto fallback_id = block->get_fallback_for(client.protocol_version);
                auto fallback_block = registry.get_universal_block(fallback_id);
                
                if (fallback_block && fallback_id != 0) {
                    auto protocol_id = registry.universal_to_protocol(fallback_id, client.protocol_version);
                    std::cout << "  " << block_name << " → " << fallback_block->name;
                    if (protocol_id) {
                        std::cout << " (Protocol ID: " << *protocol_id << ")\n";
                    } else {
                        std::cout << " (unmapped)\n";
                    }
                } else {
                    std::cout << "  " << block_name << " → air (removed)\n";
                }
            }
        }
        std::cout << "\n";
    }
}

int main() {
    std::cout << "ParallelStone Version-Aware Block System Demo\n";
    std::cout << "=============================================\n\n";
    
    try {
        demonstrate_basic_registry();
        demonstrate_version_compatibility();
        demonstrate_protocol_conversion();
        demonstrate_version_properties();
        demonstrate_ecs_integration();
        demonstrate_version_statistics();
        demonstrate_multi_client_support();
        
        std::cout << "All version-aware demonstrations completed successfully!\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}