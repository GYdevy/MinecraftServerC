#ifndef SERVER_H
#define SERVER_H

#include <stdint.h>
#include <poll.h>  /

#define BUFFER_SIZE 16384
#define MAX_CLIENTS 5

typedef enum {
    STATE_HANDSHAKE,
    STATE_STATUS,
    STATE_LOGIN,
    STATE_PLAY
} ConnectionState;

typedef struct {
    int socket;  
    uint8_t buffer[BUFFER_SIZE];
    int bufferOffset;
    int shouldClose;
    int state;

    uint8_t sendBuffer[BUFFER_SIZE];
    int sendOffset;
    int sendLength;

    char username[32]; //storing username here since this struct is passed around almost all functions.
} ClientSession;

int initializeServer();
void runServerLoop(int serverSocket);  
int handleClientData(ClientSession sessions[]);
void processPacket(ClientSession *session, uint8_t *packet, int packetLength);
void sendPacket(ClientSession *session);

#endif  // SERVER_H
