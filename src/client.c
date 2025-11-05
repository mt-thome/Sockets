#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERVER_PORT 8080
#define SERVER_IP "127.0.0.1"

int main() {
    int socket_fd;
    struct sockaddr_in server_addr;

    if((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("Erro ao criar socket");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);

    if(inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0){
        printf("Endereço IP inválido\n");
        close(socket_fd);
        exit(EXIT_FAILURE);
    }

    if(connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Erro ao conectar");
        close(socket_fd);
        exit(EXIT_FAILURE);
    }
    printf("Conectado com sucesso ao servidor %s:%d\n", SERVER_IP, SERVER_PORT);

    int blocks_processed = 0;
    while(1){
        int header[6];
        ssize_t bytes_read = recv(socket_fd, header, sizeof(header), MSG_WAITALL);
        
        if(bytes_read <= 0){
            if(bytes_read == 0){
                printf("\nServidor fechou a conexão. Total de blocos processados: %d\n", blocks_processed);
            } else {
                perror("Erro ao receber header");
            }
            break;
        }
        
        int num_rows = header[0];
        int num_cols = header[1];
        int flag_u = header[2];
        int flag_d = header[3];
        int flag_l = header[4];
        int flag_r = header[5];
        
        printf("\n=== Bloco %d recebido ===\n", blocks_processed + 1);
        printf("Dimensões: %dx%d (flags: U=%d D=%d L=%d R=%d)\n", num_rows, num_cols, flag_u, flag_d, flag_l, flag_r);
        
        int **matrix = malloc(num_rows * sizeof(int *));
        for(int i = 0; i < num_rows; i++){
            matrix[i] = malloc(num_cols * sizeof(int));
        }
        
        printf("Recebendo dados da matriz...\n");
        for(int i = 0; i < num_rows; i++){
            bytes_read = recv(socket_fd, matrix[i], num_cols * sizeof(int), MSG_WAITALL);
            if(bytes_read != num_cols * sizeof(int)){
                fprintf(stderr, "Erro ao receber linha %d (esperado: %ld bytes, recebido: %ld bytes)\n", 
                        i, num_cols * sizeof(int), bytes_read);
                for(int j = 0; j <= i; j++){
                    free(matrix[j]);
                }
                free(matrix);
                close(socket_fd);
                exit(EXIT_FAILURE);
            }
        }
        printf("Matriz recebida com sucesso!\n");
        
        int start_i = flag_u ? 1 : 0;
        int end_i = flag_d ? num_rows - 1 : num_rows;
        int start_j = flag_l ? 1 : 0;
        int end_j = flag_r ? num_cols - 1 : num_cols;
        
        int result_rows = end_i - start_i;
        int result_cols = end_j - start_j;
        
        printf("Processando área útil: %dx%d (índices de [%d:%d, %d:%d])...\n", 
               result_rows, result_cols, start_i, end_i, start_j, end_j);
        
        int **result = malloc(result_rows * sizeof(int *));
        for(int i = 0; i < result_rows; i++){
            result[i] = malloc(result_cols * sizeof(int));
        }
        
        for(int i = start_i; i < end_i; i++){
            for(int j = start_j; j < end_j; j++){
                int center = matrix[i][j];
                int top    = (i > 0) ? matrix[i-1][j] : center;           
                int bottom = (i < num_rows-1) ? matrix[i+1][j] : center;  
                int left   = (j > 0) ? matrix[i][j-1] : center;           
                int right  = (j < num_cols-1) ? matrix[i][j+1] : center;  
                
                int sum = center + top + bottom + left + right;
                result[i - start_i][j - start_j] = sum / 5;
            }
        }
        printf("Processamento concluído!\n");
        
        printf("Enviando resultado ao servidor...\n");
        for(int i = 0; i < result_rows; i++){
            ssize_t bytes_sent = send(socket_fd, result[i], result_cols * sizeof(int), 0);
            if(bytes_sent != result_cols * sizeof(int)){
                fprintf(stderr, "Erro ao enviar linha %d\n", i);
                for(int j = 0; j < num_rows; j++){
                    free(matrix[j]);
                }
                free(matrix);
                for(int j = 0; j < result_rows; j++){
                    free(result[j]);
                }
                free(result);
                close(socket_fd);
                exit(EXIT_FAILURE);
            }
        }
        printf("Resultado enviado com sucesso!\n");
        
        for(int i = 0; i < num_rows; i++){
            free(matrix[i]);
        }
        free(matrix);
        
        for(int i = 0; i < result_rows; i++){
            free(result[i]);
        }
        free(result);
        
        blocks_processed++;
    }

    close(socket_fd);
    printf("Cliente encerrado.\n");
    return 0;
}