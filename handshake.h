//
// Created by gyank on 30/01/2025.
//
#include <winsock2.h>

#include "packet_utils.h"
#ifndef HANDSHAKE_H
#define HANDSHAKE_H
int handleHandshake(SOCKET clientSocket);
int parsePacket(SOCKET clientSocket);
void handle_ping_pong(SOCKET clientSocket);
void close_ping_pong_connection(SOCKET clientSocket);
void handshake(SOCKET clientSocket);
Buffer build_status_response();
#endif //HANDSHAKE_H
