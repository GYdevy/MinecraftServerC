#ifndef SERVER_H
#define SERVER_H

#include <stdint.h>
#include <poll.h>  

#define BUFFER_SIZE 16384
#define MAX_CLIENTS 5
#define STATE_NONE -1
extern int sessionCount;
typedef enum {
    STATE_HANDSHAKE,
    STATE_STATUS,
    STATE_LOGIN,
    STATE_PLAY
} ConnectionState;
typedef struct {
    char uuid[37];
    char username[32];
    char skinUrl[256];
    int eid;
    double x, y, z; //current
    float yaw, pitch; //current
    double lastX, lastY, lastZ;  // previous position
    float lastYaw, lastPitch;    // previous rotation

    uint8_t flags;        // Movement flags (X/Y/Z/Y_ROT/X_ROT)
    int lastOnGround;
    int onGround;    // 0 - in air, 1 - on ground       
} Player;
typedef struct ClientSession{
    struct ClientSession* allSessions;  
    int sessionCount;  

    int socket;  
    uint8_t buffer[BUFFER_SIZE];
    int bufferOffset;
    int shouldClose;
    int state;

    uint8_t sendBuffer[BUFFER_SIZE];
    int sendOffset;
    int sendLength;

    Player player;
    char username[32]; //storing username here since this struct is passed around almost all functions.

    char knownPlayers[MAX_CLIENTS][16];
    int knownCount;
} ClientSession;



int initializeServer();
void runServerLoop(int serverSocket);  
int handleClientData(ClientSession *session,ClientSession sessions[]);
void processPacket(ClientSession *session,ClientSession sessions[] ,uint8_t *packet, int packetLength);
void sendPacket(ClientSession *session);

#endif  // SERVER_H
