#pragma once

#include "../compression/codec.hpp"
#include <cstddef>
#include <cstdint>
#include <vector>

namespace columnar {

enum class CompressionType {
    None,
    RLE,
    Dictionary,
    BitPacking,
    Delta,
    DeltaLength
};

CodecId codec_for(CompressionType type);

std::vector<uint8_t> compress(const void* data, size_t size, CompressionType type);

std::vector<uint8_t> decompress(const void* data, size_t size, CompressionType type);

}
