#include "bplustree.hpp"
#include <cstring>
#include <vector>

static constexpr std::uint32_t NONE = 0xFFFFFFFFu;

void BPlusTree::create(const std::string& path) {
    file_.open(path, blockio::Mode::kTruncate);
    std::vector<char> buf(blockio::kBlockSize, 0);
    const std::uint32_t r = NONE, nf = 1;
    std::memcpy(buf.data(), &r, 4);
    std::memcpy(buf.data() + 4, &nf, 4);
    file_.write_block(0, buf.data());

    BTreeNode leaf{};
    leaf.is_leaf = 1; leaf.num_keys = 0; leaf.parent = NONE; leaf.next_leaf = NONE;
    root_ = alloc_node();
    write_node(root_, leaf);
    update_header();
}

void BPlusTree::open(const std::string& path) {
    file_.open(path, blockio::Mode::kReadWrite);
    std::vector<char> buf(blockio::kBlockSize);
    file_.read_block(0, buf.data());
    std::memcpy(&root_, buf.data(), 4);
    std::memcpy(&next_free_, buf.data() + 4, 4);
}

void BPlusTree::update_header() {
    std::vector<char> buf(blockio::kBlockSize);
    file_.read_block(0, buf.data());
    std::memcpy(buf.data(), &root_, 4);
    std::memcpy(buf.data() + 4, &next_free_, 4);
    file_.write_block(0, buf.data());
}

std::uint32_t BPlusTree::alloc_node() {
    const std::uint32_t idx = next_free_++;
    std::vector<char> z(blockio::kBlockSize, 0);
    file_.write_block(idx, z.data());
    update_header();
    return idx;
}

void BPlusTree::read_node(std::uint32_t idx, BTreeNode& n) {
    std::vector<char> buf(blockio::kBlockSize);
    file_.read_block(idx, buf.data());
    std::memcpy(&n, buf.data(), sizeof(BTreeNode));
}

void BPlusTree::write_node(std::uint32_t idx, const BTreeNode& n) {
    std::vector<char> buf(blockio::kBlockSize, 0);
    std::memcpy(buf.data(), &n, sizeof(BTreeNode));
    file_.write_block(idx, buf.data());
}

std::uint32_t BPlusTree::find_leaf(std::int32_t key) {
    std::uint32_t cur = root_;
    while (true) {
        BTreeNode n; read_node(cur, n);
        if (n.is_leaf) return cur;
        int i = 0;
        while (i < static_cast<int>(n.num_keys) && key >= n.keys[i]) ++i;
        cur = static_cast<std::uint32_t>(n.children[i]);
    }
}

void BPlusTree::insert(std::int32_t key, std::uint64_t value) {
    const std::uint32_t leaf_idx = find_leaf(key);
    BTreeNode leaf; read_node(leaf_idx, leaf);

    int pos = 0;
    while (pos < static_cast<int>(leaf.num_keys) && leaf.keys[pos] < key) ++pos;
    // Chaves duplicadas: insere ao lado (mesmo nó ou desloca)
    for (int i = static_cast<int>(leaf.num_keys); i > pos; --i) {
        leaf.keys[i] = leaf.keys[i-1];
        leaf.children[i] = leaf.children[i-1];
    }
    leaf.keys[pos] = key;
    leaf.children[pos] = value;
    leaf.num_keys++;

    if (leaf.num_keys > BTREE_MAX_KEYS) split_leaf(leaf_idx, leaf);
    else write_node(leaf_idx, leaf);
}

void BPlusTree::split_leaf(std::uint32_t node_idx, BTreeNode& n) {
    const std::uint32_t mid = n.num_keys / 2;
    BTreeNode right{};
    right.is_leaf = 1;
    right.num_keys = n.num_keys - mid;
    right.parent = n.parent;
    right.next_leaf = n.next_leaf;
    for (std::uint32_t i = 0; i < right.num_keys; ++i) {
        right.keys[i] = n.keys[mid + i];
        right.children[i] = n.children[mid + i];
    }
    n.num_keys = mid;
    const std::uint32_t right_idx = alloc_node();
    n.next_leaf = right_idx;
    write_node(node_idx, n);
    write_node(right_idx, right);

    const std::int32_t up_key = right.keys[0];
    if (n.parent == NONE) {
        BTreeNode root{};
        root.is_leaf = 0; root.num_keys = 1; root.parent = NONE;
        root.keys[0] = up_key;
        root.children[0] = node_idx;
        root.children[1] = right_idx;
        const std::uint32_t root_idx = alloc_node();
        write_node(root_idx, root);
        n.parent = root_idx; right.parent = root_idx;
        write_node(node_idx, n); write_node(right_idx, right);
        root_ = root_idx;
        update_header();
    } else {
        insert_into_parent(node_idx, up_key, right_idx);
    }
}

void BPlusTree::insert_into_parent(std::uint32_t left, std::int32_t key,
                                   std::uint32_t right) {
    BTreeNode l; read_node(left, l);
    const std::uint32_t parent_idx = l.parent;
    if (parent_idx == NONE) return;
    BTreeNode p; read_node(parent_idx, p);

    int pos = 0;
    while (pos < static_cast<int>(p.num_keys) && p.keys[pos] < key) ++pos;
    for (int i = static_cast<int>(p.num_keys); i > pos; --i) {
        p.keys[i] = p.keys[i-1];
        p.children[i+1] = p.children[i];
    }
    p.keys[pos] = key;
    p.children[pos+1] = right;
    p.num_keys++;
    BTreeNode r; read_node(right, r); r.parent = parent_idx; write_node(right, r);

    if (p.num_keys > BTREE_MAX_KEYS) split_internal(parent_idx, p);
    else write_node(parent_idx, p);
}

void BPlusTree::split_internal(std::uint32_t node_idx, BTreeNode& n) {
    const std::uint32_t mid = n.num_keys / 2;
    const std::int32_t up_key = n.keys[mid];
    BTreeNode right{};
    right.is_leaf = 0;
    right.num_keys = n.num_keys - mid - 1;
    right.parent = n.parent;
    for (std::uint32_t i = 0; i < right.num_keys; ++i) {
        right.keys[i] = n.keys[mid + 1 + i];
        right.children[i] = n.children[mid + 1 + i];
    }
    right.children[right.num_keys] = n.children[n.num_keys];
    n.num_keys = mid;
    const std::uint32_t right_idx = alloc_node();
    write_node(node_idx, n);
    write_node(right_idx, right);
    for (std::uint32_t i = 0; i <= right.num_keys; ++i) {
        BTreeNode c; read_node(static_cast<std::uint32_t>(right.children[i]), c);
        c.parent = right_idx;
        write_node(static_cast<std::uint32_t>(right.children[i]), c);
    }
    if (n.parent == NONE) {
        BTreeNode root{};
        root.is_leaf = 0; root.num_keys = 1; root.parent = NONE;
        root.keys[0] = up_key;
        root.children[0] = node_idx;
        root.children[1] = right_idx;
        const std::uint32_t root_idx = alloc_node();
        write_node(root_idx, root);
        n.parent = root_idx; right.parent = root_idx;
        write_node(node_idx, n); write_node(right_idx, right);
        root_ = root_idx;
        update_header();
    } else {
        insert_into_parent(node_idx, up_key, right_idx);
    }
}

bool BPlusTree::find(std::int32_t key, std::vector<std::uint64_t>& values) {
    const std::uint32_t leaf_idx = find_leaf(key);
    BTreeNode leaf; read_node(leaf_idx, leaf);
    bool found = false;
    for (std::uint32_t i = 0; i < leaf.num_keys; ++i)
        if (leaf.keys[i] == key) { values.push_back(leaf.children[i]); found = true; }

    // Chaves duplicadas podem estar em folhas seguintes
    std::uint32_t next = leaf.next_leaf;
    while (found && next != NONE) {
        BTreeNode nx; read_node(next, nx);
        bool any = false;
        for (std::uint32_t i = 0; i < nx.num_keys; ++i) {
            if (nx.keys[i] == key) { values.push_back(nx.children[i]); any = true; }
            else if (nx.keys[i] > key) return true;
        }
        if (!any) break;
        next = nx.next_leaf;
    }
    return found;
}

void BPlusTree::range(std::int32_t ini, std::int32_t fim,
                      std::vector<std::pair<std::int32_t, std::uint64_t>>& out) {
    const std::uint32_t leaf_idx = find_leaf(ini);
    BTreeNode leaf; read_node(leaf_idx, leaf);
    while (true) {
        for (std::uint32_t i = 0; i < leaf.num_keys; ++i) {
            if (leaf.keys[i] >= ini && leaf.keys[i] <= fim)
                out.emplace_back(leaf.keys[i], leaf.children[i]);
            else if (leaf.keys[i] > fim) return;
        }
        if (leaf.next_leaf == NONE) return;
        read_node(leaf.next_leaf, leaf);
    }
}

std::uint32_t BPlusTree::height() {
    if (root_ == NONE) return 0;
    std::uint32_t h = 1, cur = root_;
    while (true) {
        BTreeNode n; read_node(cur, n);
        if (n.is_leaf) return h;
        h++;
        cur = static_cast<std::uint32_t>(n.children[0]);
    }
}