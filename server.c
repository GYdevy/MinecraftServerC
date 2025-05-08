#include "server.h"
#include "socket_utils.h"
#include "handshake.h"
#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include "login.h"
#include "play.h"
#include <unistd.h> // For close()
#include <poll.h>   // For poll()
#include <string.h>
#define PORT 61243
#define TIMEOUT 100 // Poll indefinitely
#define KEEP_ALIVE_INTERVAL 15000
#define TICK_INTERVAL 50
#define SAVE_INTERVAL 5000

void removeClient(struct pollfd fds[], ClientSession sessions[], int *nfds, int index)
{
    printf("[INFO] Removing client %d\n", fds[index].fd);
    close(fds[index].fd);

    if (index < *nfds - 1)
    {
        fds[index] = fds[*nfds - 1];
        sessions[index - 1] = sessions[*nfds - 2];
    }
    (*nfds)--;
}

void handleNewConnection(int serverSocket, struct pollfd fds[], ClientSession sessions[], int *nfds)
{
    int clientSocket = acceptClient(serverSocket);
    if (clientSocket == -1)
        return;

    if (*nfds < MAX_CLIENTS + 1)
    {
        // Initialize the new session with the socket and state
        sessions[*nfds - 1] = (ClientSession){
            .socket = clientSocket,
            .state = STATE_HANDSHAKE,
            .allSessions = sessions,
            .sessionCount = *nfds};

        // Add the new session's socket to the pollfd array
        fds[*nfds].fd = clientSocket;
        fds[*nfds].events = POLLIN;

        (*nfds)++; // Increase the number of clients
    }
    else
    {
        printf("Server full. Rejecting connection.\n");
        close(clientSocket);
    }
}

void processPacket(ClientSession *session, uint8_t *packet, int packetLength)
{
    // Read packet ID
    int offset = 0;
    int packetId = read_varint(packet, &offset);

    // Process the packet based on the packet ID and session state
    switch (session->state)
    {
    case STATE_HANDSHAKE:
        if (packetId == 0x00)
        {
            printf("[INFO] Handshake packet received.\n");
            handshake(session, packet, packetLength);
        }
        else
        {
            printf("[WARN] Unexpected packet ID %d in Handshake state.\n", packetId);
        }
        break;

    case STATE_STATUS:
        // Handle status packets
        if (packetId == 0x00)
        {
            build_send_status_response(session->socket);
            // Handle ping-pong logic, if applicable
            handle_ping_pong(session->socket);
            session->shouldClose = 1;
        }
        else
        {
            printf("[WARN] Unexpected packet ID %d in Status state.\n", packetId);
        }
        break;

    case STATE_LOGIN:
        // Handle login packets
        if (packetId == 0x00)
        {
            printf("[INFO] Login packet received.\n");
            handle_login(session, packet);
            session->state = STATE_PLAY;
        }
        else
        {
            printf("[WARN] Unexpected packet ID %d in Login state.\n", packetId);
        }
        break;

    case STATE_PLAY:
        // Handle play packets
        handle_play_state(session, packetId, packet, packetLength);
        break;

    default:
        printf("[ERROR] Unknown state.\n");
        break;
    }
}

int handleClientData(ClientSession *session)
{
    int bytesRead = recv(session->socket, (char *)(session->buffer + session->bufferOffset),
                         BUFFER_SIZE - session->bufferOffset, 0);

    if (bytesRead <= 0)
        return 1; // Client disconnected or error

    session->bufferOffset += bytesRead;

    while (session->bufferOffset > 0)
    {
        int offset = 0;

        // Read the packet length (VarInt) to get the size of the packet
        int packetLength = read_varint(session->buffer, &offset);
        if (packetLength == -1)
        {
            printf("[ERROR] Invalid packet length\n");
            return 1;
        }

        // Ensure we have enough data for the current packet
        if (packetLength + offset > session->bufferOffset)
        {
            // Incomplete packet, not enough data yet
            printf("[DEBUG] Incomplete packet, expected %d, but only %d bytes available.\n",
                   packetLength, session->bufferOffset - offset);
            break;
        }

        uint8_t packetBuffer[packetLength];
        memcpy(packetBuffer, session->buffer + offset, packetLength);

        // Process the extracted packet
        // printf("[DEBUG] Processing packet at offset %d, length %d\n", offset, packetLength);
        processPacket(session, packetBuffer, packetLength);

        int remainingBytes = session->bufferOffset - (offset + packetLength);
        if (remainingBytes > 0)
        {
            
            
            memmove(session->buffer, session->buffer + (offset + packetLength), remainingBytes);
            session->bufferOffset = remainingBytes;
            
        }
        else
        {
            session->bufferOffset = 0;
        }
    }

    return session->shouldClose ? 1 : 0;
}

void handleClientCommunication(struct pollfd fds[], ClientSession sessions[], int *nfds)
{
    for (int i = 1; i < *nfds; i++)
    {
        ClientSession *session = &sessions[i - 1];

        // Handle incoming data (POLLIN)
        if (fds[i].revents & POLLIN)
        {
            if (handleClientData(session))
            {
                removeClient(fds, sessions, nfds, i);
                i--;
                continue;
            }
        }

        // Handle outgoing data (POLLOUT)
        if (fds[i].revents & POLLOUT)
        {
            sendPacket(session);
            if (session->shouldClose)
            {
                removeClient(fds, sessions, nfds, i);
                i--;
                continue;
            }
        }
    }
}

void runServerLoop(int serverSocket)
{
    struct pollfd fds[MAX_CLIENTS + 1] = {{.fd = serverSocket, .events = POLLIN}};
    ClientSession sessions[MAX_CLIENTS] = {0};
    int nfds = 1;
    uint64_t lastKeepAlive = getCurrentTimeMillis();
    uint64_t lastTickTime = getCurrentTimeMillis();
    uint64_t lastSaveTime = getCurrentTimeMillis();
    while (1)
    {
        uint64_t now = getCurrentTimeMillis();
        while (now - lastTickTime >= TICK_INTERVAL)
        {
            for (int i = 1; i < nfds; i++)
            {
                // update_game_tick();
            }
            lastTickTime += TICK_INTERVAL;
        }

        if (now - lastKeepAlive >= KEEP_ALIVE_INTERVAL)
        {
            for (int i = 1; i < nfds; i++)
            {
                send_keep_alive(&sessions[i - 1], &fds[i]);
            }
            lastKeepAlive = now;
        }
        if (now - lastSaveTime >= SAVE_INTERVAL)
        {
            for (int i = 1; i < nfds; i++)
            {
                printf("Saving players\n");
                save_player_to_file(&sessions[i - 1]);
            }
            lastSaveTime = now;
        }
        if (fds[0].revents & POLLIN)
        {
            handleNewConnection(serverSocket, fds, sessions, &nfds);
        }

        handleClientCommunication(fds, sessions, &nfds);

        if (poll(fds, nfds, TIMEOUT) < 0)
            break;
    }
}

void sendPacket(ClientSession *session)
{
    if (session->sendLength <= 0)
        return;

    int bytesSent = send(session->socket, (char *)(session->sendBuffer + session->sendOffset),
                         session->sendLength - session->sendOffset, 0);

    if (bytesSent <= 0)
    {
        printf("[ERROR] Sending failed, closing client\n");
        session->shouldClose = 1;
        return;
    }

    session->sendOffset += bytesSent;

    if (session->sendOffset >= session->sendLength)
    {
        session->sendOffset = 0;
        session->sendLength = 0;
    }
}
