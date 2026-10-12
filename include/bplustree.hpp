#ifndef BPLUSTREE_HPP
#define BPLUSTREE_HPP

#include "blockio.hpp"
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

// =====================================================================
// Nó B+Tree — calculado a partir de BLOCK_SIZE:
//   cabeçalho: 5 × uint32 = 20 bytes
//   por chave: 4 (int32) + 8 (uint64 filho) = 12 bytes
//   MAX_KEYS = (BLOCK_SIZE − 20) / 12 − 1
//   Para 4096 B → 339; para 8192 B → 679.
// =====================================================================
constexpr std::uint32_t BTREE_HEADER  = 20;
constexpr std::uint32_t BTREE_MAX_KEYS =
    (static_cast<std::uint32_t>(blockio::kBlockSize) - BTREE_HEADER) / 12 - 1;

struct BTreeNode {
    std::uint32_t is_leaf;
    std::uint32_t num_keys;
    std::uint32_t parent;
    std::uint32_t next_leaf;   // só usado em folhas
    std::uint32_t reserved;
    std::int32_t  keys[BTREE_MAX_KEYS];
    std::uint64_t children[BTREE_MAX_KEYS + 1];   // folha: offset do registro
                                                  // interno: índice de bloco
};

class BPlusTree {
public:
    void create(const std::string& path);
    void open(const std::string& path);

    void insert(std::int32_t key, std::uint64_t value);
    bool find(std::int32_t key, std::vector<std::uint64_t>& values);
    void range(std::int32_t ini, std::int32_t fim,
               std::vector<std::pair<std::int32_t, std::uint64_t>>& out);

    std::uint64_t block_count() { return file_.block_count(); }
    const blockio::Stats& stats() const { return file_.stats(); }
    void reset_stats() { file_.reset_stats(); }
    std::uint32_t height();
    std::uint32_t root() const { return root_; }
    std::uint32_t order() const { return BTREE_MAX_KEYS; }
    blockio::BlockFile& file() { return file_; }

private:
    blockio::BlockFile file_;
    std::uint32_t root_ = 0xFFFFFFFFu;
    std::uint32_t next_free_ = 1;

    void read_node(std::uint32_t idx, BTreeNode& n);
    void write_node(std::uint32_t idx, const BTreeNode& n);
    std::uint32_t alloc_node();
    std::uint32_t find_leaf(std::int32_t key);
    void split_leaf(std::uint32_t idx, BTreeNode& n);
    void split_internal(std::uint32_t idx, BTreeNode& n);
    void insert_into_parent(std::uint32_t left, std::int32_t key, std::uint32_t right);
    void update_header();
};

#endif