#ifndef STORAGE_HPP
#define STORAGE_HPP

#include "blockio.hpp"
#include "record.hpp"
#include <cstdint>
#include <cstring>
#include <string>

// =====================================================================
// Layout do registro em disco (tamanho fixo — justificativa na seção 7.1)
// =====================================================================
//  Offset  Campo           Bytes
//  ------  --------------  -----
//       0  id                  4
//       4  ano                 4
//       8  citacoes            4
//      12  len_titulo          4
//      16  len_autores         4
//      20  len_atualizacao     4
//      24  len_snippet         4
//      28  titulo            300  (padding com \0)
//     328  autores           150
//     478  atualizacao        20
//     498  snippet          1024
//  ------  --------------  -----
//  Total: 1522 bytes por registro.
//
// Justificativa: campos com tamanho fixo máximo (com padding) permitem
// acesso aleatório dentro do bloco sem ler o registro inteiro. O
// truncamento em título/autores/snippet é aceitável porque os limites
// (300/150/1024) são os máximos declarados no enunciado.

constexpr std::uint32_t MAX_TITULO      = 300;
constexpr std::uint32_t MAX_AUTORES     = 150;
constexpr std::uint32_t MAX_ATUALIZACAO = 20;   // "AAAA-MM-DD HH:MM:SS"
constexpr std::uint32_t MAX_SNIPPET     = 1024;
constexpr std::uint32_t RECORD_HEADER   = 7 * 4; // 28
constexpr std::uint32_t RECORD_SIZE     = RECORD_HEADER + MAX_TITULO + MAX_AUTORES
                                        + MAX_ATUALIZACAO + MAX_SNIPPET;

// Cabeçalho do bloco de dados:
//  [0] num_registros_no_bloco  (4)
//  [4] proximo_bloco_overflow  (4) — 0xFFFFFFFF = nenhum
//  [8] reservado               (8)
constexpr std::uint32_t DATA_HEADER_SIZE = 16;
constexpr std::uint32_t SLOTS_PER_BLOCK  =
    (static_cast<std::uint32_t>(blockio::kBlockSize) - DATA_HEADER_SIZE) / RECORD_SIZE;
constexpr std::uint32_t NO_OVERFLOW      = 0xFFFFFFFFu;

// ---- Serialização de Article para o bloco ----
inline void serialize_article(const Article& a, char* out) {
    char* p = out;
    std::int32_t id = a.id, ano = a.ano, cit = a.citacoes;
    std::memcpy(p, &id, 4);  p += 4;
    std::memcpy(p, &ano, 4); p += 4;
    std::memcpy(p, &cit, 4); p += 4;

    const std::uint32_t lt = static_cast<std::uint32_t>(
        a.titulo.size()      > MAX_TITULO      ? MAX_TITULO      : a.titulo.size());
    const std::uint32_t la = static_cast<std::uint32_t>(
        a.autores.size()     > MAX_AUTORES     ? MAX_AUTORES     : a.autores.size());
    const std::uint32_t lu = static_cast<std::uint32_t>(
        a.atualizacao.size() > MAX_ATUALIZACAO ? MAX_ATUALIZACAO : a.atualizacao.size());
    const std::uint32_t ls = static_cast<std::uint32_t>(
        a.snippet.size()     > MAX_SNIPPET     ? MAX_SNIPPET     : a.snippet.size());

    std::memcpy(p, &lt, 4); p += 4;
    std::memcpy(p, &la, 4); p += 4;
    std::memcpy(p, &lu, 4); p += 4;
    std::memcpy(p, &ls, 4); p += 4;

    std::memset(p, 0, MAX_TITULO);       std::memcpy(p, a.titulo.data(), lt);      p += MAX_TITULO;
    std::memset(p, 0, MAX_AUTORES);      std::memcpy(p, a.autores.data(), la);     p += MAX_AUTORES;
    std::memset(p, 0, MAX_ATUALIZACAO);  std::memcpy(p, a.atualizacao.data(), lu); p += MAX_ATUALIZACAO;
    std::memset(p, 0, MAX_SNIPPET);      std::memcpy(p, a.snippet.data(), ls);     p += MAX_SNIPPET;
}

inline bool deserialize_article(const char* in, Article& a) {
    const char* p = in;
    std::int32_t id, ano, cit;
    std::uint32_t lt, la, lu, ls;
    std::memcpy(&id,  p, 4); p += 4;
    std::memcpy(&ano, p, 4); p += 4;
    std::memcpy(&cit, p, 4); p += 4;
    std::memcpy(&lt,  p, 4); p += 4;
    std::memcpy(&la,  p, 4); p += 4;
    std::memcpy(&lu,  p, 4); p += 4;
    std::memcpy(&ls,  p, 4); p += 4;
    if (lt > MAX_TITULO || la > MAX_AUTORES ||
        lu > MAX_ATUALIZACAO || ls > MAX_SNIPPET) return false;
    a.id = id; a.ano = ano; a.citacoes = cit;
    a.titulo.assign(p, lt);       p += MAX_TITULO;
    a.autores.assign(p, la);      p += MAX_AUTORES;
    a.atualizacao.assign(p, lu);  p += MAX_ATUALIZACAO;
    a.snippet.assign(p, ls);      p += MAX_SNIPPET;
    return true;
}

#endif  // STORAGE_HPP