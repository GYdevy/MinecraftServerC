// known_players.h

#ifndef KNOWN_PLAYERS_H
#define KNOWN_PLAYERS_H

#include "server.h"  // Make sure this has ClientSession declaration

#define USERNAME_LEN 16
#define MAX_KNOWN_PLAYERS 5

int is_known_player_by_name(ClientSession *session, const char *username);
void add_known_player_by_name(ClientSession *session, const char *username);
void remove_known_player_by_name(ClientSession *session, const char *username);
void is_player_in_view(ClientSession *sourceSession, ClientSession *allSessions);
int load_player_from_file(const char *filename, const char *search_username, ClientSession *session);
void normalize_yaw(ClientSession *session);
#endif // KNOWN_PLAYERS_H
