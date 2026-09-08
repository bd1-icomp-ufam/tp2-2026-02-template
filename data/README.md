# data/

Este diretório fica **vazio no repositório** e é usado em tempo de execução.

1. Baixe a amostra oficial `artigo-amostra.csv.gz` indicada no enunciado e coloque-a aqui.
2. Descompacte antes da primeira carga:

```
gzip -dk data/artigo-amostra.csv.gz
```

3. O diretório é montado como `/data` no contêiner. Os arquivos de dados e de
   índice produzidos pelo trabalho devem ser gravados em `/data/db`, de modo que
   uma execução aproveite os arquivos criados pela anterior.

Não versione o CSV nem os arquivos gerados.
