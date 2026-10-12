#include <cstdio>
#include <cstring>
#include <string>

#include "blockio.hpp"
#include "csv_reader.hpp"
#include "hash_file.hpp"
#include "bplustree.hpp"

// Chave compacta para o índice secundário: 4 primeiros bytes do título.
// Colisões são tratadas no seek2 comparando o título completo.
static std::int32_t title_key(const std::string& t) {
    std::int32_t k = 0;
    if (t.size() >= 4) std::memcpy(&k, t.data(), 4);
    else for (std::size_t i = 0; i < t.size(); ++i)
        k |= (static_cast<std::int32_t>(t[i]) << (8 * i));
    return k;
}

int main(int argc, char** argv) {
    if (argc < 2) { std::fprintf(stderr, "uso: %s <arquivo.csv>\n", argv[0]); return 2; }
    const std::string csv_path = argv[1];
    const std::string data_path       = "/data/db/artigos.dat";
    const std::string idx_id_path     = "/data/db/idx_id.btree";
    const std::string idx_titulo_path = "/data/db/idx_titulo.btree";

    try {
        CsvReader reader(csv_path);
        HashFile hf;
        hf.create(data_path, 2048);   // 2048 buckets; ver 7.1

        BPlusTree idx_id, idx_titulo;
        idx_id.create(idx_id_path);
        idx_titulo.create(idx_titulo_path);

        Article a;
        std::uint64_t n = 0;
        while (reader.next(a)) {
            const std::uint64_t off = hf.insert(a);
            idx_id.insert(a.id, off);
            idx_titulo.insert(title_key(a.titulo), off);
            ++n;
            if (n % 20000 == 0)
                std::fprintf(stderr, "  ... %llu registros\n",
                             static_cast<unsigned long long>(n));
        }

        std::printf("=== Upload concluído ===\n");
        std::printf("Registros carregados: %llu\n", (unsigned long long)n);
        std::printf("Linhas ignoradas: %llu\n", (unsigned long long)reader.skipped());
        std::printf("Tamanho do bloco: %zu bytes\n", blockio::kBlockSize);
        std::printf("Registros por bloco: %u\n", hf.records_per_block());
        std::printf("Blocos do arquivo de dados: %llu\n",
                    (unsigned long long)hf.data_block_count());
        std::printf("Taxa de ocupação: %.2f%%\n", hf.occupancy() * 100.0);
        std::printf("Blocos do índice primário: %llu\n",
                    (unsigned long long)idx_id.block_count());
        std::printf("Ordem B+Tree (max chaves/nó): %u\n", idx_id.order());
        std::printf("Altura B+Tree ID: %u\n", idx_id.height());
        std::printf("Blocos do índice secundário: %llu\n",
                    (unsigned long long)idx_titulo.block_count());
        std::printf("Altura B+Tree Título: %u\n", idx_titulo.height());
    } catch (const std::exception& e) {
        std::fprintf(stderr, "erro: %s\n", e.what());
        return 1;
    }
    return 0;
}
