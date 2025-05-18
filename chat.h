#ifndef CHAT_H
#define CHAT_H

#include "server.h"
#include "packet_utils.h"


void send_player_disconnected(ClientSession *session, ClientSession sessions[]);
void handle_chat_packet(ClientSession *session, uint8_t *packet, int packetLength);
char *extractMessageFromPacket(uint8_t *packet, int packetLength);
void broadcastChatMessage(ClientSession *sender, ClientSession sessions[], int sessionCount, const char *message);
void send_player_joined_message(ClientSession *newSession, ClientSession *allSessions);
#endif // CHAT_H