#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 8192
#define IMAGE_SIZE 2000
#define SERVER_PORT 8080
#define SERVER_IP "127.0.0.1"

int main() {
    int socket_fd;
    struct sockaddr_in server_addr;
    int dimensions[6] = {0};
    int **matrix;

    const char  *msg = "Client message invited!";

    if((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);

    if(inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0){
        printf("\nIp address not suported or invalid");
        close(socket_fd);
        exit(EXIT_FAILURE);
    }

    if(connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Conexion error");
        close(socket_fd);
        exit(EXIT_FAILURE);
    }

    printf("Connected sucssecfully with server in %s:%d\n", SERVER_IP, SERVER_PORT);

    printf("Sending message: '%s'\n", msg);
    write(socket_fd, msg, strlen(msg));

    int valread = read(socket_fd, dimensions, 2*sizeof(int));

    if(valread > 0){
        if(dimensions[2]==1 || dimensions[3]==1){
            int **matrix = malloc((dimensions[0]+1)*sizeof(int *));
        }
        else{
            int **matrix = malloc(dimensions[0]*sizeof(int *));
        }
        if(dimensions[4]==1 || dimensions[5]==1){
            for(int i=0;i<dimensions[0];i++){
                matrix[i] = malloc((dimensions[1]+1)*sizeof(int));
            }
        }
        else{
            for(int i=0;i<dimensions[0];i++){
                matrix[i] = malloc(dimensions[1]*sizeof(int));
            }
        }

        // Atribuindo os valores para cada espaço da imagem
        for(int i=0;i<dimensions[0];i++){
            for(int j=0;j<dimensions[1];j++){
                int val_matrix = read(socket_fd, matrix[i][j], sizeof(int));
                if(val_matrix < 0){
                    perror("Invalid number read from server");
                    exit(EXIT_FAILURE);
                }
            }
        }

        // Processing matrix
        if(dimensions[0] < 2000)
        for(int i=0;i<dimensions[0];i++){
            for(int j=0;j<dimensions[1];j++){

            }
        }

    }
    else{
        printf("Server dont send a anwser or closed connection.\n");
    }

    close(socket_fd);

    return 0;
}