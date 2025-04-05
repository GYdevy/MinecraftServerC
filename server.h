#ifndef SERVER_H
#define SERVER_H

#include <stdint.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#define BUFFER_SIZE 16384
#define MAX_CLIENTS 5

typedef enum {
    STATE_HANDSHAKE,
    STATE_STATUS, // <-- New state for handling server list ping
    STATE_LOGIN,
    STATE_PLAY
} ConnectionState;

typedef struct {
    SOCKET socket;
    uint8_t buffer[BUFFER_SIZE];
    int bufferOffset;
    int shouldClose;
    int state;
    // Outgoing packet queue
    uint8_t sendBuffer[BUFFER_SIZE];
    int sendOffset;
    int sendLength;
} ClientSession;

SOCKET initializeServer();

void runServerLoop(SOCKET serverSocket);

int handleClientData(ClientSession sessions[]);

void processPacket(ClientSession *session, uint8_t *packet, int packetLength);

void sendPacket(ClientSession *session);
#endif
