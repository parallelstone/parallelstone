#pragma once

#include "world.hpp"
#include "world_ecs_integration.hpp"
#include "network/packet_view.hpp"
#include <memory>
#include <vector>
#include <unordered_set>

namespace parallelstone {

// Forward declarations
namespace server { class Session; }

namespace world {

/**
 * @brief Network integration for world system
 * 
 * Handles chunk data transmission, block updates, and entity synchronization
 * between the world system and connected clients.
 */
class WorldNetworkHandler {
public:
    /**
     * @brief Initialize network handler for world
     */
    explicit WorldNetworkHandler(std::shared_ptr<WorldECSIntegration> world_integration);
    
    /**
     * @brief Get the world ECS integration
     */
    std::shared_ptr<WorldECSIntegration> get_world_integration() { return world_integration_; }
    
    // ==================== CLIENT MANAGEMENT ====================
    
    /**
     * @brief Add client session to world
     */
    void add_client(std::shared_ptr<server::Session> session);
    
    /**
     * @brief Remove client session from world
     */
    void remove_client(std::shared_ptr<server::Session> session);
    
    /**
     * @brief Get all connected clients
     */
    std::vector<std::shared_ptr<server::Session>> get_clients() const;
    
    /**
     * @brief Get clients in specific chunk
     */
    std::vector<std::shared_ptr<server::Session>> get_clients_in_chunk(std::int32_t chunk_x, std::int32_t chunk_z) const;
    
    // ==================== CHUNK TRANSMISSION ====================
    
    /**
     * @brief Send chunk data to client
     */
    void send_chunk_data(std::shared_ptr<server::Session> session, std::int32_t chunk_x, std::int32_t chunk_z);
    
    /**
     * @brief Send unload chunk packet to client
     */
    void send_unload_chunk(std::shared_ptr<server::Session> session, std::int32_t chunk_x, std::int32_t chunk_z);
    
    /**
     * @brief Update client view distance
     */
    void update_client_view_distance(std::shared_ptr<server::Session> session, std::int32_t view_distance);
    
    /**
     * @brief Send chunks around position to client
     */
    void send_chunks_around(std::shared_ptr<server::Session> session, 
                           std::int32_t center_x, std::int32_t center_z, std::int32_t radius);
    
    // ==================== BLOCK UPDATES ====================
    
    /**
     * @brief Send block update to clients
     */
    void broadcast_block_change(std::int32_t x, std::int32_t y, std::int32_t z, BlockType block_type);
    
    /**
     * @brief Send block update to specific client
     */
    void send_block_change(std::shared_ptr<server::Session> session, 
                          std::int32_t x, std::int32_t y, std::int32_t z, BlockType block_type);
    
    /**
     * @brief Send multiple block updates
     */
    void send_multi_block_change(std::shared_ptr<server::Session> session,
                                std::int32_t chunk_x, std::int32_t chunk_z,
                                const std::vector<std::tuple<std::uint8_t, std::int32_t, std::uint8_t, BlockType>>& changes);
    
    // ==================== ENTITY SYNCHRONIZATION ====================
    
    /**
     * @brief Send entity spawn packet
     */
    void broadcast_entity_spawn(ecs::Entity entity);
    
    /**
     * @brief Send entity despawn packet
     */
    void broadcast_entity_despawn(ecs::Entity entity);
    
    /**
     * @brief Send entity movement update
     */
    void broadcast_entity_movement(ecs::Entity entity);
    
    /**
     * @brief Send entity metadata update
     */
    void broadcast_entity_metadata(ecs::Entity entity);
    
    /**
     * @brief Synchronize all entities in chunk to client
     */
    void sync_chunk_entities(std::shared_ptr<server::Session> session, std::int32_t chunk_x, std::int32_t chunk_z);
    
    // ==================== PACKET HANDLERS ====================
    
    /**
     * @brief Handle player block placement
     */
    bool handle_block_place(std::shared_ptr<server::Session> session, network::PacketView& packet);
    
    /**
     * @brief Handle player block breaking
     */
    bool handle_block_break(std::shared_ptr<server::Session> session, network::PacketView& packet);
    
    /**
     * @brief Handle player movement
     */
    bool handle_player_movement(std::shared_ptr<server::Session> session, network::PacketView& packet);
    
    /**
     * @brief Handle chunk request
     */
    bool handle_chunk_request(std::shared_ptr<server::Session> session, network::PacketView& packet);
    
    // ==================== WORLD EVENTS ====================
    
    /**
     * @brief Handle world chunk loaded event
     */
    void on_chunk_loaded(std::int32_t chunk_x, std::int32_t chunk_z);
    
    /**
     * @brief Handle world chunk unloaded event
     */
    void on_chunk_unloaded(std::int32_t chunk_x, std::int32_t chunk_z);
    
    /**
     * @brief Handle world block changed event
     */
    void on_block_changed(std::int32_t x, std::int32_t y, std::int32_t z, BlockType old_block, BlockType new_block);
    
    /**
     * @brief Handle entity created event
     */
    void on_entity_created(ecs::Entity entity);
    
    /**
     * @brief Handle entity destroyed event
     */
    void on_entity_destroyed(ecs::Entity entity);
    
    // ==================== OPTIMIZATION ====================
    
    /**
     * @brief Update network priorities and batching
     */
    void update_network_optimization();
    
    /**
     * @brief Get network statistics
     */
    struct NetworkStats {
        std::uint64_t chunks_sent = 0;
        std::uint64_t block_updates_sent = 0;
        std::uint64_t entities_synchronized = 0;
        std::uint64_t bytes_transmitted = 0;
        float average_chunk_size = 0.0f;
    };
    
    NetworkStats get_network_stats() const { return network_stats_; }

private:
    /**
     * @brief Serialize chunk data for network transmission
     */
    std::vector<std::uint8_t> serialize_chunk_data(std::int32_t chunk_x, std::int32_t chunk_z);
    
    /**
     * @brief Create block change packet data
     */
    std::vector<std::uint8_t> create_block_change_packet(std::int32_t x, std::int32_t y, std::int32_t z, BlockType block_type);
    
    /**
     * @brief Create entity spawn packet data
     */
    std::vector<std::uint8_t> create_entity_spawn_packet(ecs::Entity entity);
    
    /**
     * @brief Get clients that can see the specified position
     */
    std::vector<std::shared_ptr<server::Session>> get_clients_in_range(std::int32_t x, std::int32_t z, std::int32_t range = 16) const;
    
    /**
     * @brief Update client view tracking
     */
    void update_client_views();
    
    std::shared_ptr<WorldECSIntegration> world_integration_;
    
    // Client management
    std::vector<std::weak_ptr<server::Session>> clients_;
    std::unordered_map<std::shared_ptr<server::Session>, std::unordered_set<std::uint64_t>> client_loaded_chunks_;
    std::unordered_map<std::shared_ptr<server::Session>, std::int32_t> client_view_distances_;
    
    // Network statistics
    mutable NetworkStats network_stats_;
    
    // Optimization
    std::vector<std::tuple<std::int32_t, std::int32_t, std::int32_t, BlockType>> pending_block_updates_;
    std::chrono::steady_clock::time_point last_network_update_;
};

/**
 * @brief Chunk data packet structure for network transmission
 */
struct ChunkDataPacket {
    std::int32_t chunk_x;
    std::int32_t chunk_z;
    bool is_full_chunk;
    std::vector<std::uint8_t> heightmap_data;
    std::vector<std::uint8_t> biome_data;
    std::vector<std::uint8_t> section_data;
    std::vector<std::uint8_t> block_entity_data;
    
    /**
     * @brief Serialize chunk packet to bytes
     */
    std::vector<std::uint8_t> serialize() const;
    
    /**
     * @brief Deserialize chunk packet from bytes
     */
    static ChunkDataPacket deserialize(const std::vector<std::uint8_t>& data);
    
    /**
     * @brief Calculate packet size
     */
    std::size_t calculate_size() const;
};

/**
 * @brief Block change packet for efficient updates
 */
struct BlockChangePacket {
    std::int32_t x, y, z;
    std::uint32_t block_state_id;
    
    /**
     * @brief Serialize to bytes
     */
    std::vector<std::uint8_t> serialize() const;
    
    /**
     * @brief Deserialize from bytes
     */
    static BlockChangePacket deserialize(const std::vector<std::uint8_t>& data);
};

/**
 * @brief Multi-block change packet for batch updates
 */
struct MultiBlockChangePacket {
    std::int32_t chunk_x, chunk_z;
    std::vector<std::tuple<std::uint8_t, std::int32_t, std::uint8_t, std::uint32_t>> changes; // x, y, z, state_id
    
    /**
     * @brief Serialize to bytes
     */
    std::vector<std::uint8_t> serialize() const;
    
    /**
     * @brief Deserialize from bytes
     */
    static MultiBlockChangePacket deserialize(const std::vector<std::uint8_t>& data);
};

/**
 * @brief Entity packet structures
 */
struct EntitySpawnPacket {
    std::int32_t entity_id;
    std::string entity_type;
    double x, y, z;
    float yaw, pitch;
    std::vector<std::uint8_t> metadata;
    
    std::vector<std::uint8_t> serialize() const;
    static EntitySpawnPacket deserialize(const std::vector<std::uint8_t>& data);
};

struct EntityDespawnPacket {
    std::int32_t entity_id;
    
    std::vector<std::uint8_t> serialize() const;
    static EntityDespawnPacket deserialize(const std::vector<std::uint8_t>& data);
};

struct EntityMovementPacket {
    std::int32_t entity_id;
    double x, y, z;
    float yaw, pitch;
    bool on_ground;
    
    std::vector<std::uint8_t> serialize() const;
    static EntityMovementPacket deserialize(const std::vector<std::uint8_t>& data);
};

/**
 * @brief Network packet factory for world-related packets
 */
class WorldPacketFactory {
public:
    /**
     * @brief Create chunk data packet
     */
    static ChunkDataPacket create_chunk_data_packet(const Chunk& chunk, std::int32_t chunk_x, std::int32_t chunk_z);
    
    /**
     * @brief Create block change packet
     */
    static BlockChangePacket create_block_change_packet(std::int32_t x, std::int32_t y, std::int32_t z, const BlockState& state);
    
    /**
     * @brief Create entity spawn packet from ECS entity
     */
    static EntitySpawnPacket create_entity_spawn_packet(const ecs::Registry& registry, ecs::Entity entity);
    
    /**
     * @brief Create entity movement packet from ECS entity
     */
    static EntityMovementPacket create_entity_movement_packet(const ecs::Registry& registry, ecs::Entity entity);
    
    /**
     * @brief Get packet type ID for world packets
     */
    static constexpr std::uint8_t CHUNK_DATA_PACKET_ID = 0x20;
    static constexpr std::uint8_t BLOCK_CHANGE_PACKET_ID = 0x21;
    static constexpr std::uint8_t MULTI_BLOCK_CHANGE_PACKET_ID = 0x22;
    static constexpr std::uint8_t ENTITY_SPAWN_PACKET_ID = 0x23;
    static constexpr std::uint8_t ENTITY_DESPAWN_PACKET_ID = 0x24;
    static constexpr std::uint8_t ENTITY_MOVEMENT_PACKET_ID = 0x25;
};

} // namespace world
} // namespace parallelstone