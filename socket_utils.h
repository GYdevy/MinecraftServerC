
#include <winsock2.h>
#ifndef SOCKET_UTILS_H
#define SOCKET_UTILS_H
int initializeWinSock();
SOCKET createServerSocket();
void startListening(SOCKET serverSocket);
SOCKET acceptClient(SOCKET serverSocket);
void cleanup(SOCKET serverSocket);
#endif //SOCKET_UTILS_H
