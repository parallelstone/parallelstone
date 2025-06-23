#include "../../include/protocol/data_types.hpp"
#include <stdexcept>
#include <cstring>

namespace minecraft::protocol {

// BitSet serialization implementation
BitSet ByteBuffer::readBitSet() {
    // Read the length of the BitSet as VarInt
    int32_t length = readVarInt();
    
    // Create BitSet with appropriate size
    BitSet bitSet(length * 64); // Each long contains 64 bits
    
    // Read the data array
    auto &data = bitSet.getData();
    for (int32_t i = 0; i < length; ++i) {
        uint64_t value = readLong();
        data[i] = value;
    }
    
    return bitSet;
}

void ByteBuffer::writeBitSet(const BitSet &bitSet) {
    // Write the length as VarInt (number of longs)
    const auto &data = bitSet.getData();
    int32_t length = static_cast<int32_t>(data.size());
    writeVarInt(length);
    
    // Write each long in the data array
    for (const auto &value : data) {
        writeLong(value);
    }
}

// BitSet class implementation
BitSet::BitSet(size_t size) : bitCount(size) {
    // Calculate number of 64-bit words needed
    size_t wordCount = (size + 63) / 64;
    data.resize(wordCount, 0);
}

void BitSet::set(size_t index, bool value) {
    if (index >= bitCount) {
        throw std::out_of_range("BitSet index out of range");
    }
    
    size_t wordIndex = index / 64;
    size_t bitIndex = index % 64;
    
    if (value) {
        data[wordIndex] |= (1ULL << bitIndex);
    } else {
        data[wordIndex] &= ~(1ULL << bitIndex);
    }
}

bool BitSet::get(size_t index) const {
    if (index >= bitCount) {
        throw std::out_of_range("BitSet index out of range");
    }
    
    size_t wordIndex = index / 64;
    size_t bitIndex = index % 64;
    
    return (data[wordIndex] & (1ULL << bitIndex)) != 0;
}

void BitSet::clear() {
    std::fill(data.begin(), data.end(), 0);
}

} // namespace minecraft::protocol