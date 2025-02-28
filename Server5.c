#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>  
#include <ws2tcpip.h>  
#include <pthread.h> 

#pragma comment(lib, "Ws2_32.lib")  

#define close(fd) closesocket(fd) 

// Automatic Winsock initialization and cleanup
static void init_winsock() __attribute__((constructor));
static void cleanup_winsock() __attribute__((destructor));

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

#define PORT 8080  // Port number for the server
#define MAX_CLIENTS 2

typedef struct {
    int socket;
    int id;
} client_info;

client_info clients[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_t thread_id;

void *handle_client(void *arg){     //- handle [i] client 
    char buffer[1024];
    char message[1048];
    client_info *cli = (client_info *)arg;

    while(1){
        memset(buffer, 0, sizeof(buffer));                  //reset buffer
        int bytes = recv(cli->socket, buffer, 1024, 0);
        sprintf(message, "Client %d: %s", cli->id, buffer);
        
        if (bytes <= 0 || strcmp(buffer, "Exit Server") == 0){     // delete client
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if(clients[i].id == cli->id){               
                    close(clients[i].socket);
                    clients[i].socket = 0;
                    clients[i].id = 0;
                    sprintf(message, "Client %d disconnected",cli->id);  //tell other client, client[i] was deleted
                    break;
                }
            }
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].socket != 0 && clients[i].id != cli->id) {       //send to all except this client
                send(clients[i].socket, message, strlen(message), 0);
            }
        } 

        if (bytes <= 0 || strcmp(buffer, "Exit Server") == 0){        // end while
            break;
        }
    }
    free(cli);
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};

    // 1. Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // 2. Bind socket to an IP/Port
    address.sin_family = AF_INET;          // IPv4
    address.sin_addr.s_addr = INADDR_ANY;  // Any IP address
    address.sin_port = htons(PORT);        // Host to Network Short

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Binding failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // 3. Listen for incoming connections
    if (listen(server_fd, 1) < 0) {
        
        perror("Listening failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    printf("Server is listening on port %d...\n", PORT);

    // 4. Accept a connection
    while (1) {
        pthread_mutex_lock(&clients_mutex); //lock till new socket added
        
        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        if (new_socket == -1) {
            perror("Accepting connection failed");
            close(server_fd);
            exit(EXIT_FAILURE);
        }
        
        int found = 0;
        int index;
        for (int i = 0; i < MAX_CLIENTS; i++) {     //add in empty area of array
            if (clients[i].socket == 0) {;
                clients[i].socket = new_socket;
                clients[i].id = i;
                found = 1;
                index = i;
                char message[1024];
                sprintf(message, "Cient %d joined", i);
                for (int j = 0; j < MAX_CLIENTS; j++) {
                    if (clients[j].socket != 0) {  // Send to all active clients a client joined
                        send(clients[j].socket, message, strlen(message), 0);
                    }
                }
                break;
            }
        }
        if(found == 0){
            char message[] = "Server is full";
            send(new_socket, message, strlen(message), 0);      //tell client server is full before closing connection
            close(new_socket);
            continue;
        }
        client_info *new_client = malloc(sizeof(client_info));
        new_client->socket = new_socket;
        new_client->id = index;

        pthread_create(&thread_id, NULL, handle_client, (void *)new_client);
        pthread_detach(thread_id);
        pthread_mutex_unlock(&clients_mutex);
    }

    // 7. Close the sockets
    close(server_fd);
    return 0;
}

/* References
nikhilroxtomar. (2019) Chatroom-in-C/Chatroom-in-C. GitHub. https://github.com/nikhilroxtomar/Chatroom-in-C/blob/master/server.c
    // was used as a base for code but was heavily modified
*/
