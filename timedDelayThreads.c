//Included pthread
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#define sleep(x) Sleep(1000 * (x))
typedef SOCKET socket_t;
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
typedef int socket_t;
#endif

#define PORT 8080

// Struct to pass client info to threads
typedef struct {
    socket_t socket;
    int client_id;
} client_info_t;

// Thread function
void *handle_client(void *arg) {
    client_info_t *info = (client_info_t *)arg;
    socket_t client_socket = info->socket;
    int client_id = info->client_id;
    free(info);

    char *message = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 13\r\nConnection: close\r\n\r\nHello Client!";
    
    // Simulating "bottleneck" or heavy processing
    printf("[Server] Handling client %d...\n", client_id);
    printf("[Server] Processing request for 5 seconds...\n");
    sleep(5); // Simulate bottleneck

    send(client_socket, message, (int)strlen(message), 0);
    printf("[Server] Response sent to client %d. Closing connection.\n", client_id);

#ifdef _WIN32
    closesocket(client_socket);
#else
    close(client_socket);
#endif

    return NULL;
}

int main() {
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

    socket_t server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    int addr_len = sizeof(client_addr);
    int client_count = 0;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 5) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);
    printf("This is a MULTITHREADED server. Each client will be handled concurrently.\n\n");

    while (1) {
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);
        if (client_socket != -1) {
            client_count++;
             // This is the sequential bottleneck:
            pthread_t thread_id;
            client_info_t *info = malloc(sizeof(client_info_t));
            if (!info) {
                perror("Failed to allocate memory");
#ifdef _WIN32
                closesocket(client_socket);
#else
                close(client_socket);
#endif
                continue;
            }

            info->socket = client_socket;
            info->client_id = client_count;

            if (pthread_create(&thread_id, NULL, handle_client, info) != 0) {
                perror("Failed to create thread");
                free(info);
            } else {
                pthread_detach(thread_id); // auto cleanup
            }
        }
    }

#ifdef _WIN32
    closesocket(server_socket);
    WSACleanup();
#else
    close(server_socket);
#endif

    return 0;
}