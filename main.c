#include "socket_utils.h"
#include "server.h"
#include <stdlib.h>
int main() {
    int serverSocket = initializeServer();
    if (serverSocket == -1) return 1;
    srand(time(NULL));
    runServerLoop(serverSocket);

    cleanup(serverSocket);
    return 0;
}
