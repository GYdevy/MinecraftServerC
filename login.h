
#include <stdint.h>
#include <winsock2.h>

#include "packet_utils.h"
#ifndef LOGIN_H
#define LOGIN_H
void handle_login(SOCKET clientSocket);
unsigned char* receive_login(SOCKET clientSocket, int* out_packet_length);
unsigned char* create_login_response_data(unsigned char* packet, int* response_length);
int extract_username_and_uuid(unsigned char* packet, unsigned char** username, unsigned char** uuid);
int calculate_payload_length(int username_length, int prefixed_array_length, unsigned char* buffer);
unsigned char* construct_response(unsigned char* length_prefix, int length_prefix_size, unsigned char* uuid, unsigned char* username, int username_length, unsigned char* prefixed_array_length_buf, int prefixed_array_length, int total_length);
void join_game(SOCKET clientSocket);
void prepend_packet_length(Buffer *buf);
void send_login_success(SOCKET clientSocket, const char *uuid, const char *username)  ;
#endif //LOGIN_H
