#ifndef LOGIN_H
#define LOGIN_H

#include <stdint.h>
#include "packet_utils.h"

void handle_login(ClientSession *session, uint8_t *packet);
void send_login_success(ClientSession *session, const char *uuid ,const char *formatted_uuid, const char *username);
unsigned char *create_login_response_data(const char *username, const char *uuid, int *total_length);
int extract_username_and_uuid(unsigned char *packet, unsigned char **username, unsigned char **uuid);
int calculate_payload_length(int username_length, int prefixed_array_length, unsigned char *buffer);
void prepend_packet_length(Buffer *buf);
Player* find_player_in_file(const char* username);
void fetch_player_into_session(ClientSession *session, Player *player);
int generate_eid();
int save_player_to_file(ClientSession *session);
int create_players_directory();
#endif // LOGIN_H
