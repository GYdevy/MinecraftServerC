#include "server.h"
#include "packet_utils.h"


void handle_player_position(ClientSession *session, uint8_t *packet) {
    packet += 1;
    double x = convert_double_little_endian_to_big_endian(packet);
    packet += 8; 
    double y = convert_double_little_endian_to_big_endian(packet);
    packet += 8;  
    double z = convert_double_little_endian_to_big_endian(packet);
    packet += 8;  

    
    uint8_t onGround = *packet;

    session->player.x = x;
    session->player.y = y;
    session->player.z = z;
    session->player.onGround = onGround;

    
}
void handle_player_position_rotation(ClientSession *session, uint8_t *packet){
    packet += 1;
    double x = convert_double_little_endian_to_big_endian(packet);
    packet += 8; 
    double y = convert_double_little_endian_to_big_endian(packet);
    packet += 8;  
    double z = convert_double_little_endian_to_big_endian(packet);
    packet += 8;  
    float yaw = convert_float_little_endian_to_big_endian(packet);
    packet += 4;
    float pitch = convert_float_little_endian_to_big_endian(packet);
    packet += 4;

    
    uint8_t onGround = *packet;

    session->player.x = x;
    session->player.y = y;
    session->player.z = z;
    session->player.yaw = yaw;
    session->player.pitch = pitch;
    session->player.onGround = onGround;
}
void handle_player_rotation(ClientSession *session, uint8_t *packet){
    packet +=1;
    float yaw = convert_float_little_endian_to_big_endian(packet);
    packet += 4;
    float pitch = convert_float_little_endian_to_big_endian(packet);
    packet += 4;

    uint8_t onGround = *packet;
    session->player.yaw = yaw;
    session->player.pitch = pitch;
    session->player.onGround = onGround;
}
void handle_player_movement(ClientSession *session, uint8_t *packet){
    packet += 1;
    uint8_t onGround = *packet;
    session->player.onGround = onGround;
}