#include "socket_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>  // For close()

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 61243

// Function to initialize the server
int initializeServer() {
    int serverSocket = createServerSocket();
    if (serverSocket == -1) return -1;
    startListening(serverSocket);
    printf("Server listening on port %d\n", PORT);
    return serverSocket;
}

// Function to create the server socket
int createServerSocket() {
    // Create the server socket using the POSIX socket API
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        perror("Socket creation failed");
        return -1;
    }


    int opt = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) { //reuse of port, easy testing.
        perror("setsockopt failed");
        close(serverSocket);
        return -1;
    }

    struct sockaddr_in serverAddr = {0};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    // Bind the socket to the address
    if (bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) == -1) {
        perror("Bind failed");
        close(serverSocket);
        return -1;
    }

    return serverSocket;
}

// Function to start listening for client connections
void startListening(int serverSocket) {
    if (listen(serverSocket, SOMAXCONN) == -1) {
        perror("Listen failed");
        close(serverSocket);
        exit(1);
    }
}

// Function to accept client connections
int acceptClient(int serverSocket) {
    struct sockaddr_in clientAddr;
    socklen_t addrLen = sizeof(clientAddr);
    int clientSocket = accept(serverSocket, (struct sockaddr *) &clientAddr, &addrLen);

    if (clientSocket == -1) {
        perror("Accept failed");
    } else {
        printf("Client connected\n");
    }

    return clientSocket;
}

// Cleanup function
void cleanup(int serverSocket) {
    close(serverSocket);
}
