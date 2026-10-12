#ifndef HASH_FILE_HPP
#define HASH_FILE_HPP
#include "storage.hpp"

#include "blockio.hpp"
#include "record.hpp"
#include <cstdint>
#include <string>
#include <vector>

// Arquivo de dados organizado por hashing estático (bucket = bloco).
// Bloco 0: header global [num_buckets][total_registros][reservado]
// Bloco (1+b): bucket b — [num_rec][next_overflow][reservado]
class HashFile {
public:
    void create(const std::string& path, std::uint32_t num_buckets);
    void open(const std::string& path);

    // Insere; devolve offset absoluto do registro dentro do arquivo (bytes).
    std::uint64_t insert(const Article& a);

    bool find_by_id(int id, Article& out);

    void scan_all(std::vector<Article>& out);
    void scan_by_title(const std::string& titulo, std::vector<Article>& out);
    void scan_by_range(int ini, int fim, std::vector<Article>& out);

    std::uint32_t num_buckets() const { return num_buckets_; }
    std::uint64_t data_block_count() { return data_.block_count(); }
    std::uint32_t records_per_block() const { return SLOTS_PER_BLOCK; }
    std::uint64_t total_records() const { return total_records_; }
    double occupancy();

    blockio::BlockFile& data_file() { return data_; }
    const blockio::Stats& stats() const { return data_.stats(); }
    void reset_stats() { data_.reset_stats(); }

private:
    blockio::BlockFile data_;
    std::uint32_t num_buckets_ = 0;
    std::uint64_t total_records_ = 0;

    std::uint32_t bucket_of(int id) const;
};

#endif