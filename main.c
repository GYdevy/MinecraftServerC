#include "socket_utils.h"
#include "server.h"

int main() {
    if (initializeWinSock() != 0) return 1;

    SOCKET serverSocket = initializeServer();
    if (serverSocket == INVALID_SOCKET) return 1;

    runServerLoop(serverSocket);

    cleanup(serverSocket);
    return 0;
}
