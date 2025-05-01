#ifndef PLAY_H
#define PLAY_H

#include <stdint.h>
#include <stdbool.h>
#include "packet_utils.h"

// Function prototypes

// Join the game packet
void join_game(int clientSocket);

// Send a basic stone platform chunk packet
void send_stone_platform_chunk(int socket);

// Send player position and look packet
void player_pos_look(int clientSocket);

// Send a keep-alive packet
void send_keep_alive(ClientSession *session, struct pollfd *fd);

// Handle play state packets
void handle_play_state(ClientSession *session, int packet_id, uint8_t *packet, int packet_length);

#endif // PLAY_H
