#ifndef PLAY_H
#define PLAY_H

#include <stdint.h>
#include <stdbool.h>
#include "packet_utils.h"

void join_game(ClientSession *session);
void send_stone_platform_chunk_at(ClientSession *session, int chunkX, int chunkZ);
void player_pos_look(ClientSession *session);
void send_keep_alive(ClientSession *session, struct pollfd *fd);
void handle_play_state(ClientSession *session,ClientSession sessions[] , int packet_id, uint8_t *packet, int packet_length);
void player_info_packet_join(ClientSession *sourceSession, ClientSession *allSessions);
void player_info_packet_disconnect(ClientSession *sourceSession, ClientSession *allSessions);
void send_3x3_chunks(ClientSession *session);
void spawn_player_packet(ClientSession *sourceSession, ClientSession *spawn_session);
void update_game_tick(ClientSession *sessions, int sessionCount);
void send_all_players_to_newcomer(ClientSession *newSession, ClientSession *allSessions);
void send_newcomer_to_existing_players(ClientSession *newSession, ClientSession *allSessions);
void on_player_join(ClientSession *newSession, ClientSession *allSessions) ;
void on_player_disconnect(ClientSession *discSession, ClientSession *allSessions);
void destroy_disconnect_player(ClientSession *session, ClientSession *allSessions);
#endif // PLAY_H
