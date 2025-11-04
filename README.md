# Projeto Sockets - Programação Concorrente e Paralela

Este projeto implementa um sistema cliente-servidor em C para paralelizar uma tarefa de processamento de imagem aplicando um filtro de suavização (stencil de cinco pontos). A comunicação entre os processos é realizada inteiramente via Sockets TCP.

O objetivo é processar uma matriz 2000x2000 de inteiros aplicando um filtro de suavização (stencil de cinco pontos). O servidor divide a matriz em blocos e os distribui para múltiplos clientes, que processam os blocos em paralelo e os devolvem. O servidor, então, monta a imagem processada final.

## Estrutura do Projeto

```
C/Sockets/
├── bin/                    # Executáveis compilados (criado após compilação)
├── data/                   # Arquivos de entrada
│   └── matriz_2000x2000.txt  # Matriz de entrada (2000x2000 inteiros)
├── src/                    # Código-fonte
│   ├── server.c            # Implementação do servidor
│   └── client.c            # Implementação do cliente (em desenvolvimento)
├── Makefile                # Arquivo para compilação
└── README.md               # Este arquivo
```

## Pré-requisitos

Para compilar e executar este projeto, você precisará de:
* `gcc`: Compilador C
* Sistema operacional Linux/Unix

## Servidor - Funcionalidades Implementadas

O servidor implementa as seguintes funcionalidades:

### 1. Carregamento da Matriz
- Carrega uma matriz 2000x2000 de inteiros do arquivo `data/matriz_2000x2000.txt`
- Alocação dinâmica para evitar estouro de pilha
- Validação de abertura de arquivo

### 2. Divisão em Blocos com Ghost Cells
O servidor divide a matriz em blocos configuráveis e adiciona **células fantasmas (ghost cells)** para permitir o processamento de stencil nas bordas internas:

- Cada bloco pode ter dimensões configuráveis (ex: 100x100, 200x200)
- **Ghost cells** são adicionadas automaticamente quando o bloco tem vizinhos:
  - `flag_u = 1`: adiciona 1 linha acima (ghost cell superior)
  - `flag_d = 1`: adiciona 1 linha abaixo (ghost cell inferior)
  - `flag_l = 1`: adiciona 1 coluna à esquerda (ghost cell esquerda)
  - `flag_r = 1`: adiciona 1 coluna à direita (ghost cell direita)

**Exemplo:**
- Bloco interno: 100x100 → com ghost cells: 102x102
- Bloco de canto: 100x100 → com ghost cells: 101x101 (apenas 2 direções)

### 3. Distribuição Round-Robin
- Distribui blocos entre clientes de forma circular (round-robin)
- Cliente 1 recebe blocos 1, 3, 5...
- Cliente 2 recebe blocos 2, 4, 6...

### 4. Protocolo de Comunicação

**Envio (Servidor → Cliente):**
1. **Header** (6 inteiros):
   - `num_rows`: número de linhas do bloco (incluindo ghost cells)
   - `num_cols`: número de colunas do bloco (incluindo ghost cells)
   - `flag_u`: 1 se tem ghost cell superior, 0 caso contrário
   - `flag_d`: 1 se tem ghost cell inferior, 0 caso contrário
   - `flag_l`: 1 se tem ghost cell esquerda, 0 caso contrário
   - `flag_r`: 1 se tem ghost cell direita, 0 caso contrário
2. **Dados da matriz**: linhas do bloco enviadas sequencialmente

**Recebimento (Cliente → Servidor):**
1. **Dados processados**: matriz processada (com ghost cells) enviada linha por linha
2. O servidor extrai apenas a área útil (sem ghost cells) e remonta a matriz original

### 5. Reconstrução da Matriz
- Recebe blocos processados dos clientes na mesma ordem de envio
- Remove ghost cells ao remontar a matriz
- Reconstrói a matriz 2000x2000 completa com os dados processados

### 6. Medição de Tempo
- Mede o tempo total de processamento paralelo usando `clock_gettime()` (precisão de nanossegundos)
- **Início**: antes de enviar os blocos aos clientes
- **Fim**: após receber todos os blocos processados
- Exibe estatísticas ao final da execução

## Como Compilar

### Compilar o Servidor

```bash
gcc -Wall -o bin/server src/server.c
```

### Compilar o Cliente 

```bash
gcc -Wall -o bin/client src/client.c
```

## Como Executar

### 1. Gerar Arquivo de Matriz (Opcional)

Se você não tiver o arquivo `data/matriz_2000x2000.txt`, pode gerar um com valores aleatórios:

```bash
python3 -c "
import random
with open('data/matriz_2000x2000.txt', 'w') as f:
    for i in range(2000):
        for j in range(2000):
            f.write(str(random.randint(0, 255)) + ' ')
        f.write('\n')
print('Arquivo criado!')
"
```

### 2. Iniciar o Servidor

Em um terminal, inicie o servidor:

```bash
./bin/server
```

O servidor solicitará as seguintes configurações:
- **Quantidade de clientes**: número de clientes que irão se conectar (1-10)
- **Quantidade de blocos**: número total de blocos a serem criados
- **Linhas por bloco**: altura de cada bloco (sem contar ghost cells)
- **Colunas por bloco**: largura de cada bloco (sem contar ghost cells)

**Exemplo de entrada:**
```
Digite a quantidade de clientes do teste: 2
Digite a quantidade de blocos por cliente: 4
Digite a quantidade de linhas por bloco: 100
Digite a quantidade de colunas por bloco: 100
```

O servidor então:
1. Carrega a matriz do arquivo
2. Aguarda a conexão dos clientes especificados
3. Divide a matriz em blocos (adicionando ghost cells automaticamente)
4. **Marca o tempo inicial**
5. Distribui os blocos entre os clientes (round-robin)
6. Aguarda o recebimento de todos os blocos processados
7. Remonta a matriz descartando as ghost cells
8. **Marca o tempo final e exibe estatísticas**
9. Fecha as conexões

**Exemplo de saída:**
```
=== ESTATÍSTICAS ===
Tempo total de processamento: 2.345678 segundos

Fechando conexões...
```

### 3. Iniciar o(s) Cliente(s)

Para cada cliente que você deseja executar, abra um **novo terminal** e conecte-se ao servidor. (Para testes locais, use o IP `127.0.0.1`).

```bash
# Exemplo para iniciar um cliente
./bin/cliente 127.0.0.1
```
Você pode iniciar múltiplos clientes em múltiplos terminais para executar o processamento paralelo.

## Detalhes de Implementação do Servidor

### Estruturas de Dados

**ClientInfo**: Armazena informações de cada cliente conectado
```c
typedef struct {
    int socket_fd;              // Socket do cliente
    struct sockaddr_in address; // Endereço do cliente
    int client_id;              // ID do cliente
} ClientInfo;
```

**MatrixBlock**: Representa um bloco da matriz com ghost cells
```c
typedef struct MatrixBlock {
    int **matrix;               // Dados do bloco
    int num_rows;               // Linhas (com ghost cells)
    int num_cols;               // Colunas (com ghost cells)
    int flag_r, flag_l;         // Flags para ghost cells horizontais
    int flag_u, flag_d;         // Flags para ghost cells verticais
    struct MatrixBlock *next;   // Próximo bloco na fila
} MatrixBlock;
```

**BlockQueue**: Fila encadeada de blocos
```c
typedef struct {
    MatrixBlock *head;          // Primeiro bloco da fila
    int count;                  // Número de blocos na fila
} BlockQueue;
```

### Funções Principais

1. **`init_server()`**: Inicializa o socket do servidor na porta 8080
2. **`wait_for_clients()`**: Aceita conexões de N clientes
3. **`divide_matrix()`**: Divide a matriz em blocos e adiciona ghost cells
4. **`send_matrix_block()`**: Envia um bloco (header + dados) para o cliente
5. **`distribute_matrix_blocks()`**: Distribui blocos entre clientes (round-robin)
6. **`receive_processed_block()`**: Recebe um bloco processado linha por linha do cliente
7. **`receive_results_from_clients()`**: Recebe todos os blocos processados e remonta a matriz
8. **`free_block_queue()`**: Libera toda a memória alocada

### Algoritmo de Ghost Cells

Para cada bloco na posição `(block_row, block_col)`:

1. **Determinar flags**: verificar se existem vizinhos em cada direção
2. **Calcular dimensões**: `actual_size = base_size + ghost_cells`
3. **Copiar dados centrais**: região original do bloco
4. **Copiar ghost cells**: 
   - Superior: linha `start_row - 1` da matriz original
   - Inferior: linha `start_row + num_lines` da matriz original
   - Esquerda: coluna `start_col - 1` da matriz original
   - Direita: coluna `start_col + num_columns` da matriz original
5. **Preencher cantos**: intersecções das ghost cells (se aplicável)

### Configuração da Rede

- **Porta**: 8080
- **Protocolo**: TCP (SOCK_STREAM)
- **Máximo de clientes**: 10
- **Opção SO_REUSEADDR**: Habilitada para reutilização rápida da porta

### Fluxo de Execução Completo

```
1. Servidor carrega matriz 2000x2000
2. Divide em blocos com ghost cells
3. Aguarda N clientes conectarem
4. [TEMPO INICIAL] ⏱️
5. Envia blocos (round-robin)
6. Aguarda blocos processados
7. Remonta matriz (descarta ghost cells)
8. [TEMPO FINAL] ⏱️
9. Exibe estatísticas
10. Fecha conexões
```

## Notas Técnicas

- **Alocação dinâmica**: A matriz 2000x2000 é alocada no heap para evitar stack overflow
- **Liberação de memória**: Todas as estruturas são liberadas adequadamente ao final
- **Cada bloco tem cópia própria**: Não há compartilhamento de dados entre blocos
- **Ghost cells evitam comunicação**: Clientes não precisam trocar dados entre si
- **Medição de tempo precisa**: Usa `clock_gettime(CLOCK_MONOTONIC)` com precisão de nanossegundos
- **Protocolo confiável**: TCP garante entrega ordenada dos dados
- **Recepção com MSG_WAITALL**: Garante recebimento completo de cada linha da matriz

## Testes e Relatório

O projeto requer a medição de tempo para 14 cenários de teste diferentes, variando o número de clientes e a forma como a imagem é dividida.

Para automatizar a execução desses testes, utilize o script `tests/run_tests.sh` (conforme definido na estrutura do projeto). Ele executará os 14 cenários e salvará os tempos de execução em `output/tempos_execucao.csv`.

O relatório final (`relatorio.pdf`) contém a análise e comparação de desempenho desses testes, conforme solicitado.