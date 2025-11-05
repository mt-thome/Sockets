# Projeto Sockets - Programação Concorrente e Paralela
Este projeto implementa um sistema cliente-servidor em C para processamento paralelo de imagens usando sockets TCP. O servidor distribui blocos de uma matriz 2000x2000 para múltiplos clientes que aplicam um filtro de suavização (stencil de 5 pontos) e retornam os resultados processados.

## Objetivo

Processar uma matriz 2000x2000 de inteiros aplicando um filtro de suavização (média dos 5 pontos: centro + 4 vizinhos cardeais). O servidor divide a matriz em blocos com **ghost cells**, distribui para clientes que processam em paralelo, e remonta a matriz final.

## Estrutura do Projeto

```
C/Sockets/
├── bin/                        # Executáveis compilados
│   ├── server                  # Servidor
│   └── client                  # Cliente
├── data/                       # Dados de entrada
│   └── matriz_2000x2000.txt    # Matriz 2000x2000 de inteiros
├── src/                        # Código-fonte
│   ├── server.c                # Implementação do servidor
│   └── client.c                # Implementação do cliente
├── tests/                      # Scripts de teste
│   └── test_simple.sh          # Teste básico
└── README.md                   # Esta documentação
```

## Pré-requisitos

- **gcc**: Compilador C
- **Sistema**: Linux/Unix
- **Bibliotecas**: POSIX (padrão em sistemas Unix)

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
# Compilar servidor
gcc -o bin/server src/server.c -Wall -Wextra

# Compilar cliente
gcc -o bin/client src/client.c -Wall -Wextra

# Ou compilar ambos de uma vez
gcc -o bin/server src/server.c -Wall -Wextra && \
gcc -o bin/client src/client.c -Wall -Wextra
```

## Como Executar

### 1. Verificar Arquivo de Matriz

O arquivo `data/matriz_2000x2000.txt` deve existir com 2000 linhas de 2000 inteiros cada.

**Gerar arquivo de teste (opcional):**
```bash
python3 -c "
import random
with open('data/matriz_2000x2000.txt', 'w') as f:
    for i in range(2000):
        linha = ' '.join(str(random.randint(0, 255)) for _ in range(2000))
        f.write(linha + '\n')
print('Matriz gerada!')
"
```

### 2. Iniciar o Servidor

Em um terminal:
```bash
cd /home/user/Projetos/C/Sockets
./bin/server
```

**Configuração interativa:**
```
=== Configuração do Servidor ===
Digite a quantidade de clientes do teste: 2
Digite a quantidade de blocos por cliente: 4
Digite a quantidade de linhas por bloco: 500
Digite a quantidade de colunas por bloco: 500
```

O servidor então:
1. ✓ Carrega matriz 2000×2000
2. ✓ Cria 4 blocos com ghost cells
3. Aguarda 2 clientes conectarem...

### 3. Iniciar Cliente(s)

**Para cada cliente**, abra um **novo terminal**:

```bash
cd /home/user/Projetos/C/Sockets
./bin/client
```

**Saída do cliente:**
```
Conectado com sucesso ao servidor 127.0.0.1:8080

=== Bloco 1 recebido ===
Dimensões: 501x501 (flags: U=0 D=1 L=0 R=1)
Recebendo dados da matriz...
Matriz recebida com sucesso!
Processando área útil: 500x500...
Processamento concluído!
Enviando resultado ao servidor...
Resultado enviado com sucesso!

=== Bloco 2 recebido ===
...

Servidor fechou a conexão. Total de blocos processados: 2
Cliente encerrado.
```

### 4. Resultado do Servidor

Após todos os clientes processarem:

```
=== MODO ASSÍNCRONO (2 clientes) ===
Distribuindo 4 blocos...
...
Cliente 1 está pronto! Recebendo bloco 1...
Cliente 2 está pronto! Recebendo bloco 2...
Cliente 1 está pronto! Recebendo bloco 3...
Cliente 2 está pronto! Recebendo bloco 4...

Todos os blocos processados foram recebidos e remontados! (4/4)

=== ESTATÍSTICAS ===
Tempo total de processamento: 0.011211 segundos

Fechando conexões...
Servidor encerrado.
```

## Exemplo de Teste Completo

### Teste com 1 Cliente (Modo Síncrono)

**Terminal 1 - Servidor:**
```bash
./bin/server
# Input: 1, 1, 500, 500
```

**Terminal 2 - Cliente:**
```bash
./bin/client
```

**Resultado esperado:**
- ✓ Modo síncrono (ping-pong) ativado
- ✓ 1 bloco processado
- ✓ Tempo: ~0.003 segundos

### Teste com 2 Clientes (Modo Assíncrono com select)

**Terminal 1 - Servidor:**
```bash
./bin/server
# Input: 2, 4, 500, 500
```

**Terminal 2 - Cliente 1:**
```bash
./bin/client
```

**Terminal 3 - Cliente 2:**
```bash
./bin/client
```

**Resultado esperado:**
- ✓ Modo assíncrono ativado
- ✓ 4 blocos distribuídos (round-robin)
- ✓ Recepção com `select()` (ordem flexível)
- ✓ Tempo: ~0.011 segundos

## Arquitetura e Implementação

### Estruturas de Dados

**ClientInfo** - Informações de cliente conectado:
```c
typedef struct {
    int socket_fd;              // Socket do cliente
    struct sockaddr_in address; // Endereço do cliente
    int client_id;              // ID do cliente
} ClientInfo;
```

**MatrixBlock** - Representa um bloco da matriz com ghost cells:
```c
typedef struct MatrixBlock {
    int **matrix;               // Dados do bloco (COM ghost cells)
    int num_rows;               // Linhas totais (COM ghost cells)
    int num_cols;               // Colunas totais (COM ghost cells)
    int flag_u, flag_d;         // Flags ghost cells vertical
    int flag_l, flag_r;         // Flags ghost cells horizontal
    struct MatrixBlock *next;   // Próximo bloco (lista encadeada)
} MatrixBlock;
```

**BlockQueue** - Fila de blocos:
```c
typedef struct {
    MatrixBlock *head;          // Primeiro bloco
    int count;                  // Total de blocos
} BlockQueue;
```

### Funções Principais do Servidor

| Função | Descrição |
|--------|-----------|
| `init_server()` | Cria socket TCP na porta 8080, habilita SO_REUSEADDR |
| `wait_for_clients()` | Aceita N conexões de clientes via `accept()` |
| `divide_matrix()` | Divide matriz em blocos e adiciona ghost cells |
| `send_matrix_block()` | Envia header (6 ints) + dados linha por linha |
| `process_single_client_sync()` | Protocolo ping-pong para 1 cliente |
| `distribute_matrix_blocks()` | Distribui blocos em round-robin (múltiplos clientes) |
| `receive_processed_block()` | Recebe matriz linha por linha com `MSG_WAITALL` |
| `receive_results_from_clients()` | Usa `select()` para receber de qualquer cliente pronto |
| `free_block_queue()` | Libera toda memória alocada |

### Protocolo Adaptativo

**1 Cliente → Modo Síncrono (ping-pong):**
```
Servidor                Cliente
   |--[Bloco 1 COM ghosts]-->|
   |                         | (processa)
   |<-[Result 1 SEM ghosts]--|
   |--[Bloco 2 COM ghosts]-->|
   |                         | (processa)
   |<-[Result 2 SEM ghosts]--|
```

**N Clientes → Modo Assíncrono (select):**
```
Servidor                Cliente 1          Cliente 2
   |--[Bloco 1]---------->|                     |
   |--[Bloco 2]-------------------------->|     |
   |--[Bloco 3]---------->|                     |
   |--[Bloco 4]-------------------------->|     |
   |                      | (processa)         | (processa)
   |                                            |
   |  select() monitora ambos os sockets       |
   |                                            |
   |<-[Result 2]---------------------------| ✓ Pronto primeiro!
   |<-[Result 1]----------|                     |
   |<-[Result 4]---------------------------|     |
   |<-[Result 3]----------|                     |
```

## Configuração de Rede

- **Porta**: 8080
- **Protocolo**: TCP (SOCK_STREAM)
- **Endereço**: 0.0.0.0 (aceita qualquer interface)
- **Max clientes**: 10
- **Flags socket**: 
  - `SO_REUSEADDR`: Permite reutilização rápida da porta
  - `MSG_WAITALL`: Garante recepção completa dos dados

## Notas Técnicas

### Decisões de Design

1. **Protocolo Adaptativo**
   - 1 cliente → síncrono (evita confusão no canal único)
   - N clientes → assíncrono com `select()` (maximiza paralelismo)

2. **Ghost Cells**
   - Enviadas COM ghost cells (contexto para stencil)
   - Recebidas SEM ghost cells (reduz tráfego de retorno)

3. **Alocação de Memória**
   - Matriz 2000×2000 no heap (~16MB, evita stack overflow)
   - Cada bloco tem cópia independente (sem compartilhamento)

4. **Sincronização**
   - `select()` evita bloqueio em cliente lento
   - `MSG_WAITALL` garante recepção atômica de dados

## Testes de Desempenho

### Cenários Recomendados

O projeto suporta testes variando:
- **Número de clientes**: 1, 2, 4, 8
- **Divisão da matriz**: diferentes tamanhos de bloco
- **Total de blocos**: 1, 4, 16, 64

Exemplos:
```bash
# Teste 1: 1 cliente, 1 bloco 2000x2000
# Input: 1, 1, 2000, 2000

# Teste 2: 2 clientes, 4 blocos 1000x1000
# Input: 2, 4, 1000, 1000

# Teste 3: 4 clientes, 16 blocos 500x500
# Input: 4, 16, 500, 500

# Teste 4: 8 clientes, 64 blocos 250x250
# Input: 8, 64, 250, 250
```

### Análise de Resultados

- **Speedup esperado**: Próximo linear até 4 clientes
- **Overhead de comunicação**: Aumenta com mais blocos pequenos
- **Select() eficiente**: Evita gargalo na recepção

## Referências

- **POSIX Sockets API**: `man 2 socket`, `man 2 select`
- **TCP Protocol**: RFC 793
- **Clock functions**: `man 2 clock_gettime`