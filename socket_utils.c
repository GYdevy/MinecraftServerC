#include "socket_utils.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#define PORT 61243
#pragma comment(lib, "ws2_32.lib")

int initializeWinSock() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed with error: %d\n", WSAGetLastError());
        return 1;
    }
    return 0;
}

SOCKET initializeServer() {
    SOCKET serverSocket = createServerSocket();
    if (serverSocket == INVALID_SOCKET) return INVALID_SOCKET;
    startListening(serverSocket);
    printf("Server listening on port %d\n", PORT);
    return serverSocket;
}

SOCKET createServerSocket() {
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        printf("Socket creation failed with error: %d\n", WSAGetLastError());
        WSACleanup();
        return INVALID_SOCKET;
    }

    struct sockaddr_in serverAddr = {0};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    if (bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        printf("Bind failed with error: %d\n", WSAGetLastError());
        closesocket(serverSocket);
        WSACleanup();
        return INVALID_SOCKET;
    }

    return serverSocket;
}

void startListening(SOCKET serverSocket) {
    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        printf("Listen failed with error: %d\n", WSAGetLastError());
        closesocket(serverSocket);
        WSACleanup();
        exit(1);
    }
}

SOCKET acceptClient(SOCKET serverSocket) {
    struct sockaddr_in clientAddr;
    int addrLen = sizeof(clientAddr);
    SOCKET clientSocket = accept(serverSocket, (struct sockaddr *) &clientAddr, &addrLen);

    if (clientSocket == INVALID_SOCKET) {
        printf("Accept failed with error: %d\n", WSAGetLastError());
    } else {
        printf("Client connected\n");
    }

    return clientSocket;
}

void cleanup(SOCKET serverSocket) {
    closesocket(serverSocket);
    WSACleanup();
}
