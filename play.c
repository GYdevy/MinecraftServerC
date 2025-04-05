#include "play.h"

#include <stdio.h>

#include "login.h"
#include "packet_utils.h"
#include "server.h"
#define HEIGHTMAP_SIZE 36

void join_game(SOCKET clientSocket) {
    Buffer join_packet;
    buffer_init(&join_packet, 256);

    uint8_t packet_id = 0x26; // Packet ID
    int ent_id = 214; // random ahh number
    uint8_t gamemode = 1; // creative
    int dim = 0; // overworld
    long hash = 1234567890; // random ahh not sure what to do with it
    uint8_t max = 5; // ignored
    char ltype = 0; // string enum? default
    int viewD = 0x2; // view distance
    bool debug = 0; // Debug flag
    bool res = 1; // Result flag

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

//Basic stone platform chunk packet.
void send_stone_platform_chunk(SOCKET socket) {
    Sleep(500);
    Buffer chunkPacket;
    buffer_init(&chunkPacket, 4096);

    int chunkX = 0;
    int chunkZ = 0;
    bool full_chunk = 1;
    int bit_mask = 1;


    //Packet ID
    write_varInt_buffer(&chunkPacket, 0x22); // VarInt for packet ID 0x22

    //Chunk Coordinates
    buffer_append(&chunkPacket, &chunkX, sizeof(int32_t));
    buffer_append(&chunkPacket, &chunkZ, sizeof(int32_t));

    //Full Chunk
    write_varInt_buffer(&chunkPacket, full_chunk);

    //Primary Bit Mask
    write_varInt_buffer(&chunkPacket, bit_mask);

    //Heightmaps
    uint8_t tag_compound = 0x0A; // TAG_Compound
    buffer_append(&chunkPacket, &tag_compound, 1);

    uint16_t name_length = htons(0); // Root compound has empty name
    buffer_append(&chunkPacket, &name_length, 2);

    uint8_t tag_long_array = 0x0C;
    buffer_append(&chunkPacket, &tag_long_array, 1);

    uint16_t name_len = htons(15);
    buffer_append(&chunkPacket, &name_len, 2);
    buffer_append(&chunkPacket, "MOTION_BLOCKING", 15);

    int32_t long_array_length = htonl(36);
    buffer_append(&chunkPacket, &long_array_length, sizeof(int32_t));
    uint64_t zero = 0;
    for (int i = 0; i < 36; i++) {
        buffer_append(&chunkPacket, &zero, sizeof(uint64_t));
    }

    // End NBT with TAG_End
    uint8_t tag_end = 0;
    buffer_append(&chunkPacket, &tag_end, 1);


    // Biomes (1024 * int32_t)
    uint32_t biome_id = htonl(1); // Plains biome
    for (int i = 0; i < 1024; i++) {
        buffer_append(&chunkPacket, &biome_id, sizeof(uint32_t));
    }


    // Chunk Section Data
    Buffer section;
    buffer_init(&section, 2048);

    write_varInt_buffer(&section, 8); // Bits per block
    write_varInt_buffer(&section, 3); // Palette size
    write_varInt_buffer(&section, 256); // Number of palette entries

    // Define palette for stone (ID 1)
    uint32_t stone_block_id = 1; // Stone block ID
    buffer_append(&section, &stone_block_id, sizeof(uint32_t)); // Stone palette entry


    write_varInt_buffer(&section, 4096); // Number of blocks
    write_varInt_buffer(&section, 512); // Number of long values in section (ceil(4096 * 8 / 64))

    // Align buffer before appending stone block data
    buffer_align(&section, 8);

    uint64_t stone_encoded = 0x0101010101010101ULL;
    for (int i = 0; i < 512; i++) {
        buffer_append(&section, &stone_encoded, sizeof(uint64_t));
    }

    // Append Data Size and actual section data
    write_varInt_buffer(&chunkPacket, section.size);
    buffer_append(&chunkPacket, section.data, section.size);


    //Empty block entities
    write_varInt_buffer(&chunkPacket, 0);

    prepend_packet_length(&chunkPacket);


    send(socket, chunkPacket.data, chunkPacket.size, 0);
}


//this should be a dynamic packet with x,y,z coords
void player_pos_look(SOCKET clientSocket) {
    Buffer player_poslook_packet;
    buffer_init(&player_poslook_packet, 512);

    uint8_t packet_id = 0x36;
    double x = 5, y = 17, z = 5;
    float yaw = 0;
    float pitch = 0;
    uint8_t flags = 0;
    int tp_id = 0x0a;

    buffer_append(&player_poslook_packet, &packet_id, 1); // Packet ID
    buffer_append_be(&player_poslook_packet, &x, sizeof(x)); // x
    buffer_append_be(&player_poslook_packet, &y, sizeof(y)); // y
    buffer_append_be(&player_poslook_packet, &z, sizeof(z)); // z
    buffer_append_be(&player_poslook_packet, &yaw, sizeof(yaw)); // yaw
    buffer_append_be(&player_poslook_packet, &pitch, sizeof(pitch)); // pitch
    buffer_append(&player_poslook_packet, &flags, 1); // flags
    buffer_append(&player_poslook_packet, &tp_id, 1); // tp_id

    prepend_packet_length(&player_poslook_packet);
    send(clientSocket, player_poslook_packet.data, player_poslook_packet.size, 0);
    buffer_free(&player_poslook_packet);
    send_stone_platform_chunk(clientSocket);
}


void send_keep_alive(ClientSession *session, struct pollfd *fd) {
    if (!session || session->socket == INVALID_SOCKET) return;
    Buffer keepalive_packet;
    buffer_init(&keepalive_packet, 64);
    uint8_t packet_id = 0x21;

    buffer_append(&keepalive_packet, &packet_id, 1);
    uint64_t keepAliveID = getCurrentTimeMillis(); // Generate keep-alive ID

    buffer_append(&keepalive_packet, &keepAliveID, sizeof(uint64_t));

    prepend_packet_length(&keepalive_packet);
    // Add to send buffer
    memcpy(session->sendBuffer, keepalive_packet.data, keepalive_packet.size);
    session->sendOffset = 0;

    session->sendLength = keepalive_packet.size;

    // Enable POLLOUT to send data
    fd->events |= POLLOUT;
    buffer_free(&keepalive_packet);
}

//TODO handle with update game tick
void handle_play_state(ClientSession *session, int packet_id, uint8_t *packet, int packet_length) {
    switch (packet_id) {
        case 0x05: {
            printf("RECEIVED CLIENT SETTINGS\n"); //IDK what to do with it yet
        }
        case 0x0B: {
            printf("RECEIVED CLIENT PLUGIN MESSAGE\n");
        }
        case 0x00: {
            printf("RECEIVED CLIENT TELEPORT CONFIRM\n");
        }
        case 0x0F: {
            printf("KEEP ALIVE RECEIVED\n");
        }
        case 0x22: {
            printf("RECEIVED PLAYER LOOK AND POS\n");
        }
    }
}
