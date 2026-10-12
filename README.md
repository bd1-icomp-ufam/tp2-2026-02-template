# TP2 — Introdução a Bancos de Dados (ICC205 — 2026/02)

Trabalho Prático II: armazenamento e consulta de registros de artigos
científicos em memória secundária. O sistema é composto por duas partes:

- **Parte 1:** arquivo de dados paginado organizado por *hashing* estático.
- **Parte 2:** índices primário (ID) e secundário (Título) em B+Tree,
  armazenados em memória secundária, construídos sobre o mesmo arquivo
  de dados.

A métrica do trabalho é a **contagem de blocos lógicos** lidos, medida
pela camada `blockio` fornecida pelo professor. A comparação entre os
caminhos de acesso das duas partes está no relatório (`docs/tp2_relatorio.pdf`).


## 1. Estrutura do repositório

```
tp2/
├── src/            código C++ (fontes dos 7 programas + estruturas)
├── include/        headers
├── bin/            binários gerados por `make build` (não versionado)
├── data/           CSV de entrada + arquivos gerados (runtime)
│   └── db/         artigos.dat, idx_id.btree, idx_titulo.btree
├── queries/        oficial.txt (consultas oficiais)
├── docs/           tp2_relatorio.pdf
├── tests/          smoke_blockio.cpp
├── Makefile        compilação local e alvos Docker
├── Dockerfile      imagem do contêiner
├── docker-compose.yml
├── ENUNCIADO.md    especificação do trabalho
└── README.md       este arquivo
```

### Programas disponíveis

| Programa            | Parte | O que faz                                             |
| ------------------- | ----- | ----------------------------------------------------- |
| `bin/upload`        | 1     | Carrega o CSV no arquivo de dados por hashing         |
| `bin/findrec`       | 1     | Busca por ID no arquivo de dados                      |
| `bin/findtitle`     | 1     | Busca por título — varredura sequencial               |
| `bin/findrange`     | 1     | Busca por intervalo de IDs — varredura sequencial     |
| `bin/seek1`         | 2     | Busca por ID via B+Tree primária                      |
| `bin/seek2`         | 2     | Busca por título via B+Tree secundária                |
| `bin/seekrange`     | 2     | Busca por intervalo via B+Tree primária + folhas      |


## 2. Requisitos

- **Compilar localmente:** `g++` (C++17) e `make` em Linux/Ubuntu.
- **Executar via Docker:** Docker e `docker-compose` (opcional).


## 3. Preparar o CSV de entrada

O CSV oficial (`artigo-amostra.csv`) **não é versionado** por ser grande
(~106 MB descompactado). Coloque-o em `data/` antes da primeira carga:

```bash
# Se você recebeu o arquivo comprimido:
gzip -dk data/artigo-amostra.csv.gz

# Confirme que existe:
ls -lh data/artigo-amostra.csv
```

O container monta `./data` em `/data`, e o `upload` lê o CSV de
`/data/artigo-amostra.csv`. Os arquivos gerados ficam em `/data/db/`.

---

## 4. Compilar

### Local

```bash
make clean
make build                     # compila com BLOCK_SIZE=4096 (default)
make test                      # roda o smoke test da camada blockio
```

Saída esperada do `make test`:

```
blockio ok (bloco de 4096 bytes)
```

Para compilar com bloco de 8 KB (Seção 7.5 do enunciado):

```bash
make clean
make BLOCK_SIZE=8192 build
```

Os binários ficam em `bin/`.

### Docker

```bash
make docker-build              # equivale a: docker build -t tp2 .
```

Se quiser a imagem com bloco de 8 KB:

```bash
make clean
docker build --build-arg BLOCK_SIZE=8192 -t tp2-8k .
```

> **Nota:** rode `make clean` **antes** do `docker build` sempre que
> trocar `BLOCK_SIZE`. Caso contrário, o `COPY . /app` do Dockerfile
> pode copiar binários antigos e o `make` dentro do contêiner não
> recompila.

---

## 5. Executar as consultas

### Local (sem Docker)

Se você compilou localmente, os binários esperam `/data/db/...`. Crie
um link ou ajuste os caminhos conforme sua instalação. O caminho
recomendado é o Docker (seção 5.2), que já faz o bind mount correto.

### Docker — comandos oficiais

```bash
# 1) Upload (carga inicial)
docker run --rm -v "$PWD/data:/data" tp2 ./bin/upload /data/artigo-amostra.csv

# 2) Consultas — Parte 1 (sem índice)
docker run --rm -v "$PWD/data:/data" tp2 ./bin/findrec  316639
docker run --rm -v "$PWD/data:/data" tp2 ./bin/findtitle "Review of particle physics particle data group"
docker run --rm -v "$PWD/data:/data" tp2 ./bin/findrange 1000 1100

# 3) Consultas — Parte 2 (com índice)
docker run --rm -v "$PWD/data:/data" tp2 ./bin/seek1     316639
docker run --rm -v "$PWD/data:/data" tp2 ./bin/seek2     "Review of particle physics particle data group"
docker run --rm -v "$PWD/data:/data" tp2 ./bin/seekrange 1000 1100
```

### Atalhos via Makefile

O `Makefile` também oferece alvos para conveniência:

```bash
make docker-run-upload      # roda o upload
make docker-run-findrec ARGS="316639"
make docker-run-seek1   ARGS="316639"
# ... idem para findtitle, findrange, seek2, seekrange
```

### Testar com BLOCK_SIZE=8192

```bash
make clean
docker build --build-arg BLOCK_SIZE=8192 -t tp2-8k .
docker run --rm -v "$PWD/data:/data" tp2-8k ./bin/upload /data/artigo-amostra.csv
docker run --rm -v "$PWD/data:/data" tp2-8k ./bin/findrec 316639
docker run --rm -v "$PWD/data:/data" tp2-8k ./bin/seek1   316639
```

---

## 6. Exemplo de entrada e saída

### Entrada (`entrada.csv`)

```
"1";"Poster: 3D sketching";"2013";"A. Leal|D. Bowman";"0";"2016-07-28 09:36:29";"Poster ..."
"2";"A rotating-magnet";   "2014";"X. Y";           "5";"2017-01-01 00:00:00";"Resumo ..."
```

### `upload`

```
$ docker run --rm -v "$PWD/data:/data" tp2 ./bin/upload /data/entrada.csv
=== Upload concluído ===
Registros carregados: 2
Linhas ignoradas: 0
Tamanho do bloco: 4096 bytes
Registros por bloco: 2
Blocos do arquivo de dados: 2050
Taxa de ocupação: 0.05%
Blocos do índice primário: 2
Ordem B+Tree (max chaves/nó): 338
Altura B+Tree ID: 1
Blocos do índice secundário: 2
Altura B+Tree Título: 1
```

### `findrec` (Parte 1)

```
$ docker run --rm -v "$PWD/data:/data" tp2 ./bin/findrec 1
Arquivo de dados: /data/db/artigos.dat
ID: 1
Título: Poster: 3D sketching
Ano: 2013
Autores: A. Leal|D. Bowman
Citações: 0
Atualização: 2016-07-28 09:36:29
Snippet: Poster ...
Blocos lidos: 1
Total de blocos do arquivo: 2050
```

### `seek1` (Parte 2)

```
$ docker run --rm -v "$PWD/data:/data" tp2 ./bin/seek1 1
Arquivo de dados: /data/db/artigos.dat
Arquivo de índice: /data/db/idx_id.btree
ID: 1
Título: Poster: 3D sketching
...
Blocos lidos no índice: 4
Blocos lidos no arquivo de dados: 1
Total de blocos do índice: 2
```

> O `seek1` foi mais caro que o `findrec` neste exemplo porque a B+Tree
> precisa descer até a folha antes de ler o dado. Isso é esperado e está
> discutido no relatório (Seção 7.4).

### Consulta que não encontra resultado

```
$ docker run --rm -v "$PWD/data:/data" tp2 ./bin/seek1 999999
Arquivo de dados: /data/db/artigos.dat
Arquivo de índice: /data/db/idx_id.btree
Registro com ID 999999 não encontrado.
Blocos lidos no índice: 4
Blocos lidos no arquivo de dados: 0
Total de blocos do índice: 2
```

---

## 7. Layout dos arquivos gerados em `/data/db/`

Após rodar o `upload`, três arquivos são criados em `data/db/`:

| Arquivo             | Conteúdo                                            | Tamanho em 4096 B |
| ------------------- | --------------------------------------------------- | ----------------- |
| `artigos.dat`       | Arquivo de dados paginado (hashing + overflow)      | ~400 MB           |
| `idx_id.btree`      | B+Tree primária sobre `id`                          | ~5 MB             |
| `idx_titulo.btree`  | B+Tree secundária sobre `titulo`                    | ~4 MB             |

Cada arquivo é uma sequência de blocos de `BLOCK_SIZE` bytes.

**Layout do bloco de dados:**

```
[0..3]   num_registros_no_bloco   uint32
[4..7]   proximo_bloco_overflow   uint32 (0xFFFFFFFF = nenhum)
[8..15]  reservado
[16..]   slots de registro (1522 bytes cada)
```

**Layout do registro** (1.522 bytes, tamanho fixo):

```
[0..3]     id
[4..7]     ano
[8..11]    citacoes
[12..15]   len_titulo
[16..19]   len_autores
[20..23]   len_atualizacao
[24..27]   len_snippet
[28..327]  titulo        (padding com \0)
[328..477] autores
[478..497] atualizacao
[498..1521] snippet
```

**Layout do nó da B+Tree:**

```
[0..3]     is_leaf
[4..7]     num_keys
[8..11]    parent
[12..15]   next_leaf (folhas)
[16..19]   reservado
[20..]     chaves int32 + filhos uint64
```

---

## 8. Medições

A métrica do trabalho é **blocos lógicos**, não tempo. Cada leitura
passa por `blockio::BlockFile::read_block`, que incrementa
`stats().reads`. O contador é determinístico e não depende de cache do
sistema operacional, disco ou máquina. Os números medidos estão no
relatório:

- **Seção 7.3:** consultas oficiais em 4.096 B e 8.192 B.
- **Seção 7.4:** comparação entre as duas partes.
- **Seção 7.5:** efeito do tamanho do bloco.
- **Seção 7.6:** casos patológicos (chave inexistente, título repetido, intervalo vazio).

Nenhum acesso direto aos arquivos (`fstream`, `mmap`, etc.) é usado nos
programas de consulta — todos passam por `blockio`. Acessos fora da
camada invalidariam a medição.

---

## 9. Recompilar e reproduzir

Sequência completa para reproduzir os números do relatório:

```bash
# 1. Compilar (4 KB)
make clean
make build
make test                       # smoke test

# 2. Docker
docker build -t tp2 .

# 3. Upload
docker run --rm -v "$PWD/data:/data" tp2 ./bin/upload /data/artigo-amostra.csv

# 4. Consultas (uma por vez ou em script)
docker run --rm -v "$PWD/data:/data" tp2 ./bin/findrec 5
docker run --rm -v "$PWD/data:/data" tp2 ./bin/seek1   5
# ...

# 5. Repetir com BLOCK_SIZE=8192
make clean
make BLOCK_SIZE=8192 build
docker build --build-arg BLOCK_SIZE=8192 -t tp2-8k .
docker run --rm -v "$PWD/data:/data" tp2-8k ./bin/upload /data/artigo-amostra.csv
docker run --rm -v "$PWD/data:/data" tp2-8k ./bin/findrec 5
docker run --rm -v "$PWD/data:/data" tp2-8k ./bin/seek1   5
```

---





## 12. Referências

1. ELMASRI, R.; NAVATHE, S. B. *Fundamentals of Database Systems*, 6ª ed. Addison Wesley, 2010. Caps. 17 e 18.
2. GARCIA-MOLINA, H.; ULLMAN, J. D.; WIDOM, J. *Database Systems: The Complete Book*, 2ª ed. Prentice Hall, 2008. Caps. 13 e 14.
3. FOLK, M. J.; ZOELLICK, B.; RICCARDI, G. *File Structures: An Object-Oriented Approach with C++*. Addison Wesley, 1998.
