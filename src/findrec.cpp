#include <cstdio>
#include <cstdlib>
#include "blockio.hpp"
#include "hash_file.hpp"

int main(int argc, char** argv) {
    if (argc < 2) { std::fprintf(stderr, "uso: findrec <ID>\n"); return 2; }
    const int id = std::atoi(argv[1]);
    const std::string data_path = "/data/db/artigos.dat";
    try {
        HashFile hf;
        hf.open(data_path);
        hf.reset_stats();
        Article r;
        const bool ok = hf.find_by_id(id, r);

        std::printf("Arquivo de dados: %s\n", data_path.c_str());
        if (ok) {
            std::printf("ID: %d\n", r.id);
            std::printf("Título: %s\n", r.titulo.c_str());
            std::printf("Ano: %d\n", r.ano);
            std::printf("Autores: %s\n", r.autores.c_str());
            std::printf("Citações: %d\n", r.citacoes);
            std::printf("Atualização: %s\n", r.atualizacao.c_str());
            std::printf("Snippet: %s\n", r.snippet.c_str());
        } else {
            std::printf("Registro com ID %d não encontrado.\n", id);
        }
        std::printf("Blocos lidos: %llu\n",
                    (unsigned long long)hf.stats().reads);
        std::printf("Total de blocos do arquivo: %llu\n",
                    (unsigned long long)hf.data_block_count());
    } catch (const std::exception& e) {
        std::fprintf(stderr, "erro: %s\n", e.what());
        return 1;
    }
    return 0;
}
