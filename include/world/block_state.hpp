#pragma once

#include "compile_time_blocks.hpp"
#include "block_registry.hpp"
#include <unordered_map>
#include <string>
#include <variant>
#include <memory>

namespace parallelstone {
namespace world {

/**
 * @brief Value type for block properties
 */
using PropertyValue = std::variant<bool, std::int32_t, std::string>;

/**
 * @brief Block state representing a block type with its properties
 * 
 * Modern C++20 implementation using the compile-time block system.
 * Stores block type and properties in an efficient, immutable structure.
 */
class BlockState {
public:
    /**
     * @brief Default constructor (creates AIR block)
     */
    BlockState() noexcept : block_type_(BlockType::AIR) {}
    
    /**
     * @brief Construct block state from block type
     */
    explicit BlockState(BlockType type) noexcept : block_type_(type) {}
    
    /**
     * @brief Construct block state with properties
     */
    BlockState(BlockType type, const std::unordered_map<std::string, PropertyValue>& properties)
        : block_type_(type), properties_(properties) {}
    
    /**
     * @brief Get block type
     */
    BlockType get_block_type() const noexcept { return block_type_; }
    
    /**
     * @brief Check if block is air
     */
    bool is_air() const noexcept { return block_type_ == BlockType::AIR; }
    
    /**
     * @brief Get block properties
     */
    const std::unordered_map<std::string, PropertyValue>& get_properties() const noexcept {
        return properties_;
    }
    
    /**
     * @brief Get specific property value
     */
    template<typename T>
    T get_property(const std::string& name, const T& default_value = T{}) const {
        auto it = properties_.find(name);
        if (it != properties_.end()) {
            if (std::holds_alternative<T>(it->second)) {
                return std::get<T>(it->second);
            }
        }
        return default_value;
    }
    
    /**
     * @brief Set property value
     */
    template<typename T>
    void set_property(const std::string& name, const T& value) {
        properties_[name] = value;
    }
    
    /**
     * @brief Check if property exists
     */
    bool has_property(const std::string& name) const noexcept {
        return properties_.find(name) != properties_.end();
    }
    
    /**
     * @brief Get block properties from registry
     */
    BlockProperties get_block_properties() const {
        return BlockRegistry::get_properties(block_type_);
    }
    
    /**
     * @brief Get block name
     */
    std::string_view get_name() const {
        return BlockRegistry::get_name(block_type_);
    }
    
    /**
     * @brief Get protocol ID for network transmission
     */
    std::uint32_t get_protocol_id() const noexcept {
        // Simple implementation: block type ID
        // TODO: Implement property-aware protocol ID calculation
        return static_cast<std::uint32_t>(block_type_);
    }
    
    /**
     * @brief Create block state from protocol ID
     */
    static BlockState from_protocol_id(std::uint32_t protocol_id) {
        auto block_type = BlockRegistry::from_protocol_id(static_cast<std::uint16_t>(protocol_id));
        return BlockState(block_type);
    }
    
    /**
     * @brief Equality comparison
     */
    bool operator==(const BlockState& other) const noexcept {
        return block_type_ == other.block_type_ && properties_ == other.properties_;
    }
    
    /**
     * @brief Inequality comparison
     */
    bool operator!=(const BlockState& other) const noexcept {
        return !(*this == other);
    }
    
    /**
     * @brief Hash function for use in containers
     */
    std::size_t hash() const noexcept {
        std::size_t h1 = std::hash<std::uint16_t>{}(static_cast<std::uint16_t>(block_type_));
        std::size_t h2 = 0;
        
        // Simple property hash (not perfect but sufficient)
        for (const auto& [key, value] : properties_) {
            h2 ^= std::hash<std::string>{}(key);
            std::visit([&h2](const auto& v) {
                h2 ^= std::hash<std::decay_t<decltype(v)>>{}(v);
            }, value);
        }
        
        return h1 ^ (h2 << 1);
    }

private:
    BlockType block_type_;
    std::unordered_map<std::string, PropertyValue> properties_;
};

/**
 * @brief Block state registry for managing block states and protocol mappings
 */
class BlockStateRegistry {
public:
    /**
     * @brief Get or create block state from protocol ID
     */
    static const BlockState& get_block_state(std::uint32_t protocol_id);
    
    /**
     * @brief Get protocol ID for block state
     */
    static std::uint32_t get_protocol_id(const BlockState& state);
    
    /**
     * @brief Register custom block state
     */
    static void register_block_state(const BlockState& state, std::uint32_t protocol_id);
    
    /**
     * @brief Get all registered block states
     */
    static std::vector<BlockState> get_all_states();

private:
    static std::unordered_map<std::uint32_t, BlockState> protocol_to_state_;
    static std::unordered_map<BlockState, std::uint32_t> state_to_protocol_;
    static std::uint32_t next_protocol_id_;
};

} // namespace world
} // namespace parallelstone

/**
 * @brief Hash specialization for BlockState
 */
template<>
struct std::hash<parallelstone::world::BlockState> {
    std::size_t operator()(const parallelstone::world::BlockState& state) const noexcept {
        return state.hash();
    }
};