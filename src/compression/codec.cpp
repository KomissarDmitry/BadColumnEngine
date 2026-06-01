#include "codec.hpp"
#include <cstring>

namespace columnar {

void bitpack_encode(const std::vector<uint64_t>& values,
                    uint8_t bit_width,
                    std::vector<uint8_t>& out) {
    if (bit_width == 0) return;
    size_t total_bits = values.size() * static_cast<size_t>(bit_width);
    size_t total_bytes = (total_bits + 7) / 8;
    size_t base = out.size();
    out.resize(base + total_bytes, 0);

    size_t bit_pos = 0;
    uint64_t mask = (bit_width >= 64) ? ~0ULL : ((1ULL << bit_width) - 1);
    for (uint64_t v : values) {
        v &= mask;

        size_t b = base + bit_pos / 8;
        size_t shift = bit_pos % 8;

        uint64_t shifted = v << shift;
        for (size_t k = 0; k < 9 && shift + bit_width > k * 8; ++k) {
            if (b + k >= out.size()) break;
            out[b + k] |= static_cast<uint8_t>((shifted >> (8 * k)) & 0xFF);
        }
        bit_pos += bit_width;
    }
}

void bitpack_decode(const uint8_t* buf, size_t buf_size,
                    uint64_t count, uint8_t bit_width,
                    std::vector<uint64_t>& values) {
    values.clear();
    values.reserve(count);
    if (bit_width == 0) {
        values.assign(count, 0);
        return;
    }
    uint64_t mask = (bit_width >= 64) ? ~0ULL : ((1ULL << bit_width) - 1);
    size_t bit_pos = 0;
    for (uint64_t i = 0; i < count; ++i) {
        size_t b = bit_pos / 8;
        size_t shift = bit_pos % 8;
        uint64_t v = 0;
        for (size_t k = 0; k < 9; ++k) {
            if (b + k >= buf_size) break;
            v |= static_cast<uint64_t>(buf[b + k]) << (8 * k);
            if ((k + 1) * 8 >= shift + bit_width) break;
        }
        v = (v >> shift) & mask;
        values.push_back(v);
        bit_pos += bit_width;
    }
}

}
