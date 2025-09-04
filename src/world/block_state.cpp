#include "world/block_state.hpp"
#include <mutex>

namespace parallelstone {
namespace world {

// Static member definitions
std::unordered_map<std::uint32_t, BlockState> BlockStateRegistry::protocol_to_state_;
std::unordered_map<BlockState, std::uint32_t> BlockStateRegistry::state_to_protocol_;
std::uint32_t BlockStateRegistry::next_protocol_id_ = 1;

const BlockState& BlockStateRegistry::get_block_state(std::uint32_t protocol_id) {
    static std::mutex registry_mutex;
    static BlockState air_state(BlockType::AIR);
    
    std::lock_guard lock(registry_mutex);
    
    auto it = protocol_to_state_.find(protocol_id);
    if (it != protocol_to_state_.end()) {
        return it->second;
    }
    
    // Create default block state from protocol ID
    auto block_type = BlockRegistry::from_protocol_id(static_cast<std::uint16_t>(protocol_id));
    if (block_type == BlockType::UNKNOWN) {
        return air_state;
    }
    
    BlockState state(block_type);
    protocol_to_state_[protocol_id] = state;
    state_to_protocol_[state] = protocol_id;
    
    return protocol_to_state_[protocol_id];
}

std::uint32_t BlockStateRegistry::get_protocol_id(const BlockState& state) {
    static std::mutex registry_mutex;
    
    std::lock_guard lock(registry_mutex);
    
    auto it = state_to_protocol_.find(state);
    if (it != state_to_protocol_.end()) {
        return it->second;
    }
    
    // Create new protocol ID
    std::uint32_t protocol_id = next_protocol_id_++;
    protocol_to_state_[protocol_id] = state;
    state_to_protocol_[state] = protocol_id;
    
    return protocol_id;
}

void BlockStateRegistry::register_block_state(const BlockState& state, std::uint32_t protocol_id) {
    static std::mutex registry_mutex;
    
    std::lock_guard lock(registry_mutex);
    
    protocol_to_state_[protocol_id] = state;
    state_to_protocol_[state] = protocol_id;
    
    if (protocol_id >= next_protocol_id_) {
        next_protocol_id_ = protocol_id + 1;
    }
}

std::vector<BlockState> BlockStateRegistry::get_all_states() {
    static std::mutex registry_mutex;
    
    std::lock_guard lock(registry_mutex);
    
    std::vector<BlockState> states;
    states.reserve(state_to_protocol_.size());
    
    for (const auto& [state, protocol_id] : state_to_protocol_) {
        states.push_back(state);
    }
    
    return states;
}

} // namespace world
} // namespace parallelstone