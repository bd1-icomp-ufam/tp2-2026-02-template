#include <cstdio>
#include <string>
#include <vector>
#include "blockio.hpp"
#include "hash_file.hpp"

int main(int argc, char** argv) {
    if (argc < 2) { std::fprintf(stderr, "uso: findtitle \"<Título>\"\n"); return 2; }
    const std::string titulo = argv[1];
    const std::string data_path = "/data/db/artigos.dat";
    try {
        HashFile hf;
        hf.open(data_path);
        hf.reset_stats();
        std::vector<Article> out;
        hf.scan_by_title(titulo, out);

        std::printf("Arquivo de dados: %s\n", data_path.c_str());
        std::printf("Registros encontrados: %zu\n", out.size());
        for (const auto& r : out) {
            std::printf("---\n");
            std::printf("ID: %d\n", r.id);
            std::printf("Título: %s\n", r.titulo.c_str());
            std::printf("Ano: %d\n", r.ano);
            std::printf("Autores: %s\n", r.autores.c_str());
            std::printf("Citações: %d\n", r.citacoes);
            std::printf("Atualização: %s\n", r.atualizacao.c_str());
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
