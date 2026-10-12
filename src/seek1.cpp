#include <cstdio>
#include <cstdlib>
#include <vector>
#include "blockio.hpp"
#include "hash_file.hpp"
#include "bplustree.hpp"
#include "storage.hpp"

int main(int argc, char** argv) {
    if (argc < 2) { std::fprintf(stderr, "uso: seek1 <ID>\n"); return 2; }
    const int id = std::atoi(argv[1]);
    const std::string data_path = "/data/db/artigos.dat";
    const std::string idx_path  = "/data/db/idx_id.btree";
    try {
        HashFile hf;
        hf.open(data_path);
        hf.reset_stats();

        BPlusTree idx;
        idx.open(idx_path);
        idx.reset_stats();

        std::vector<std::uint64_t> vals;
        const bool found = idx.find(id, vals);

        Article r;
        bool rok = false;
        if (found && !vals.empty()) {
            const std::uint64_t off = vals[0];
            const std::uint64_t block_idx = off / blockio::kBlockSize;
            const std::uint64_t in_block  = off % blockio::kBlockSize;
            std::vector<char> buf(blockio::kBlockSize);
            hf.data_file().read_block(block_idx, buf.data());
            rok = deserialize_article(buf.data() + in_block, r);
        }

        std::printf("Arquivo de dados: %s\n", data_path.c_str());
        std::printf("Arquivo de índice: %s\n", idx_path.c_str());
        if (rok) {
            std::printf("ID: %d\n", r.id);
            std::printf("Título: %s\n", r.titulo.c_str());
            std::printf("Ano: %d\n", r.ano);
            std::printf("Autores: %s\n", r.autores.c_str());
            std::printf("Citações: %d\n", r.citacoes);
            std::printf("Atualização: %s\n", r.atualizacao.c_str());
        } else {
            std::printf("Registro com ID %d não encontrado.\n", id);
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
