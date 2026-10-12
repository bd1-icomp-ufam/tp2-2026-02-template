#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <vector>
#include "blockio.hpp"
#include "hash_file.hpp"

int main(int argc, char** argv) {
    if (argc < 3) { std::fprintf(stderr, "uso: findrange <ID-inicial> <ID-final>\n"); return 2; }
    const int ini = std::atoi(argv[1]);
    const int fim = std::atoi(argv[2]);
    const std::string data_path = "/data/db/artigos.dat";
    try {
        HashFile hf;
        hf.open(data_path);
        hf.reset_stats();
        std::vector<Article> out;
        hf.scan_by_range(ini, fim, out);
        std::sort(out.begin(), out.end(),
                  [](const Article& a, const Article& b){ return a.id < b.id; });

        std::printf("Arquivo de dados: %s\n", data_path.c_str());
        std::printf("Intervalo: [%d, %d]\n", ini, fim);
        std::printf("Registros encontrados: %zu\n", out.size());
        for (const auto& r : out) {
            std::printf("---\n");
            std::printf("ID: %d\n", r.id);
            std::printf("Título: %s\n", r.titulo.c_str());
            std::printf("Ano: %d\n", r.ano);
        }
        std::printf("Blocos lidos: %llu\n", (unsigned long long)hf.stats().reads);
        std::printf("Total de blocos do arquivo: %llu\n",
                    (unsigned long long)hf.data_block_count());
    } catch (const std::exception& e) {
        std::fprintf(stderr, "erro: %s\n", e.what());
        return 1;
    }
    return 0;
}
