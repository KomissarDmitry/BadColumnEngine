#pragma once

/*
TODO:
Интерфейс:

std::vector<uint8_t> compress(const void* data, size_t size, CompressionType type);
std::vector<uint8_t> decompress(const void* data, size_t size, CompressionType type);

*/


namespace columnar {

//
enum class CompressionType {
    None,
    RLE,
    Dictionary,
    BitPacking,
    Delta,
    DeltaLength
};


}
