#include "columnar_store.hpp"
#include "csv.hpp"
#include "../compression/codec.hpp"
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <unordered_map>
#include <optional>
#include <sstream>

namespace columnar {

#ifndef ROWS_PER_GROUP
#define ROWS_PER_GROUP 65536
#endif

static const char MAGIC_V2[4] = {'B', 'C', 'E', '2'};
static const char MAGIC_V3[4] = {'B', 'C', 'E', '3'};
static const uint8_t CODEC_NONE = 0;

static bool compress_enabled() {
    static bool v = []() {
        const char* e = std::getenv("BCE_COMPRESS");
        return e ? (std::string(e) != "0") : true;
    }();
    return v;
}

template <class T>
static void put(std::ostream& o, const T& v) {
    o.write(reinterpret_cast<const char*>(&v), sizeof(T));
}
template <class T>
static T get(std::istream& i) {
    T v{};
    i.read(reinterpret_cast<char*>(&v), sizeof(T));
    return v;
}

template <class T>
static void buf_put(std::vector<uint8_t>& b, const T& v) {
    const uint8_t* p = reinterpret_cast<const uint8_t*>(&v);
    b.insert(b.end(), p, p + sizeof(T));
}
static void buf_put_bytes(std::vector<uint8_t>& b, const void* data, size_t n) {
    const uint8_t* p = reinterpret_cast<const uint8_t*>(data);
    b.insert(b.end(), p, p + n);
}

static std::vector<uint8_t> try_none(const Column& col, size_t start, size_t count) {
    std::vector<uint8_t> b;
    b.push_back(static_cast<uint8_t>(CodecId::None));
    std::ostringstream oss;
    col.write_binary_range(oss, start, count);
    std::string s = oss.str();
    buf_put_bytes(b, s.data(), s.size());
    return b;
}

static std::optional<std::vector<uint8_t>> try_bitpack(
        const Column& col, size_t start, size_t count, int64_t mn, int64_t mx) {
    if (col.type() != DataType::Int64 || count == 0) return std::nullopt;
    const auto& d = static_cast<const Int64Column&>(col).data();
    uint64_t range = static_cast<uint64_t>(mx - mn);
    uint8_t bw = bits_for(range);
    std::vector<uint8_t> b;
    b.push_back(static_cast<uint8_t>(CodecId::BitPack));
    buf_put<int64_t>(b, mn);
    buf_put<uint8_t>(b, bw);
    buf_put<uint64_t>(b, count);
    std::vector<uint64_t> vals(count);
    for (size_t i = 0; i < count; ++i)
        vals[i] = static_cast<uint64_t>(d[start + i] - mn);
    bitpack_encode(vals, bw, b);
    return b;
}

static std::optional<std::vector<uint8_t>> try_dict_string(
        const Column& col, size_t start, size_t count) {
    if (col.type() != DataType::String || count == 0) return std::nullopt;
    const auto& d = static_cast<const StringColumn&>(col).data();
    std::unordered_map<std::string, uint32_t> dict;
    std::vector<uint32_t> codes;
    codes.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        const std::string& s = d[start + i];
        auto it = dict.find(s);
        if (it == dict.end()) {
            uint32_t id = static_cast<uint32_t>(dict.size());
            dict.emplace(s, id);
            codes.push_back(id);
        } else {
            codes.push_back(it->second);
        }
    }
    std::vector<std::string> entries(dict.size());
    for (auto& kv : dict) entries[kv.second] = kv.first;
    uint8_t cw = bits_for(dict.empty() ? 0 : dict.size() - 1);

    std::vector<uint8_t> b;
    b.push_back(static_cast<uint8_t>(CodecId::DictString));
    buf_put<uint32_t>(b, static_cast<uint32_t>(dict.size()));
    for (const auto& s : entries) {
        buf_put<uint32_t>(b, static_cast<uint32_t>(s.size()));
        buf_put_bytes(b, s.data(), s.size());
    }
    buf_put<uint32_t>(b, static_cast<uint32_t>(count));
    buf_put<uint8_t>(b, cw);
    std::vector<uint64_t> vals(codes.begin(), codes.end());
    bitpack_encode(vals, cw, b);
    return b;
}

static std::optional<std::vector<uint8_t>> try_rle_int64(
        const Column& col, size_t start, size_t count) {
    if (col.type() != DataType::Int64 || count == 0) return std::nullopt;
    const auto& d = static_cast<const Int64Column&>(col).data();

    std::vector<std::pair<int64_t, uint32_t>> runs;
    int64_t cur = d[start];
    uint32_t len = 1;
    for (size_t i = 1; i < count; ++i) {
        int64_t v = d[start + i];
        if (v == cur && len < 0xFFFFFFFF) {
            ++len;
        } else {
            runs.emplace_back(cur, len);
            cur = v; len = 1;
        }
    }
    runs.emplace_back(cur, len);

    std::vector<uint8_t> b;
    b.push_back(static_cast<uint8_t>(CodecId::RleInt64));
    buf_put<uint32_t>(b, static_cast<uint32_t>(runs.size()));
    for (auto& [v, l] : runs) {
        buf_put<int64_t>(b, v);
        buf_put<uint32_t>(b, l);
    }
    return b;
}

static std::optional<std::vector<uint8_t>> try_delta_int64(
        const Column& col, size_t start, size_t count) {
    if (col.type() != DataType::Int64 || count == 0) return std::nullopt;
    const auto& d = static_cast<const Int64Column&>(col).data();
    int64_t first = d[start];
    if (count == 1) {
        std::vector<uint8_t> b;
        b.push_back(static_cast<uint8_t>(CodecId::DeltaInt64));
        buf_put<uint64_t>(b, count);
        buf_put<int64_t>(b, first);
        buf_put<int64_t>(b, 0);
        buf_put<uint8_t>(b, 1);
        return b;
    }

    std::vector<int64_t> deltas(count - 1);
    int64_t mn = INT64_MAX, mx = INT64_MIN;
    for (size_t i = 1; i < count; ++i) {
        int64_t v = d[start + i] - d[start + i - 1];
        deltas[i - 1] = v;
        mn = std::min(mn, v);
        mx = std::max(mx, v);
    }
    uint64_t range = static_cast<uint64_t>(mx - mn);
    uint8_t bw = bits_for(range);
    std::vector<uint8_t> b;
    b.push_back(static_cast<uint8_t>(CodecId::DeltaInt64));
    buf_put<uint64_t>(b, count);
    buf_put<int64_t>(b, first);
    buf_put<int64_t>(b, mn);
    buf_put<uint8_t>(b, bw);
    std::vector<uint64_t> vals(deltas.size());
    for (size_t i = 0; i < deltas.size(); ++i)
        vals[i] = static_cast<uint64_t>(deltas[i] - mn);
    bitpack_encode(vals, bw, b);
    return b;
}

static std::optional<std::vector<uint8_t>> try_dlba(
        const Column& col, size_t start, size_t count) {
    if (col.type() != DataType::String || count == 0) return std::nullopt;
    const auto& d = static_cast<const StringColumn&>(col).data();
    uint32_t mn = UINT32_MAX, mx = 0;
    uint64_t total = 0;
    for (size_t i = 0; i < count; ++i) {
        uint32_t l = static_cast<uint32_t>(d[start + i].size());
        mn = std::min(mn, l);
        mx = std::max(mx, l);
        total += l;
    }
    uint8_t bw = bits_for(static_cast<uint64_t>(mx - mn));
    std::vector<uint8_t> b;
    b.push_back(static_cast<uint8_t>(CodecId::DLBA));
    buf_put<uint32_t>(b, static_cast<uint32_t>(count));
    buf_put<uint32_t>(b, mn);
    buf_put<uint8_t>(b, bw);
    std::vector<uint64_t> lens(count);
    for (size_t i = 0; i < count; ++i)
        lens[i] = static_cast<uint64_t>(d[start + i].size()) - mn;
    bitpack_encode(lens, bw, b);
    buf_put<uint64_t>(b, total);

    for (size_t i = 0; i < count; ++i) {
        const std::string& s = d[start + i];
        buf_put_bytes(b, s.data(), s.size());
    }
    return b;
}

static void encode_chunk(std::ostream& out,
                         const Column& col,
                         size_t start, size_t count,
                         int64_t min_i, int64_t max_i) {
    if (!compress_enabled()) {
        auto raw = try_none(col, start, count);
        out.write(reinterpret_cast<const char*>(raw.data()), raw.size());
        return;
    }

    std::vector<uint8_t> best = try_none(col, start, count);

    auto consider = [&](std::optional<std::vector<uint8_t>> cand) {
        if (cand && cand->size() < best.size()) best = std::move(*cand);
    };
    consider(try_bitpack(col, start, count, min_i, max_i));
    consider(try_dict_string(col, start, count));
    consider(try_rle_int64(col, start, count));
    consider(try_delta_int64(col, start, count));
    consider(try_dlba(col, start, count));

    out.write(reinterpret_cast<const char*>(best.data()), best.size());
}

static std::shared_ptr<Column> decode_chunk(std::istream& in,
                                            DataType type, uint64_t count) {
    uint8_t codec_byte = get<uint8_t>(in);
    auto codec = static_cast<CodecId>(codec_byte);
    switch (codec) {
        case CodecId::None: {
            auto c = make_column(type);
            c->read_binary(in, count);
            return std::shared_ptr<Column>(c.release());
        }
        case CodecId::BitPack: {
            if (type != DataType::Int64)
                throw std::runtime_error("BitPack only valid for Int64");
            int64_t base = get<int64_t>(in);
            uint8_t bw = get<uint8_t>(in);
            uint64_t cnt = get<uint64_t>(in);
            if (cnt != count) throw std::runtime_error("BitPack count mismatch");
            size_t bytes = (cnt * static_cast<size_t>(bw) + 7) / 8;
            std::vector<uint8_t> buf(bytes);
            in.read(reinterpret_cast<char*>(buf.data()), bytes);
            std::vector<uint64_t> raw;
            bitpack_decode(buf.data(), buf.size(), cnt, bw, raw);
            auto c = std::make_shared<Int64Column>();
            for (uint64_t v : raw) c->push(base + static_cast<int64_t>(v));
            return c;
        }
        case CodecId::DictString: {
            if (type != DataType::String)
                throw std::runtime_error("DictString only valid for String");
            uint32_t dict_size = get<uint32_t>(in);
            std::vector<std::string> dict(dict_size);
            for (uint32_t i = 0; i < dict_size; ++i) {
                uint32_t len = get<uint32_t>(in);
                dict[i].resize(len);
                in.read(dict[i].data(), len);
            }
            uint32_t cnt = get<uint32_t>(in);
            if (cnt != count) throw std::runtime_error("DictString count mismatch");
            uint8_t cw = get<uint8_t>(in);
            size_t bytes = (cnt * static_cast<size_t>(cw) + 7) / 8;
            std::vector<uint8_t> buf(bytes);
            in.read(reinterpret_cast<char*>(buf.data()), bytes);
            std::vector<uint64_t> codes;
            bitpack_decode(buf.data(), buf.size(), cnt, cw, codes);
            auto c = std::make_shared<StringColumn>();
            for (uint64_t code : codes) {
                if (code >= dict.size())
                    throw std::runtime_error("DictString: code out of range");
                c->append_from_string(dict[code]);
            }
            return c;
        }
        case CodecId::RleInt64: {
            if (type != DataType::Int64)
                throw std::runtime_error("RleInt64 only valid for Int64");
            uint32_t nruns = get<uint32_t>(in);
            auto c = std::make_shared<Int64Column>();
            for (uint32_t i = 0; i < nruns; ++i) {
                int64_t v = get<int64_t>(in);
                uint32_t l = get<uint32_t>(in);
                for (uint32_t k = 0; k < l; ++k) c->push(v);
            }
            if (c->size() != count) throw std::runtime_error("RleInt64 count mismatch");
            return c;
        }
        case CodecId::DeltaInt64: {
            if (type != DataType::Int64)
                throw std::runtime_error("DeltaInt64 only valid for Int64");
            uint64_t cnt = get<uint64_t>(in);
            if (cnt != count) throw std::runtime_error("DeltaInt64 count mismatch");
            int64_t first = get<int64_t>(in);
            int64_t mn = get<int64_t>(in);
            uint8_t bw = get<uint8_t>(in);
            auto c = std::make_shared<Int64Column>();
            if (cnt == 0) return c;
            c->push(first);
            if (cnt == 1) return c;
            size_t nd = cnt - 1;
            size_t bytes = (nd * static_cast<size_t>(bw) + 7) / 8;
            std::vector<uint8_t> buf(bytes);
            in.read(reinterpret_cast<char*>(buf.data()), bytes);
            std::vector<uint64_t> raw;
            bitpack_decode(buf.data(), buf.size(), nd, bw, raw);
            int64_t prev = first;
            for (uint64_t v : raw) {
                int64_t delta = mn + static_cast<int64_t>(v);
                prev += delta;
                c->push(prev);
            }
            return c;
        }
        case CodecId::DLBA: {
            if (type != DataType::String)
                throw std::runtime_error("DLBA only valid for String");
            uint32_t cnt = get<uint32_t>(in);
            if (cnt != count) throw std::runtime_error("DLBA count mismatch");
            uint32_t mn = get<uint32_t>(in);
            uint8_t bw = get<uint8_t>(in);
            size_t lbytes = (cnt * static_cast<size_t>(bw) + 7) / 8;
            std::vector<uint8_t> lbuf(lbytes);
            in.read(reinterpret_cast<char*>(lbuf.data()), lbytes);
            std::vector<uint64_t> raw;
            bitpack_decode(lbuf.data(), lbuf.size(), cnt, bw, raw);
            uint64_t total = get<uint64_t>(in);
            std::vector<char> blob(total);
            in.read(blob.data(), total);
            auto c = std::make_shared<StringColumn>();
            size_t off = 0;
            for (uint32_t i = 0; i < cnt; ++i) {
                size_t len = mn + raw[i];
                c->append_from_string(std::string(blob.data() + off, len));
                off += len;
            }
            return c;
        }
        default:
            throw std::runtime_error("Unknown codec: " +
                                     std::to_string(static_cast<int>(codec_byte)));
    }
}

void ColumnStoreWriter::convert_csv(const std::string& data_csv,
                                    const std::string& schema_csv,
                                    const std::string& out) {
    Schema schema = Schema::read_from_csv(schema_csv);

    std::ofstream file(out, std::ios::binary);
    if (!file) throw std::runtime_error("Cannot open file for writing: " + out);
    file.write(MAGIC_V3, 4);

    const uint64_t rpg = ROWS_PER_GROUP;
    std::vector<RowGroupMeta> groups;
    uint64_t total_rows = 0;

    std::vector<std::unique_ptr<Column>> columns;
    for (size_t i = 0; i < schema.column_count(); ++i)
        columns.push_back(make_column(schema.column(i).type));
    uint64_t buffered = 0;

    auto flush_group = [&]() {
        RowGroupMeta g;
        g.num_rows = buffered;

        for (size_t c = 0; c < columns.size(); ++c) {
            ChunkMeta cm;
            cm.offset = static_cast<uint64_t>(file.tellp());

            DataType t = schema.column(c).type;
            int64_t mn_i = 0, mx_i = 0;
            if (buffered > 0 && t == DataType::Int64) {
                const auto& d = static_cast<Int64Column*>(columns[c].get())->data();
                mn_i = std::numeric_limits<int64_t>::max();
                mx_i = std::numeric_limits<int64_t>::min();
                for (uint64_t r = 0; r < buffered; ++r) {
                    mn_i = std::min(mn_i, d[r]);
                    mx_i = std::max(mx_i, d[r]);
                }
                cm.has_stats = true; cm.min_i = mn_i; cm.max_i = mx_i;
            } else if (buffered > 0 && t == DataType::Float64) {
                const auto& d = static_cast<Float64Column*>(columns[c].get())->data();
                double mn = std::numeric_limits<double>::infinity();
                double mx = -std::numeric_limits<double>::infinity();
                for (uint64_t r = 0; r < buffered; ++r) {
                    mn = std::min(mn, d[r]);
                    mx = std::max(mx, d[r]);
                }
                cm.has_stats = true; cm.min_f = mn; cm.max_f = mx;
            }

            encode_chunk(file, *columns[c], 0, buffered, mn_i, mx_i);
            cm.length = static_cast<uint64_t>(file.tellp()) - cm.offset;
            g.columns.push_back(cm);
        }
        groups.push_back(std::move(g));

        for (size_t i = 0; i < schema.column_count(); ++i)
            columns[i] = make_column(schema.column(i).type);
        buffered = 0;
    };

    CsvReader reader(data_csv);
    std::vector<std::string> row;
    while (reader.read_row(row)) {
        if (row.size() != schema.column_count())
            throw std::runtime_error("Row has wrong number of fields: got " +
                                     std::to_string(row.size()) + ", expected " +
                                     std::to_string(schema.column_count()));
        for (size_t i = 0; i < row.size(); ++i)
            columns[i]->append_from_string(row[i]);
        ++buffered;
        ++total_rows;
        if (buffered == rpg) flush_group();
    }

    if (buffered > 0 || total_rows == 0) flush_group();

    uint64_t footer_start = static_cast<uint64_t>(file.tellp());

    put<uint32_t>(file, static_cast<uint32_t>(schema.column_count()));
    for (size_t i = 0; i < schema.column_count(); ++i) {
        const auto& ci = schema.column(i);
        put<uint32_t>(file, static_cast<uint32_t>(ci.name.size()));
        file.write(ci.name.data(), ci.name.size());
        put<uint8_t>(file, static_cast<uint8_t>(ci.type));
    }
    put<uint32_t>(file, static_cast<uint32_t>(groups.size()));
    for (const auto& g : groups) {
        put<uint64_t>(file, g.num_rows);
        for (size_t c = 0; c < g.columns.size(); ++c) {
            const auto& cm = g.columns[c];
            put<uint64_t>(file, cm.offset);
            put<uint64_t>(file, cm.length);
            put<uint8_t>(file, cm.has_stats ? 1 : 0);
            if (cm.has_stats) {
                if (schema.column(c).type == DataType::Float64) {
                    put<double>(file, cm.min_f);
                    put<double>(file, cm.max_f);
                } else {
                    put<int64_t>(file, cm.min_i);
                    put<int64_t>(file, cm.max_i);
                }
            }
        }
    }

    uint64_t footer_len = static_cast<uint64_t>(file.tellp()) - footer_start;
    put<uint64_t>(file, footer_len);
    file.write(MAGIC_V3, 4);

    std::cout << "Converted " << total_rows << " rows, "
              << groups.size() << " row group(s) -> " << out << "\n";
}

StoreReader::StoreReader(const std::string& path) : file_(path, std::ios::binary) {
    if (!file_) throw std::runtime_error("Cannot open store: " + path);

    char magic[4];
    file_.read(magic, 4);
    bool is_v3 = (std::memcmp(magic, MAGIC_V3, 4) == 0);
    bool is_v2 = (std::memcmp(magic, MAGIC_V2, 4) == 0);
    if (!is_v3 && !is_v2)
        throw std::runtime_error("Bad magic (not a BCE2/BCE3 file): " + path);

    file_.seekg(0, std::ios::end);
    uint64_t file_size = static_cast<uint64_t>(file_.tellg());

    file_.seekg(file_size - 4, std::ios::beg);
    file_.read(magic, 4);
    if (std::memcmp(magic, MAGIC_V3, 4) != 0 && std::memcmp(magic, MAGIC_V2, 4) != 0)
        throw std::runtime_error("Bad trailing magic: " + path);

    file_.seekg(file_size - 12, std::ios::beg);
    uint64_t footer_len = get<uint64_t>(file_);

    file_.seekg(file_size - 12 - footer_len, std::ios::beg);

    uint32_t ncols = get<uint32_t>(file_);
    for (uint32_t i = 0; i < ncols; ++i) {
        uint32_t nl = get<uint32_t>(file_);
        std::string name(nl, '\0');
        file_.read(name.data(), nl);
        uint8_t t = get<uint8_t>(file_);
        schema_.add_column(name, static_cast<DataType>(t));
    }

    uint32_t ngroups = get<uint32_t>(file_);
    groups_.resize(ngroups);
    for (uint32_t g = 0; g < ngroups; ++g) {
        groups_[g].num_rows = get<uint64_t>(file_);
        groups_[g].columns.resize(ncols);
        for (uint32_t c = 0; c < ncols; ++c) {
            ChunkMeta cm;
            cm.offset = get<uint64_t>(file_);
            cm.length = get<uint64_t>(file_);
            cm.has_stats = get<uint8_t>(file_) != 0;
            if (cm.has_stats) {
                if (schema_.column(c).type == DataType::Float64) {
                    cm.min_f = get<double>(file_);
                    cm.max_f = get<double>(file_);
                } else {
                    cm.min_i = get<int64_t>(file_);
                    cm.max_i = get<int64_t>(file_);
                }
            }
            groups_[g].columns[c] = cm;
        }
    }
}

size_t StoreReader::column_index(const std::string& name) const {
    for (size_t i = 0; i < schema_.column_count(); ++i)
        if (schema_.column(i).name == name) return i;
    throw std::runtime_error("No such column: " + name);
}

std::shared_ptr<Column> StoreReader::read_chunk(size_t rg, size_t col) {
    const ChunkMeta& cm = groups_[rg].columns[col];
    file_.seekg(cm.offset, std::ios::beg);
    return decode_chunk(file_, schema_.column(col).type, groups_[rg].num_rows);
}

}
