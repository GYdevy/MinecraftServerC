#ifndef SERVER_H
#define SERVER_H

#include <stdint.h>
#include <poll.h>  // For poll()

#define BUFFER_SIZE 16384
#define MAX_CLIENTS 5

typedef enum {
    STATE_HANDSHAKE,
    STATE_STATUS,
    STATE_LOGIN,
    STATE_PLAY
} ConnectionState;

typedef struct {
    int socket;  // SOCKET changed to int
    uint8_t buffer[BUFFER_SIZE];
    int bufferOffset;
    int shouldClose;
    int state;
    uint8_t sendBuffer[BUFFER_SIZE];
    int sendOffset;
    int sendLength;
} ClientSession;

int initializeServer();
void runServerLoop(int serverSocket);  // SOCKET changed to int
int handleClientData(ClientSession sessions[]);
void processPacket(ClientSession *session, uint8_t *packet, int packetLength);
void sendPacket(ClientSession *session);

#endif  // SERVER_H
