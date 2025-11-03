#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <errno.h>

#define PORT 8080
#define TEST_PORT 8081
#define MAX_CLIENTS 10
#define IMAGE_SIZE 2000
#define BUFFER_SIZE 1024

typedef struct {
    int socket_fd;
    struct sockaddr_in address;
    int client_id;
} ClientInfo;

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

// Função para inicializar servidor de teste em outra porta
int init_test_server(int port){
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Test Socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("Test Setsockopt failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Test Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 5) < 0) {
        perror("Test Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    printf("Servidor de teste inicializado na porta %d\n", port);
    return server_fd;
}

void close_server(int server_fd){
    close(server_fd);
}

// Thread para gerenciar servidor de teste
void* test_server_thread(void* arg) {
    int test_server_fd = *((int*)arg);
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];
    
    printf("Thread de teste iniciada. Aguardando conexões na porta %d...\n", TEST_PORT);
    
    while(1) {
        int client_socket = accept(test_server_fd, (struct sockaddr *)&client_addr, &addr_len);
        
        if (client_socket < 0) {
            perror("Erro ao aceitar cliente de teste");
            continue;
        }
        
        printf("\n[TESTE] Cliente conectado: %s:%d\n", 
               inet_ntoa(client_addr.sin_addr), 
               ntohs(client_addr.sin_port));
        
        // Receber string do cliente
        memset(buffer, 0, BUFFER_SIZE);
        ssize_t bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
        
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            printf("[TESTE] Mensagem recebida: '%s'\n", buffer);
            
            // Preparar resposta
            char response[BUFFER_SIZE];
            snprintf(response, BUFFER_SIZE, 
                    "Servidor recebeu sua mensagem: '%s' (tamanho: %zd bytes)", 
                    buffer, bytes_received);
            
            // Enviar resposta
            if (send(client_socket, response, strlen(response), 0) < 0) {
                perror("[TESTE] Erro ao enviar resposta");
            } else {
                printf("[TESTE] Resposta enviada: '%s'\n", response);
            }
        } else if (bytes_received == 0) {
            printf("[TESTE] Cliente desconectou\n");
        } else {
            perror("[TESTE] Erro ao receber dados");
        }
        
        close(client_socket);
        printf("[TESTE] Conexão fechada\n\n");
    }
    
    return NULL;
}

// Função para aguardar conexões de clientes
int wait_for_clients(int server_fd, ClientInfo *clients, int num_clients) {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    printf("Aguardando %d clientes se conectarem...\n", num_clients);
    for (int i = 0; i < num_clients; i++) {
        int client_socket = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
        if (client_socket < 0) {
            perror("Erro ao aceitar cliente");
            return -1;
        }
        clients[i].socket_fd = client_socket;
        clients[i].address = client_addr;
        clients[i].client_id = i;
        printf("Cliente %d conectado: %s:%d\n", i + 1, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    }
    printf("Todos os %d clientes conectados!\n", num_clients);
    return 0;
}

// Função para enviar bloco de matriz para um cliente
int send_matrix_block(int client_socket, double **matrix, int start_row, int num_rows, int num_cols) {
    // Enviar dimensões do bloco
    int dimensions[2] = {num_rows, num_cols};
    if (send(client_socket, dimensions, sizeof(dimensions), 0) < 0) {
        perror("Erro ao enviar dimensões");
        return -1;
    }
    // Enviar dados do bloco linha por linha
    for (int i = start_row; i < start_row + num_rows; i++) {
        if (send(client_socket, matrix[i], num_cols * sizeof(double), 0) < 0) {
            perror("Erro ao enviar linha da matriz");
            return -1;
        }
    }
    return 0;
}

// Função para distribuir blocos da matriz para os clientes
void distribute_matrix_blocks(ClientInfo *clients, int num_clients, int num_blocks, int block_rows, int block_cols) {
    printf("\nGerando matriz e distribuindo blocos...\n");
    
    // Calcular dimensões totais da matriz
    int total_rows = num_blocks * block_rows;
    int total_cols = block_cols;
    
    printf("Matriz total: %d x %d\n", total_rows, total_cols);
    printf("Cada bloco: %d x %d\n", block_rows, block_cols);
    printf("Blocos por cliente: %d\n", num_blocks);
    
    // Criar matriz completa
    double **matrix = create_matrix(total_rows, total_cols);
    
    // Distribuir blocos para cada cliente
    int current_row = 0;
    
    for (int client = 0; client < num_clients; client++) {
        printf("\nEnviando %d blocos para Cliente %d...\n", num_blocks, client + 1);
        for (int block = 0; block < num_blocks; block++) {
            printf("  Enviando bloco %d (linhas %d a %d)...\n", block + 1, current_row, current_row + block_rows - 1);
            if (send_matrix_block(clients[client].socket_fd, matrix, current_row, block_rows, block_cols) < 0) {
                printf(" Erro ao enviar bloco para Cliente %d\n", client + 1);
            } else {
                printf("  Bloco %d enviado com sucesso!\n", block + 1);
            }
            current_row += block_rows;
            // Reiniciar se passar do tamanho da matriz (distribuição cíclica)
            if (current_row >= total_rows) {
                current_row = 0;
            }
        }
    }
    printf("\nTodos os blocos foram distribuídos!\n");
    
    // Liberar memória da matriz
    free_matrix(matrix, total_rows);
}

// Função para fechar todas as conexões com clientes
void close_all_clients(ClientInfo *clients, int num_clients) {
    for (int i = 0; i < num_clients; i++) {
        close(clients[i].socket_fd);
        printf("Conexão com Cliente %d fechada.\n", i + 1);
    }
}

int main(){
    FILE *fp;
    double matrix[IMAGE_SIZE][IMAGE_SIZE];
    int i, j, num_clients, num_blocks, num_lines, num_columns;

    fp = fopen("matriz_2000x2000.txt", "r");
    if (fp == NULL) {
        perror("Erro ao abrir arquivo");
        return 1;
    }
    // Ler dados da matriz do arquivo
    for (i = 0; i < IMAGE_SIZE; i++) {
        for (j = 0; j < IMAGE_SIZE; j++) {
            fscanf(fp, "%lf", &matrix[i][j]);
        }
    }
    fclose(fp);

    // Inicializar servidor de teste em thread separada
    int test_server_fd = init_test_server(TEST_PORT);
    pthread_t test_thread;
    if (pthread_create(&test_thread, NULL, test_server_thread, &test_server_fd) != 0) {
        perror("Erro ao criar thread de teste");
        return 1;
    }
    pthread_detach(test_thread); // Deixar thread rodar independentemente

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

    divide_matrix_into_blocks(matrix, IMAGE_SIZE, IMAGE_SIZE, num_clients, num_blocks, num_lines, num_columns);

    // Inicializar servidor
    int server_fd = init_server();
    printf("\nServidor inicializado e escutando na porta %d\n", PORT);
    
    // Array para armazenar informações dos clientes
    ClientInfo *clients = (ClientInfo *)malloc(num_clients * sizeof(ClientInfo));
    // Aguardar todos os clientes se conectarem
    if (wait_for_clients(server_fd, clients, num_clients) < 0) {
        printf("Erro ao aguardar clientes.\n");
        free(clients);
        close_server(server_fd);
        close_server(test_server_fd);
        return 1;
    }
    
    // Distribuir blocos da matriz para os clientes
    distribute_matrix_blocks(clients, num_clients, num_blocks, num_lines, num_columns);
    
    // Fechar todas as conexões
    printf("\nFechando conexões...\n");
    close_all_clients(clients, num_clients);
    free(clients);
    
    close_server(server_fd);
    close_server(test_server_fd);
    printf("Servidor encerrado.\n");
    
    return 0;
}