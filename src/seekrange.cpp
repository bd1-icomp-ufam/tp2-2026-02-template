#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <vector>
#include "blockio.hpp"
#include "hash_file.hpp"
#include "bplustree.hpp"
#include "storage.hpp"

int main(int argc, char** argv) {
    if (argc < 3) { std::fprintf(stderr, "uso: seekrange <ID-inicial> <ID-final>\n"); return 2; }
    const int ini = std::atoi(argv[1]);
    const int fim = std::atoi(argv[2]);
    const std::string data_path = "/data/db/artigos.dat";
    const std::string idx_path  = "/data/db/idx_id.btree";
    try {
        HashFile hf;
        hf.open(data_path);
        hf.reset_stats();

        BPlusTree idx;
        idx.open(idx_path);
        idx.reset_stats();

        std::vector<std::pair<std::int32_t, std::uint64_t>> res;
        idx.range(ini, fim, res);
        std::sort(res.begin(), res.end());

        std::vector<Article> out;
        for (const auto& p : res) {
            const std::uint64_t off = p.second;
            const std::uint64_t block_idx = off / blockio::kBlockSize;
            const std::uint64_t in_block  = off % blockio::kBlockSize;
            std::vector<char> buf(blockio::kBlockSize);
            hf.data_file().read_block(block_idx, buf.data());
            Article r;
            if (deserialize_article(buf.data() + in_block, r)) out.push_back(r);
        }

        std::printf("Arquivo de dados: %s\n", data_path.c_str());
        std::printf("Arquivo de índice: %s\n", idx_path.c_str());
        std::printf("Intervalo: [%d, %d]\n", ini, fim);
        std::printf("Registros encontrados: %zu\n", out.size());
        for (const auto& r : out) {
            std::printf("---\n");
            std::printf("ID: %d\n", r.id);
            std::printf("Título: %s\n", r.titulo.c_str());
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
