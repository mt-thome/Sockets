# Projeto 2 Sockets - Programação Concorrente e Paralela

Este projeto implementa um sistema cliente-servidor em C para paralelizar uma tarefa de "falsificação" de renderização de imagem, conforme especificado na disciplina de Programação Concorrente e Paralela. A comunicação entre os processos é realizada inteiramente via Sockets TCP.

O objetivo é processar uma imagem false (um arquivo de 2000x2000 inteiros) aplicando um filtro de suavização (stencil de cinco pontos). O servidor divide a imagem em pedaços e os distribui para múltiplos clientes, que processam os pedaços em paralelo e os devolvem. O servidor, então, monta a imagem processada final.

## Estrutura do Projeto

```
projeto_sockets/
├── bin/                # Executáveis compilados (servidor, cliente)
├── data/               # Arquivos de entrada (imagem_original.txt)
├── include/            # Arquivos de cabeçalho (.h)
├── src/                # Código-fonte (.c)
├── Makefile            # Arquivo para compilação
└── README.md           # Este arquivo
```

## Pré-requisitos

Para compilar e executar este projeto, você precisará de:
* `make`: Para compilar o projeto.
* `gcc`: (ou outro compilador C) Para ser usado pelo `make`.

## Como Compilar

O projeto utiliza um `Makefile` para gerenciar a compilação. Para compilar todos os programas (`servidor` e `cliente`), basta executar o comando na raiz do projeto:

```bash
make
```
Os executáveis serão criados e colocados na pasta `bin/`.

## Como Executar

A execução do projeto é dividida em 3 etapas:

### 1. Iniciar o Servidor

Em um terminal, inicie o servidor. Ele irá carregar a imagem `data/imagem_original.txt` e começará a escutar por conexões de clientes.

```bash
./bin/servidor
```
O servidor indicará quando estiver pronto e aguardando clientes.

### 2. Iniciar o(s) Cliente(s)

Para cada cliente que você deseja executar, abra um **novo terminal** e conecte-se ao servidor. (Para testes locais, use o IP `127.0.0.1`).

```bash
# Exemplo para iniciar um cliente
./bin/cliente 127.0.0.1
```
Você pode iniciar múltiplos clientes em múltiplos terminais para executar o processamento paralelo.

## Testes e Relatório

O projeto requer a medição de tempo para 14 cenários de teste diferentes, variando o número de clientes e a forma como a imagem é dividida.

Para automatizar a execução desses testes, utilize o script `tests/run_tests.sh` (conforme definido na estrutura do projeto). Ele executará os 14 cenários e salvará os tempos de execução em `output/tempos_execucao.csv`.

O relatório final (`relatorio.pdf`) contém a análise e comparação de desempenho desses testes, conforme solicitado.