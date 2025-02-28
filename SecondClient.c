#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <pthread.h> 

#pragma comment(lib, "Ws2_32.lib")

static void init_winsock() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fprintf(stderr, "WSAStartup failed\n");
        exit(EXIT_FAILURE);
    }
}

static void cleanup_winsock() {
    WSACleanup();
}

#define PORT 8080
pthread_t send_id;
pthread_t receive_id;


void *send_to(void *arg) {
    int client_fd = *(int *)arg;
    char message[1024];
    puts("Connected to the server.");
    puts("Enter \"Exit Server\" to exit");

    while(1){
        memset(message, 0, 1024);
        if (fgets(message, sizeof(message), stdin) == NULL) {
            break;
        }
        message[strcspn(message, "\n")] = 0;    //Tim Čas. (2015, Feb 11)

        send(client_fd, message, strlen(message), 0);
        if(strcmp(message, "Exit Server") == 0){
            break;  
        }
        memset(message, 0, 1024);
        
    }
    return NULL;
}

void *receive_from(void *arg) {
    int client_fd = *(int *)arg;
    char buffer[1024];

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int bytes = recv(client_fd, buffer, 1024, 0);
        
        if (bytes <= 0) {
            if (bytes == 0) {
                printf("Disconnected form server\n");
                closesocket(client_fd);
            }
            break;
        }

        printf("%s \n", buffer);
        memset(buffer, 0, sizeof(buffer));
    }
    return NULL;
}

int main() {
    init_winsock();

    int client_fd;
    struct sockaddr_in server_address;
    char buffer[1024] = {0};

    // Create the socket
    client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd == INVALID_SOCKET) {
        fprintf(stderr, "Socket creation failed: %d\n", WSAGetLastError());
        cleanup_winsock();
        exit(EXIT_FAILURE);
    }

    // Configure server address
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    server_address.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Connect to the server
    if (connect(client_fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        fprintf(stderr, "Connection failed: %d\n", WSAGetLastError());
        closesocket(client_fd);
        cleanup_winsock();
        exit(EXIT_FAILURE);
    }

    // Send data to the server
    int *client_fd_ptr = malloc(sizeof(int));
    *client_fd_ptr = client_fd;
    pthread_create(&send_id, NULL, send_to, (void *)client_fd_ptr);
    pthread_create(&receive_id, NULL, receive_from, (void *)client_fd_ptr);
    pthread_join(send_id, NULL);  
    pthread_join(receive_id, NULL);  

    free(client_fd_ptr);  
    closesocket(client_fd);
    cleanup_winsock();

    return 0;
}

/* 
References
Tim Čas. (2015, Feb 11). Removing trailing newline character from fgets() input. Stack Overflow. 
    https://stackoverflow.com/questions/2693776/removing-trailing-newline-character-from-fgets-input
‌*/