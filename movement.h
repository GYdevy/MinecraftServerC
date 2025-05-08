#ifndef MOVEMENT_H
#define MOVEMENT_H
#include "server.h"
void handle_player_position(ClientSession *session, uint8_t *packet);
void handle_player_position_rotation(ClientSession *session, uint8_t *packet);
void handle_player_rotation(ClientSession *session, uint8_t *packet);
void handle_player_movement(ClientSession *session, uint8_t *packet);


#endif //MOVEMENT_H