#pragma once

#include "../core/column.hpp"
#include <cstdint>
#include <vector>

namespace columnar {

enum class CodecId : uint8_t {
    None       = 0,
    BitPack    = 1,
    DictString = 2,
    RleInt64   = 3,
    DeltaInt64 = 4,
    DLBA       = 5,
};

inline uint8_t bits_for(uint64_t range) {
    if (range == 0) return 1;
    uint8_t b = 0;
    while (range) { ++b; range >>= 1; }
    return b;
}

void bitpack_encode(const std::vector<uint64_t>& values,
                    uint8_t bit_width,
                    std::vector<uint8_t>& out);

void bitpack_decode(const uint8_t* buf, size_t buf_size,
                    uint64_t count, uint8_t bit_width,
                    std::vector<uint64_t>& values);

}
