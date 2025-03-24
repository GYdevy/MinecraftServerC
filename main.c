#include "socket_utils.h"
#include "handshake.h"
#include "packet_utils.h"
#include <stdio.h>

#define PORT 25565

int main() {
    if (initializeWinSock() != 0) return 1;

    SOCKET serverSocket = createServerSocket();
    if (serverSocket == INVALID_SOCKET) return 1;

    startListening(serverSocket);
    printf("Server listening on port %d\n", PORT);

    while (1) {
        SOCKET clientSocket = acceptClient(serverSocket);
        if (clientSocket != INVALID_SOCKET) {
            handshake(clientSocket);
        }
    }

    cleanup(serverSocket);
    return 0;
}
