//
// Created by gyank on 24/03/2025.
//

#include "play.h"

#include "login.h"
#include "packet_utils.h"

void join_game(SOCKET clientSocket) {
    Buffer join_packet;
    buffer_init(&join_packet, 256);

    uint8_t packet_id = 0x26;        // Packet ID
    int ent_id = 214;                // random ahh number
    uint8_t gamemode = 1;            // survival
    int dim = 0;                     // overworld
    long hash = 1234567890;          // random ahh not sure what to do with it
    uint8_t max = 5;                 // ignored
    char ltype = 0;                  // string enum? default
    int viewD = 0x2;                 // view distance
    bool debug = 0;                  // Debug flag
    bool res = 1;                    // Result flag

    // Append the data fields
    buffer_append(&join_packet, &packet_id, 1);
    buffer_append(&join_packet, &ent_id, 4);
    buffer_append(&join_packet, &gamemode, 1);
    buffer_append(&join_packet, &dim, 4);
    buffer_append(&join_packet, &hash, 8);
    buffer_append(&join_packet, &max, 1);
    buffer_append(&join_packet, &ltype, 1);
    buffer_append(&join_packet, &viewD, 1);
    buffer_append(&join_packet, &debug, 1);
    buffer_append(&join_packet, &res, 1);

    prepend_packet_length(&join_packet);

    send(clientSocket, join_packet.data, join_packet.size, 0);


    buffer_free(&join_packet);
    player_pos_look(clientSocket);
}


//this should be a dynamic packet probably
void player_pos_look(SOCKET clientSocket) {
    Buffer player_poslook_packet;
    buffer_init(&player_poslook_packet, 512);

    uint8_t packet_id = 0x36;
    double x = 0, y = 0, z = 0;
    float yaw = 0;
    float pitch = 0;
    uint8_t flags = 0;
    int tp_id = 0x00;


    buffer_append(&player_poslook_packet, &packet_id, 1);  // Packet ID
    buffer_append(&player_poslook_packet, &x, sizeof(double));  // x
    buffer_append(&player_poslook_packet, &y, sizeof(double));  // y
    buffer_append(&player_poslook_packet, &z, sizeof(double));  // z
    buffer_append(&player_poslook_packet, &yaw, sizeof(float));  // yaw
    buffer_append(&player_poslook_packet, &pitch, sizeof(float));  // pitch
    buffer_append(&player_poslook_packet, &flags, 1);  // flags
    buffer_append(&player_poslook_packet, &tp_id, 1);  // tp_id

    prepend_packet_length(&player_poslook_packet);

    send(clientSocket, player_poslook_packet.data, player_poslook_packet.size, 0);


    buffer_free(&player_poslook_packet);
}

void send_keepalive(SOCKET clientSocket) {

    Buffer keepalive_packet;
    buffer_init(&keepalive_packet, 64);
    int packet_id = 0x21;
    long keepalive_id = 0x00;
    write_varInt_buffer(&keepalive_packet, packet_id);
    buffer_append(&keepalive_packet, &keepalive_packet, 8);
    prepend_packet_length(&keepalive_packet);
    print_packet(keepalive_packet.data, keepalive_packet.size);
    send(clientSocket, keepalive_packet.data, keepalive_packet.size, 0);
    buffer_free(&keepalive_packet);


}
