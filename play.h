#include "server.h"
#ifndef PLAY_H
#include <winsock2.h>
#define PLAY_H

void join_game(SOCKET clientSocket);

void handle_play_state(ClientSession *session, int packet_id, uint8_t *packet, int packet_length);

void player_pos_look(SOCKET clientSocket);

void send_keep_alive(ClientSession *session, struct pollfd *fd);

void send_stone_platform_chunk(SOCKET socket);

void sendUpdateLight(SOCKET socket);

uint64_t htobe64(uint64_t val);

void write_heightmap_nbt_to_file(const char *filename);
#endif //PLAY_H
