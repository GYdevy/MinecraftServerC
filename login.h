#ifndef LOGIN_H
#define LOGIN_H

#include <stdint.h>
#include "packet_utils.h"

void handle_login(ClientSession *session, uint8_t *packet);
void send_login_success(ClientSession *session, const char *uuid, const char *username);
unsigned char *create_login_response_data(const char *username, const char *uuid, int *total_length);
int extract_username_and_uuid(unsigned char *packet, unsigned char **username, unsigned char **uuid);
int calculate_payload_length(int username_length, int prefixed_array_length, unsigned char *buffer);
void prepend_packet_length(Buffer *buf);

#endif // LOGIN_H
