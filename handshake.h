//
// Created by gyank on 30/01/2025.
//
#include <winsock2.h>

#include "packet_utils.h"
#include "server.h"
#ifndef HANDSHAKE_H
#define HANDSHAKE_H

int parsePacket(SOCKET clientSocket);

void handle_ping_pong(SOCKET clientSocket);

void handshake(ClientSession *session, uint8_t *data, int length);

void build_send_status_response(SOCKET clientSocket);
#endif //HANDSHAKE_H
