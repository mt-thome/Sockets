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
#define rows 0
#define coll 1
#define up 2
#define down 3
#define left 4
#define rigth 5
#define SERVER_IP "127.0.0.1"

int main() {
    int socket_fd;
    struct sockaddr_in server_addr;
    // dimensions is [rows][colluns][up][down][left][rigth]
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
    // dimensions is [rows][colluns][up][down][left][rigth]
    
    if(valread > 0){
        // declaration of awnser matrix
        int a_matrix[dimensions[rows]][dimensions[coll]];

        // Declaration of matrix received
        matrix = malloc((dimensions[rows]+2)*sizeof(int *));
        for(int i=0;i<dimensions[rows]+2;i++)
            matrix[i] = malloc((dimensions[coll])*sizeof(int));
        
        // Declaration of aux variables to ghost border
        int m=1, n=1, m_max=dimensions[rows]-1, n_max=dimensions[coll]-1;

        // Treating these variables to ghost border
        if(dimensions[up]>0)
            m--;
        if(dimensions[down]>0)
            m_max++;
        if(dimensions[left]>0)
            n--;
        if(dimensions[rigth]>0)
            n_max++;

        // Getting image from server
        for(int i=m;i<m_max;i++){
            for(int j=n;j<n_max;j++){
                int val_matrix = read(socket_fd, matrix[i][j], sizeof(int));
                if(val_matrix < 0){
                    perror("Invalid number read from server");
                    exit(EXIT_FAILURE);
                }
            }
        }

        // Treating ghost border
        if(dimensions[up]==0){
            for(int j=1;j<n_max;j++)
                matrix[0][j] = matrix[1][j];
        }
        if(dimensions[down]==0){
            for(int j=1;j<n_max;j++)
                matrix[m_max+1][j] = matrix[m_max][j];
        }
        if(dimensions[left]==0){
            for(int j=1;j<m_max;j++)
                matrix[j][0] = matrix[j][1];
        }
        if(dimensions[rigth]==0){
            for(int j=1;j<m_max;j++)
                matrix[j][n_max+1] = matrix[j][n_max];
        }

        // dimensions is [rows][colluns][up][down][left][rigth]        
        // pode dar errado isso aki
        for(int i=0;i<dimensions[rows];i++){
            for(int j=0;j<dimensions[coll];j++){
                a_matrix[i][j] = (int)(matrix[i][j] + matrix[i+1][j] + matrix[i-1][j] + matrix[i][j-1] + matrix[i][j+1])/5;
            }
        }

        send(socket_fd, a_matrix, sizeof(int)*dimensions[rows]*dimensions[coll], 0);
    }
    else{
        printf("Server dont send a anwser or closed connection.\n");
    }

    close(socket_fd);

    return 0;
}