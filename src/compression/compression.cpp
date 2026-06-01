#include "compression.hpp"
#include <cstring>
#include <stdexcept>

namespace columnar {

CodecId codec_for(CompressionType type) {
    switch (type) {
        case CompressionType::None:        return CodecId::None;
        case CompressionType::RLE:         return CodecId::RleInt64;
        case CompressionType::Dictionary:  return CodecId::DictString;
        case CompressionType::BitPacking:  return CodecId::BitPack;
        case CompressionType::Delta:       return CodecId::DeltaInt64;
        case CompressionType::DeltaLength: return CodecId::DLBA;
    }
    return CodecId::None;
}

static void put_u32(std::vector<uint8_t>& out, uint32_t v) {
    out.push_back(static_cast<uint8_t>(v & 0xFF));
    out.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
    out.push_back(static_cast<uint8_t>((v >> 16) & 0xFF));
    out.push_back(static_cast<uint8_t>((v >> 24) & 0xFF));
}
static uint32_t get_u32(const uint8_t* p) {
    return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) |
           (static_cast<uint32_t>(p[2]) << 16) | (static_cast<uint32_t>(p[3]) << 24);
}

std::vector<uint8_t> compress(const void* data, size_t size, CompressionType type) {
    const uint8_t* in = static_cast<const uint8_t*>(data);
    std::vector<uint8_t> out;

    switch (type) {
        case CompressionType::None: {
            out.assign(in, in + size);
            return out;
        }
        case CompressionType::RLE: {

            size_t i = 0;
            while (i < size) {
                uint8_t v = in[i];
                uint32_t run = 1;
                while (i + run < size && in[i + run] == v && run < 0xFFFFFFFF) ++run;
                put_u32(out, run);
                out.push_back(v);
                i += run;
            }
            return out;
        }
        default:

            throw std::runtime_error(
                "compress(): byte-level path supports only None and RLE; "
                "use column codecs (codec.hpp) for the rest");
    }
}

std::vector<uint8_t> decompress(const void* data, size_t size, CompressionType type) {
    const uint8_t* in = static_cast<const uint8_t*>(data);
    std::vector<uint8_t> out;

    switch (type) {
        case CompressionType::None: {
            out.assign(in, in + size);
            return out;
        }
        case CompressionType::RLE: {
            size_t i = 0;
            while (i + 5 <= size) {
                uint32_t run = get_u32(in + i);
                uint8_t v = in[i + 4];
                i += 5;
                out.insert(out.end(), run, v);
            }
            return out;
        }
        default:
            throw std::runtime_error(
                "decompress(): byte-level path supports only None and RLE");
    }
}

}
