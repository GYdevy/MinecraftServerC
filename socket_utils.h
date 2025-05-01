#ifndef SOCKET_UTILS_H
#define SOCKET_UTILS_H

#include <sys/socket.h>  
#include <netinet/in.h>  
#include <arpa/inet.h>   

int initializeServer();
int createServerSocket();
void startListening(int serverSocket);
int acceptClient(int serverSocket);
void cleanup(int serverSocket);

#endif  // SOCKET_UTILS_H
