#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <errno.h>

#define PORT 8080
#define MAX_CLIENTS 10
#define IMAGE_SIZE 2000

typedef struct {
    int socket_fd;
    struct sockaddr_in address;
    int client_id;
} ClientInfo;

typedef struct MatrixBlock {
    int **matrix;
    int num_rows;
    int num_cols;
    int flag_r;
    int flag_l;
    int flag_u;
    int flag_d;
    struct MatrixBlock *next;
} MatrixBlock;

typedef struct {
    MatrixBlock *head;
    int count;
} BlockQueue;

// Função para inicializar o servidor
int init_server(){
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("Setsockopt failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    return server_fd;
}

// Função para fechar o servidor
void close_server(int server_fd){
    close(server_fd);
}

// Função para aguardar conexões de clientes
int wait_for_clients(int server_fd, ClientInfo *clients, int num_clients) {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    printf("Aguardando %d clientes se conectarem...\n", num_clients);
    for (int i = 0; i < num_clients; i++) {
        int client_socket;
        if ((client_socket = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len)) < 0) {
            perror("Erro ao aceitar cliente");
            return -1;
        }
        clients[i].socket_fd = client_socket;
        clients[i].address = client_addr;
        clients[i].client_id = i + 1;
        printf("Cliente %d conectado: %s:%d\n", i + 1, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    }
    printf("Todos os %d clientes conectados!\n", num_clients);
    return 0;
}

// Função para enviar bloco de matriz para um cliente (com flags e dimensões)
int send_matrix_block(int client_socket, MatrixBlock *block) {
    // Enviar dimensões do bloco e flags
    int header[6] = {
        block->num_rows, 
        block->num_cols,
        block->flag_u,
        block->flag_d,
        block->flag_l,
        block->flag_r
    };
    if (send(client_socket, header, sizeof(header), 0) < 0) {
        perror("Erro ao enviar header");
        return -1;
    }
    
    // Enviar dados do bloco linha por linha
    for (int i = 0; i < block->num_rows; i++) {
        if (send(client_socket, block->matrix[i], block->num_cols * sizeof(int), 0) < 0) {
            perror("Erro ao enviar linha da matriz");
            return -1;
        }
    }
    return 0;
}

void divide_matrix(int num_blocks, int num_columns, int num_lines, int **matrix, BlockQueue *matrixQueue) {
    int div_h, div_v;
    div_h = IMAGE_SIZE / num_lines;
    div_v = IMAGE_SIZE / num_columns;

    for(int b = 0; b < num_blocks; b++) {
        int block_row = (b / div_v) % div_h;
        int block_col = b % div_v;
        
        // Determinar flags (se tem vizinhos)
        int flag_u = (block_row > 0) ? 1 : 0;
        int flag_d = (block_row < div_h - 1) ? 1 : 0;
        int flag_l = (block_col > 0) ? 1 : 0;
        int flag_r = (block_col < div_v - 1) ? 1 : 0;
        
        // Calcular dimensões reais com ghost cells
        int actual_rows = num_lines + flag_u + flag_d;
        int actual_cols = num_columns + flag_l + flag_r;
        
        // Alocar nova matriz para CADA bloco (incluindo ghost cells)
        int **block_data = malloc(actual_rows * sizeof(int *));
        for (int i = 0; i < actual_rows; i++) {
            block_data[i] = malloc(actual_cols * sizeof(int));
        }
        
        // Copiar dados do bloco principal
        int start_row = block_row * num_lines;
        int start_col = block_col * num_columns;
        
        for (int i = 0; i < num_lines; i++) {
            for (int j = 0; j < num_columns; j++) {
                // Offset na matriz do bloco considerando ghost cells
                int block_i = i + flag_u;
                int block_j = j + flag_l;
                block_data[block_i][block_j] = matrix[start_row + i][start_col + j];
            }
        }
        
        // Copiar ghost cells (células vizinhas)
        // Ghost cell superior
        if (flag_u) {
            for (int j = 0; j < num_columns; j++) {
                int block_j = j + flag_l;
                block_data[0][block_j] = matrix[start_row - 1][start_col + j];
            }
        }
        
        // Ghost cell inferior
        if (flag_d) {
            for (int j = 0; j < num_columns; j++) {
                int block_j = j + flag_l;
                block_data[actual_rows - 1][block_j] = matrix[start_row + num_lines][start_col + j];
            }
        }
        
        // Ghost cell esquerda
        if (flag_l) {
            for (int i = 0; i < num_lines; i++) {
                int block_i = i + flag_u;
                block_data[block_i][0] = matrix[start_row + i][start_col - 1];
            }
        }
        
        // Ghost cell direita
        if (flag_r) {
            for (int i = 0; i < num_lines; i++) {
                int block_i = i + flag_u;
                block_data[block_i][actual_cols - 1] = matrix[start_row + i][start_col + num_columns];
            }
        }
        
        // Preencher cantos se necessário (para stencil diagonal, se precisar)
        if (flag_u && flag_l) {
            block_data[0][0] = matrix[start_row - 1][start_col - 1];
        }
        if (flag_u && flag_r) {
            block_data[0][actual_cols - 1] = matrix[start_row - 1][start_col + num_columns];
        }
        if (flag_d && flag_l) {
            block_data[actual_rows - 1][0] = matrix[start_row + num_lines][start_col - 1];
        }
        if (flag_d && flag_r) {
            block_data[actual_rows - 1][actual_cols - 1] = matrix[start_row + num_lines][start_col + num_columns];
        }
        
        MatrixBlock *new_block = malloc(sizeof(MatrixBlock));
        new_block->matrix = block_data;
        new_block->num_rows = actual_rows;
        new_block->num_cols = actual_cols;
        new_block->flag_r = flag_r;
        new_block->flag_l = flag_l;
        new_block->flag_u = flag_u;
        new_block->flag_d = flag_d;
        new_block->next = NULL;
        
        if (matrixQueue->head == NULL) {
            matrixQueue->head = new_block;
        } else {
            MatrixBlock *temp = matrixQueue->head;
            while (temp->next != NULL) {
                temp = temp->next;
            }
            temp->next = new_block;
        }
        matrixQueue->count++;
    }
}

// Função para distribuir blocos da matriz para os clientes
void distribute_matrix_blocks(ClientInfo *clients, int num_clients, BlockQueue *queue) {
    MatrixBlock *current_block = queue->head;
    int client_idx = 0;
    int block_count = 0;
    
    printf("\nDistribuindo %d blocos para %d clientes...\n", queue->count, num_clients);
    
    while (current_block != NULL) {
        printf("Enviando bloco %d para Cliente %d (tamanho: %dx%d, flags: U=%d D=%d L=%d R=%d)...\n", 
               block_count + 1, client_idx + 1,
               current_block->num_rows, current_block->num_cols,
               current_block->flag_u, current_block->flag_d,
               current_block->flag_l, current_block->flag_r);
        
        if (send_matrix_block(clients[client_idx].socket_fd, current_block) < 0) {
            printf("Erro ao enviar bloco para o Cliente %d\n", client_idx + 1);
        } else {
            printf("Bloco enviado com sucesso!\n");
        }
        
        current_block = current_block->next;
        block_count++;
        client_idx = (client_idx + 1) % num_clients; // Round-robin
    }
    printf("\nTodos os blocos foram distribuídos!\n");
}

// Função para fechar todas as conexões com clientes
void close_all_clients(ClientInfo *clients, int num_clients) {
    for (int i = 0; i < num_clients; i++) {
        close(clients[i].socket_fd);
        printf("Conexão com Cliente %d fechada.\n", i + 1);
    }
}

// Função para liberar memória da fila de blocos
void free_block_queue(BlockQueue *queue) {
    MatrixBlock *current = queue->head;
    while (current != NULL) {
        MatrixBlock *next = current->next;
        // Liberar a matriz do bloco
        for (int i = 0; i < current->num_rows; i++) {
            free(current->matrix[i]);
        }
        free(current->matrix);
        free(current);
        current = next;
    }
    free(queue);
}

// Função para receber um bloco processado de um cliente
int receive_processed_block(int client_socket, int **block_buffer, int num_rows, int num_cols) {
    for (int i = 0; i < num_rows; i++) {
        ssize_t bytes_received = recv(client_socket, block_buffer[i], num_cols * sizeof(int), MSG_WAITALL);
        size_t expected_bytes = num_cols * sizeof(int);
        
        if (bytes_received < 0) {
            perror("Erro ao receber linha do bloco processado");
            return -1;
        }
        if ((size_t)bytes_received != expected_bytes) {
            fprintf(stderr, "Erro: recebido %zd bytes, esperado %zu bytes\n", bytes_received, expected_bytes);
            return -1;
        }
    }
    return 0;
}

// Função para receber resultados dos clientes e remontar a matriz
void receive_results_from_clients(ClientInfo *clients, int num_clients, BlockQueue *queue, int **matrix, int num_lines, int num_columns) {
    MatrixBlock *current_block = queue->head;
    int client_idx = 0;
    int block_count = 0;
    
    printf("\nAguardando resultados processados dos clientes...\n");
    
    while (current_block != NULL) {
        printf("Recebendo bloco processado %d do Cliente %d...\n", block_count + 1, client_idx + 1);
        
        int **processed_block = malloc(current_block->num_rows * sizeof(int *));
        for (int i = 0; i < current_block->num_rows; i++) {
            processed_block[i] = malloc(current_block->num_cols * sizeof(int));
        }

        if (receive_processed_block(clients[client_idx].socket_fd, processed_block, current_block->num_rows, current_block->num_cols) < 0) {
            printf("Erro ao receber bloco do Cliente %d\n", client_idx + 1);
        } else {
            printf("Bloco %d recebido com sucesso!\n", block_count + 1);
            
            int div_h = IMAGE_SIZE / num_lines;
            int div_v = IMAGE_SIZE / num_columns;
            int block_row = (block_count / div_v) % div_h;
            int block_col = block_count % div_v;
            
            int start_row = block_row * num_lines;
            int start_col = block_col * num_columns;
            
            int offset_i = current_block->flag_u ? 1 : 0;
            int offset_j = current_block->flag_l ? 1 : 0;
            
            for (int i = 0; i < num_lines; i++) {
                for (int j = 0; j < num_columns; j++) {
                    matrix[start_row + i][start_col + j] = processed_block[i + offset_i][j + offset_j];
                }
            }
        }
        
        for (int i = 0; i < current_block->num_rows; i++) {
            free(processed_block[i]);
        }
        free(processed_block);
        
        current_block = current_block->next;
        block_count++;
        client_idx = (client_idx + 1) % num_clients;
    }
    
    printf("\nTodos os blocos processados foram recebidos e remontados!\n");
}

int main(){
    FILE *fp;
    int **matrix;
    int i, j, num_clients, num_blocks, num_lines, num_columns;
    struct timespec start_time, end_time;
    

    matrix = malloc(IMAGE_SIZE * sizeof(int *));
    for (i = 0; i < IMAGE_SIZE; i++) {
        matrix[i] = malloc(IMAGE_SIZE * sizeof(int));
    }

    fp = fopen("data/matriz_2000x2000.txt", "r");
    if (fp == NULL) {
        perror("Erro ao abrir arquivo");
        for (i = 0; i < IMAGE_SIZE; i++) {
            free(matrix[i]);
        }
        free(matrix);
        return 1;
    }
    
    printf("Carregando matriz do arquivo...\n");
    for (i = 0; i < IMAGE_SIZE; i++) {
        for (j = 0; j < IMAGE_SIZE; j++) {
            fscanf(fp, "%d", &matrix[i][j]);
        }
    }
    fclose(fp);
    printf("Matriz carregada com sucesso!\n\n");

    printf("=== Configuração do Servidor ===\n");
    printf("Digite a quantidade de clientes do teste: ");
    scanf("%d", &num_clients);
    if (num_clients > MAX_CLIENTS || num_clients <= 0) {
        printf("Número de clientes inválido! (1-%d)\n", MAX_CLIENTS);
        return 1;
    }
    printf("Digite a quantidade de blocos por cliente: ");
    scanf("%d", &num_blocks);
    printf("Digite a quantidade de linhas por bloco: ");
    scanf("%d", &num_lines);
    printf("Digite a quantidade de colunas por bloco: ");
    scanf("%d", &num_columns);

    BlockQueue *queue = malloc(sizeof(BlockQueue));
    queue->head = NULL;
    queue->count = 0;

    int server_fd = init_server();
    printf("\nServidor inicializado e escutando na porta %d\n", PORT);

    divide_matrix(num_blocks, num_columns, num_lines, matrix, queue);

    ClientInfo *clients = (ClientInfo *)malloc(num_clients * sizeof(ClientInfo));
    if (wait_for_clients(server_fd, clients, num_clients) < 0) {
        printf("Erro ao aguardar clientes.\n");
        free(clients);
        close_server(server_fd);
        return 1;
    }
    
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    
    distribute_matrix_blocks(clients, num_clients, queue);

    receive_results_from_clients(clients, num_clients, queue, matrix, num_lines, num_columns);
    
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    
    double time_elapsed = (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_nsec - start_time.tv_nsec) / 1000000000.0;
    
    printf("\n=== ESTATÍSTICAS ===\n");
    printf("Tempo total de processamento: %.6f segundos\n", time_elapsed);
    
    
    printf("\nFechando conexões...\n");
    close_all_clients(clients, num_clients);
    free(clients);
    
    free_block_queue(queue);
    
    for (i = 0; i < IMAGE_SIZE; i++) {
        free(matrix[i]);
    }
    free(matrix);
    
    close_server(server_fd);
    printf("Servidor encerrado.\n");
    
    return 0;
}