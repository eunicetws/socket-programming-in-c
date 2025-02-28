#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>  
#include <ws2tcpip.h>   
#include <time.h>   

#pragma comment(lib, "Ws2_32.lib")  
#define close(fd) closesocket(fd) 

// Automatic Winsock initialization and cleanup
static void init_winsock() __attribute__((constructor));
static void cleanup_winsock() __attribute__((destructor));

//Task 1
void upper(char buffer[]){
        //declares a pointer named buffer that can hold the address of a character
    for(int i = 0; buffer[i]; i++) {
        buffer[i] = toupper(buffer[i]);    //turns individual character to uppercase
    }
}

void reverse(char buffer[]){
    
    int buffer_length = strlen(buffer); 
    for (size_t i = 0; i < buffer_length / 2; i++) {       
        char temp = buffer[i];                          //copy front 
        buffer[i] = buffer[buffer_length - 1 - i];      //copy back to front
        buffer[buffer_length - 1 - i] = temp;           //copy front to back
    }
    buffer[buffer_length] = '\0';   

}

//Task 2
    // IP, Port : 143
    // length: line 153

void alphanumeric(char buffer[]) { //allows client to repeatedly input meassage to sent to server and receive from servermm
    int alnum = 1;  //flag = true

    // Check if all characters are alphanumeric individually
    for (int i = 0; buffer[i] != '\0'; i++) {
        if (!isalnum(buffer[i])) {
            alnum = 0;  //flag = false (not alnum)
            break;
        }
    }

    if (alnum == 0) {   //not alnum
        strcpy(buffer, "A unique symbol is detected, please enter alphanumeric characters only"); //output error message
    }
}

int exit_server(char buffer[]){
    if (strcmp(buffer, "Exit Server") == 0) {
        strcpy(buffer, "Exiting...");      //reversed, because reversed function is below it
        return 0;
    } else{
        return 1;
    }
}

//Task 3 - Anonymous (2023) //this reference is used for both task 3 and 4
int commands(char buffer[], int non_command){
    time_t rawtime; //raw time format
    struct tm *ptm; //time structure
    time(&rawtime); //get time
    ptm = gmtime(&rawtime);     // is GMT

    if (strcmp(buffer, "Date") == 0) {
        ptm->tm_hour += 8;          //MTC, malaysia time
        mktime(ptm);                //If exceed limit, ex. hour, add to next, ex. day

        strcpy(buffer, asctime(ptm));
        buffer[strcspn(buffer, "\n")] = 0; 
        char time_zone[]=" GMT + 8";
        memcpy(buffer + strlen(buffer), time_zone, strlen(time_zone) + 1);  
    } else {
        return non_command;
    }  
    return 0;
}

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
    address.sin_port = htons(PORT);        // Host to Network Short     ;htons for server addr, ntons for client addr

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Binding failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // 3. Listen for incoming connections
    if (listen(server_fd, 1) > 0) {
        
        perror("Listening failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    printf("Server is listening on port %d...\n", PORT);

    // 4. Accept a connection
    while(1){
        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        if (new_socket == -1) {
            perror("Accepting connection failed");
            close(server_fd);
            exit(EXIT_FAILURE);
        }
        sprintf(buffer, "IP Address: %s, Port: %d\n", inet_ntoa(address.sin_addr), (int) ntohs(address.sin_port)); //- Andrew Barber & Misha. (2014, May 21)   
        printf("%s", buffer);
        memset(buffer, 0, 1024);                            // after send ip and port address, reset buffer

        // 5. Receive data from the client
        while(1){ 
            memset(buffer, 0, 1024);
            int non_command = 1;

            int bytes = recv(new_socket, buffer, sizeof(buffer), 0);
            printf("Buffer Length: %d\n", strlen(buffer));
            printf("Received from client: %s\n", buffer);
            if (bytes==0){      //- freakish. (2017, April 12), prevent unconditional loop even if client is disconnected 
                break;
            }
            non_command = exit_server(buffer);
            if(non_command == 0){
                strcpy(buffer, "Exiting...");
                send(new_socket, buffer, strlen(buffer), 0);
                close(new_socket);
                break;
            }
            non_command = commands(buffer, non_command);
        
            if(non_command == 1){
                reverse(buffer);
                alphanumeric(buffer);
                upper(buffer);
            }

            send(new_socket, buffer, strlen(buffer), 0);
            printf("Sent to client: %s\n", buffer);
        }
    }

    // 7. Close the sockets
    close(server_fd);

    return 0;
}

/* References
Task 3
    Anonymous (2023). Cplusplus.com. https://cplusplus.com/reference/ctime/gmtime/

Task 2
    Andrew Barber & Misha. (2014, May 21). How to get ip address from sock structure in c? Stack Overflow. https://stackoverflow.com/questions/3060950/how-to-get-ip-address-from-sock-structure-in-c
    freakish. (2017, April 12). Do I have to close the socket on the server side after client disconnects? Stack Overflow. https://stackoverflow.com/questions/43368928/do-i-have-to-close-the-socket-on-the-server-side-after-client-disconnects
*/
