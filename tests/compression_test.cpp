#include <gtest/gtest.h>
#include "../src/compression/compression.hpp"
#include <string>

using namespace columnar;

TEST(CompressionTest, NoneRoundtrip) {
    std::string data = "hello world";
    auto c = compress(data.data(), data.size(), CompressionType::None);
    auto d = decompress(c.data(), c.size(), CompressionType::None);
    EXPECT_EQ(std::string(d.begin(), d.end()), data);
}

TEST(CompressionTest, RleRoundtrip) {
    std::string data = "aaaaabbbccccccccd";
    auto c = compress(data.data(), data.size(), CompressionType::RLE);

    auto d = decompress(c.data(), c.size(), CompressionType::RLE);
    EXPECT_EQ(std::string(d.begin(), d.end()), data);
}

TEST(CompressionTest, RleCompressesLongRun) {
    std::string data(1000, 'x');
    auto c = compress(data.data(), data.size(), CompressionType::RLE);
    EXPECT_EQ(c.size(), 5u);
    auto d = decompress(c.data(), c.size(), CompressionType::RLE);
    EXPECT_EQ(d.size(), 1000u);
}

TEST(CompressionTest, CodecMapping) {
    EXPECT_EQ(codec_for(CompressionType::RLE), CodecId::RleInt64);
    EXPECT_EQ(codec_for(CompressionType::Dictionary), CodecId::DictString);
    EXPECT_EQ(codec_for(CompressionType::Delta), CodecId::DeltaInt64);
}
