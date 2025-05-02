#ifndef PLAY_H
#define PLAY_H

#include <stdint.h>
#include <stdbool.h>
#include "packet_utils.h"

void join_game(ClientSession *session);
void send_stone_platform_chunk(ClientSession *session);
void player_pos_look(ClientSession *session);
void send_keep_alive(ClientSession *session, struct pollfd *fd);
void handle_play_state(ClientSession *session, int packet_id, uint8_t *packet, int packet_length);
void parse_send_chat(ClientSession *session, uint8_t *packet);
#endif // PLAY_H
