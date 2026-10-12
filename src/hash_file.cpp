#include "hash_file.hpp"
#include "storage.hpp"
#include <cstring>
#include <vector>

// ---------------------------------------------------------------------
// Hashing: mistura multiplicativa (splitmix-like) módulo num_buckets.
// Escolhida por distribuir bem IDs sequenciais sem pré-análise.
// ---------------------------------------------------------------------
std::uint32_t HashFile::bucket_of(int id) const {
    std::uint32_t x = static_cast<std::uint32_t>(id);
    x = ((x >> 16) ^ x) * 0x45d9f3bu;
    x = ((x >> 16) ^ x) * 0x45d9f3bu;
    x = (x >> 16) ^ x;
    return x % num_buckets_;
}

void HashFile::create(const std::string& path, std::uint32_t num_buckets) {
    data_.open(path, blockio::Mode::kTruncate);
    num_buckets_ = num_buckets;
    total_records_ = 0;

    std::vector<char> h(blockio::kBlockSize, 0);
    std::memcpy(h.data(), &num_buckets_, 4);
    std::memcpy(h.data() + 4, &total_records_, 8);
    data_.write_block(0, h.data());

    for (std::uint32_t i = 0; i < num_buckets; ++i) {
        std::vector<char> z(blockio::kBlockSize, 0);
        const std::uint32_t zero = 0, no = NO_OVERFLOW;
        std::memcpy(z.data(), &zero, 4);
        std::memcpy(z.data() + 4, &no, 4);
        data_.write_block(1 + i, z.data());
    }
}

void HashFile::open(const std::string& path) {
    data_.open(path, blockio::Mode::kReadWrite);
    std::vector<char> h(blockio::kBlockSize);
    data_.read_block(0, h.data());
    std::memcpy(&num_buckets_, h.data(), 4);
    std::memcpy(&total_records_, h.data() + 4, 8);
}

std::uint64_t HashFile::insert(const Article& a) {
    const std::uint32_t b = bucket_of(a.id);
    std::uint64_t block_idx = 1 + b;
    std::vector<char> buf(blockio::kBlockSize);

    while (true) {
        data_.read_block(block_idx, buf.data());
        std::uint32_t num_rec = 0, next_ovf = 0;
        std::memcpy(&num_rec, buf.data(), 4);
        std::memcpy(&next_ovf, buf.data() + 4, 4);

        if (num_rec < SLOTS_PER_BLOCK) {
            const std::uint32_t slot_off = DATA_HEADER_SIZE + num_rec * RECORD_SIZE;
            serialize_article(a, buf.data() + slot_off);
            num_rec++;
            std::memcpy(buf.data(), &num_rec, 4);
            data_.write_block(block_idx, buf.data());
            total_records_++;
            return block_idx * blockio::kBlockSize + slot_off;
        }

        if (next_ovf == NO_OVERFLOW) {
            // Overflow: aloca novo bloco e liga ao anterior
            const std::uint64_t nb = data_.alloc_block();
            std::vector<char> z(blockio::kBlockSize, 0);
            const std::uint32_t zero = 0, no = NO_OVERFLOW;
            std::memcpy(z.data(), &zero, 4);
            std::memcpy(z.data() + 4, &no, 4);
            data_.write_block(nb, z.data());

            next_ovf = static_cast<std::uint32_t>(nb);
            std::memcpy(buf.data() + 4, &next_ovf, 4);
            data_.write_block(block_idx, buf.data());
            block_idx = nb;
            continue;
        }
        block_idx = next_ovf;
    }
}

bool HashFile::find_by_id(int id, Article& out) {
    const std::uint32_t b = bucket_of(id);
    std::uint64_t block_idx = 1 + b;
    std::vector<char> buf(blockio::kBlockSize);

    while (block_idx != NO_OVERFLOW) {
        data_.read_block(block_idx, buf.data());
        std::uint32_t num_rec = 0, next_ovf = 0;
        std::memcpy(&num_rec, buf.data(), 4);
        std::memcpy(&next_ovf, buf.data() + 4, 4);
        for (std::uint32_t i = 0; i < num_rec; ++i) {
            const char* slot = buf.data() + DATA_HEADER_SIZE + i * RECORD_SIZE;
            Article r;
            if (deserialize_article(slot, r) && r.id == id) { out = r; return true; }
        }
        block_idx = next_ovf;
    }
    return false;
}

void HashFile::scan_all(std::vector<Article>& out) {
    const std::uint64_t n = data_.block_count();
    std::vector<char> buf(blockio::kBlockSize);
    for (std::uint64_t i = 1; i < n; ++i) {
        data_.read_block(i, buf.data());
        std::uint32_t num_rec = 0;
        std::memcpy(&num_rec, buf.data(), 4);
        if (num_rec == 0) continue;
        for (std::uint32_t j = 0; j < num_rec; ++j) {
            const char* slot = buf.data() + DATA_HEADER_SIZE + j * RECORD_SIZE;
            Article r;
            if (deserialize_article(slot, r)) out.push_back(r);
        }
    }
}

void HashFile::scan_by_title(const std::string& titulo, std::vector<Article>& out) {
    std::vector<Article> all;
    scan_all(all);
    for (auto& r : all) if (r.titulo == titulo) out.push_back(r);
}

void HashFile::scan_by_range(int ini, int fim, std::vector<Article>& out) {
    std::vector<Article> all;
    scan_all(all);
    for (auto& r : all) if (r.id >= ini && r.id <= fim) out.push_back(r);
}

double HashFile::occupancy() {
    const std::uint64_t n = data_.block_count();
    if (n <= 1) return 0.0;
    std::uint64_t used = 0, total = 0;
    std::vector<char> buf(blockio::kBlockSize);
    for (std::uint64_t i = 1; i < n; ++i) {
        data_.read_block(i, buf.data());
        std::uint32_t num_rec = 0;
        std::memcpy(&num_rec, buf.data(), 4);
        used += num_rec;
        total += SLOTS_PER_BLOCK;
    }
    return total ? static_cast<double>(used) / total : 0.0;
}