#include "socket_utils.h"
#include "server.h"

int main() {
    int serverSocket = initializeServer();
    if (serverSocket == -1) return 1;

    runServerLoop(serverSocket);

    cleanup(serverSocket);
    return 0;
}
