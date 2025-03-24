#ifndef PLAY_H
#include <winsock2.h>
#define PLAY_H
void join_game(SOCKET clientSocket);
void player_pos_look(SOCKET clientSocket);
void send_keepalive(SOCKET clientSocket);
#endif //PLAY_H
