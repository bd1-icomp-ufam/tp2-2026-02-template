#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include "blockio.hpp"
#include "hash_file.hpp"
#include "bplustree.hpp"
#include "storage.hpp"

static std::int32_t title_key(const std::string& t) {
    std::int32_t k = 0;
    if (t.size() >= 4) std::memcpy(&k, t.data(), 4);
    else for (std::size_t i = 0; i < t.size(); ++i)
        k |= (static_cast<std::int32_t>(t[i]) << (8 * i));
    return k;
}

int main(int argc, char** argv) {
    if (argc < 2) { std::fprintf(stderr, "uso: seek2 \"<Título>\"\n"); return 2; }
    const std::string titulo = argv[1];
    const std::string data_path = "/data/db/artigos.dat";
    const std::string idx_path  = "/data/db/idx_titulo.btree";
    try {
        HashFile hf;
        hf.open(data_path);
        hf.reset_stats();

        BPlusTree idx;
        idx.open(idx_path);
        idx.reset_stats();

        std::vector<std::uint64_t> vals;
        idx.find(title_key(titulo), vals);

        std::vector<Article> out;
        for (std::uint64_t off : vals) {
            const std::uint64_t block_idx = off / blockio::kBlockSize;
            const std::uint64_t in_block  = off % blockio::kBlockSize;
            std::vector<char> buf(blockio::kBlockSize);
            hf.data_file().read_block(block_idx, buf.data());
            Article r;
            if (deserialize_article(buf.data() + in_block, r) && r.titulo == titulo)
                out.push_back(r);
        }

        std::printf("Arquivo de dados: %s\n", data_path.c_str());
        std::printf("Arquivo de índice: %s\n", idx_path.c_str());
        std::printf("Registros encontrados: %zu\n", out.size());
        for (const auto& r : out) {
            std::printf("---\n");
            std::printf("ID: %d\n", r.id);
            std::printf("Título: %s\n", r.titulo.c_str());
            std::printf("Ano: %d\n", r.ano);
            std::printf("Autores: %s\n", r.autores.c_str());
        }
        std::printf("Blocos lidos no índice: %llu\n",
                    (unsigned long long)idx.stats().reads);
        std::printf("Blocos lidos no arquivo de dados: %llu\n",
                    (unsigned long long)hf.stats().reads);
        std::printf("Total de blocos do índice: %llu\n",
                    (unsigned long long)idx.block_count());
    } catch (const std::exception& e) {
        std::fprintf(stderr, "erro: %s\n", e.what());
        return 1;
    }
    return 0;
}
