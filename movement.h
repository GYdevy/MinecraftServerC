#ifndef MOVEMENT_H
#define MOVEMENT_H
#include "server.h"
void handle_player_position(ClientSession *session, uint8_t *packet);
void handle_player_position_rotation(ClientSession *session, uint8_t *packet);
void handle_player_rotation(ClientSession *session, uint8_t *packet);
void handle_player_movement(ClientSession *session, uint8_t *packet);
void update_last_pos(ClientSession *session);
void update_last_rot(ClientSession *session);
void broadcast_movement(ClientSession *source, ClientSession *target);
void send_entity_position_packet(ClientSession *source, ClientSession *target, short deltaX, short deltaY, short deltaZ);
void send_entity_rotation_packet(ClientSession *source, ClientSession *target);
void send_entity_position_rotation_packet(ClientSession *source, ClientSession *target, short deltaX, short deltaY, short deltaZ);
void send_entity_movement(ClientSession *source, ClientSession *target);
void send_entity_head_rotation_packet(ClientSession *source, ClientSession *target, float yaw_degrees);
#endif //MOVEMENT_H