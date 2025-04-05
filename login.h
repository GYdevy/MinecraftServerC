
#include <stdint.h>
#include <winsock2.h>

#include "packet_utils.h"
#ifndef LOGIN_H
#define LOGIN_H

void handle_login(SOCKET clientSocket, uint8_t *packet);

unsigned char *create_login_response_data(const char *username, const char *uuid, int *total_length);

int extract_username_and_uuid(unsigned char *packet, unsigned char **username, unsigned char **uuid);

int calculate_payload_length(int username_length, int prefixed_array_length, unsigned char *buffer);

void join_game(SOCKET clientSocket);

void prepend_packet_length(Buffer *buf);

void send_login_success(SOCKET clientSocket, const char *uuid, const char *username);
#endif //LOGIN_H
