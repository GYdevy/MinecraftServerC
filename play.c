#include "play.h"
#include <stdio.h>
#include "login.h"
#include "packet_utils.h"
#include "server.h"
#include "extract_uuid.h"
#include <string.h>
#include "chat.h"
#include "movement.h"
#include "play_helpers.h"
#include <unistd.h> // For sleep function in Linux

#define HEIGHTMAP_SIZE 36

void join_game(ClientSession *session)
{
    Buffer join_packet;
    buffer_init(&join_packet, 256);

    uint8_t packet_id = PLAY_CLIENTBOUND_JOIN_GAME_PACKET; // Packet ID
    int ent_id = 214;         // random number
    uint8_t gamemode = 1;     // creative
    int dim = 0;              // overworld
    long hash = 1234567890;   // random number for hash
    uint8_t max = 5;          // ignored
    char ltype = 0;           // default
    int viewD = 0x8;          // view distance
    bool debug = 0;           // Debug flag
    bool res = 1;             // Result flag

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

    send(session->socket, join_packet.data, join_packet.size, 0);

    on_player_join(session, session->allSessions);

    buffer_free(&join_packet);
    player_pos_look(session);
}
void on_player_join(ClientSession *newSession, ClientSession *allSessions)
{

    newSession->state = STATE_PLAY;
    player_info_packet_join(newSession, allSessions); // Add player to tab list
    send_existing_players_to_newcomer(newSession, newSession->allSessions);
    send_newcomer_to_existing_players(newSession, allSessions);
    send_player_joined_message(newSession, allSessions);
    // Inform other players of the new player's spawn
    for (int i = 0; i < sessionCount; i++)
    {
        ClientSession *other = &allSessions[i];
        if (other != newSession && other->state == STATE_PLAY)
        {
            spawn_player_packet(other, newSession); // Show new player to others
            spawn_player_packet(newSession, other); // Show others to new player
        }
    }
}
void on_player_disconnect(ClientSession *discSession, ClientSession *allSessions)
{

    player_info_packet_disconnect(discSession, allSessions); // remove player from tab list
    send_player_disconnected(discSession, allSessions);
    destroy_disconnect_player(discSession, allSessions);

    close(discSession->socket);
    discSession->socket = -1;
    discSession->state = STATE_NONE;
    discSession->player.eid = -1;
    memset(discSession->username, 0, sizeof(discSession->username));
}

void destroy_disconnect_player(ClientSession *session, ClientSession *allSessions)
{

    Buffer buf;
    buffer_init(&buf, 64);
    int packet_id = PLAY_CLIENTBOUND_DESTROY_ENTITIES;
    uint8_t count = 0x01;
    write_varInt_buffer(&buf, packet_id);
    buffer_append(&buf, &count, sizeof(count));
    write_varInt_buffer(&buf, session->player.eid);
    prepend_packet_length(&buf);

    for (int i = 0; i < sessionCount; i++)
    {
        ClientSession *other = &allSessions[i];
        if (other != session && other->state == STATE_PLAY)
        {
            buffer_to_sendbuffer(other, &buf);
            sendPacket(other);
        }
    }

    buffer_free(&buf);
}

// Basic stone platform chunk packet.
void send_stone_platform_chunk_at(ClientSession *session, int chunkX, int chunkZ)
{
    Buffer chunkPacket;
    buffer_init(&chunkPacket, 4096);

    bool full_chunk = 1;
    int bit_mask = 1;

    // Packet ID
    write_varInt_buffer(&chunkPacket, PLAY_CLIENTBOUND_CHUNK_DATA_PACKET); 

    // Chunk Coordinates
    buffer_append(&chunkPacket, &chunkX, sizeof(int32_t)); // Chunk X
    buffer_append(&chunkPacket, &chunkZ, sizeof(int32_t)); // Chunk Z

    // Full Chunk
    write_varInt_buffer(&chunkPacket, full_chunk);

    // Primary Bit Mask
    write_varInt_buffer(&chunkPacket, bit_mask);

    // Heightmaps
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
    for (int i = 0; i < 36; i++)
    {
        buffer_append(&chunkPacket, &zero, sizeof(uint64_t));
    }

    // End NBT with TAG_End
    uint8_t tag_end = 0;
    buffer_append(&chunkPacket, &tag_end, 1);

    // Biomes (1024 * int32_t)
    uint32_t biome_id = htonl(1); // Plains biome
    for (int i = 0; i < 1024; i++)
    {
        buffer_append(&chunkPacket, &biome_id, sizeof(uint32_t));
    }

    // Chunk Section Data
    Buffer section;
    buffer_init(&section, 2048);

    write_varInt_buffer(&section, 8);   // Bits per block
    write_varInt_buffer(&section, 3);   // Palette size
    write_varInt_buffer(&section, 256); // Number of palette entries

    // Define palette for stone (ID 1)
    uint32_t stone_block_id = 1;                                // Stone block ID
    buffer_append(&section, &stone_block_id, sizeof(uint32_t)); // Stone palette entry

    write_varInt_buffer(&section, 4096); // Number of blocks
    write_varInt_buffer(&section, 512);  // Number of long values in section (ceil(4096 * 8 / 64))

    // Align buffer before appending stone block data
    buffer_align(&section, 8);

    uint64_t stone_encoded = 0x0101010101010101ULL;
    for (int i = 0; i < 512; i++)
    {
        buffer_append(&section, &stone_encoded, sizeof(uint64_t));
    }

    write_varInt_buffer(&chunkPacket, section.size);
    buffer_append(&chunkPacket, section.data, section.size);

    write_varInt_buffer(&chunkPacket, 0);

    prepend_packet_length(&chunkPacket);

    // Send the chunk data to the player
    send(session->socket, chunkPacket.data, chunkPacket.size, 0);
    buffer_free(&chunkPacket); // Free the chunk packet buffer
}
void send_3x3_chunks(ClientSession *session)
{
    // Calculate the player's current chunk coordinates
    int player_chunk_x = (int)(session->player.x / 16); // Minecraft chunks are 16x16 blocks
    int player_chunk_z = (int)(session->player.z / 16);

    // Send chunks in a 3x3 grid around the player
    for (int dx = -1; dx <= 1; dx++)
    {
        for (int dz = -1; dz <= 1; dz++)
        {
            int chunkX = player_chunk_x + dx; // X coordinate of chunk
            int chunkZ = player_chunk_z + dz; // Z coordinate of chunk

            // Call your function to send a single chunk at (chunkX, chunkZ)
            send_stone_platform_chunk_at(session, chunkX, chunkZ);
        }
    }
}

// Now dynamic
void player_pos_look(ClientSession *session)
{
    Buffer player_poslook_packet;
    buffer_init(&player_poslook_packet, 512);

    uint8_t packet_id = PLAY_CLIENTBOUND_PLAYER_POSITION_LOOK_PACKET;
    double x = session->player.x;
    double y = session->player.y;
    double z = session->player.z;
    float yaw = session->player.yaw;
    float pitch = session->player.pitch;
    while (yaw < 0)
        yaw += 360.0f;
    while (yaw >= 360.0f)
        yaw -= 360.0f;
    uint8_t flags = 0;
    int tp_id = 0x0a;

    buffer_append(&player_poslook_packet, &packet_id, 1);            // Packet ID
    buffer_append_be(&player_poslook_packet, &x, sizeof(x));         // x
    buffer_append_be(&player_poslook_packet, &y, sizeof(y));         // y
    buffer_append_be(&player_poslook_packet, &z, sizeof(z));         // z
    buffer_append_be(&player_poslook_packet, &yaw, sizeof(yaw));     // yaw
    buffer_append_be(&player_poslook_packet, &pitch, sizeof(pitch)); // pitch
    buffer_append(&player_poslook_packet, &flags, 1);                // flags
    buffer_append(&player_poslook_packet, &tp_id, 1);                // tp_id

    prepend_packet_length(&player_poslook_packet);
    send(session->socket, player_poslook_packet.data, player_poslook_packet.size, 0);
    buffer_free(&player_poslook_packet);

    send_3x3_chunks(session);
}

void send_keep_alive(ClientSession *session, struct pollfd *fd)
{
    if (!session || session->socket == -1)
        return; // Invalid socket
    Buffer keepalive_packet;
    buffer_init(&keepalive_packet, 64);
    uint8_t packet_id = PLAY_CLIENTBOUND_KEEP_ALIVE_PACKET;

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

void handle_play_state(ClientSession *session, ClientSession sessions[], int packet_id, uint8_t *packet, int packet_length)
{
    switch (packet_id)
    {
    case PLAY_SERVERBOUND_ANIMATION_PACKET:
    {
        send_player_arm_swing(session, sessions);

        break;
    }
    case 0x05:
    {
        // printf("RECEIVED CLIENT SETTINGS\n");
        break;
    }
    case 0x0B:
    {
        // printf("RECEIVED CLIENT PLUGIN MESSAGE\n");
        break;
    }
    case 0x00:
    {
        // printf("RECEIVED CLIENT TELEPORT CONFIRM\n");
        break;
    }
    case PLAY_SERVERBOUND_KEEP_ALIVE_PACKET:
    {
        // printf("KEEP ALIVE RECEIVED\n");
        break;
    }
    case PLAY_SERVERBOUND_CHAT_MESSAGE_PACKET:
    {

        handle_chat_packet(session, packet, packet_length);
        break;
    }

    case 0x22:
    {
        printf("BEACON?\n");
        break;
    }
        // Player movement and position packets

    case PLAY_SERVERBOUND_PLAYER_POSITION_PACKET:
    { // Player position

        uint8_t *current_packet = packet;
        handle_player_position(session, current_packet);
        break;
    }
    case PLAY_SERVERBOUND_PLAYER_POSITION_ROTATION_PACKET:
    { // Player position and rotation

        uint8_t *current_packet = packet;
        handle_player_position_rotation(session, current_packet);
        break;
    }
    case PLAY_SERVERBOUND_PLAYER_ROTATION_PACKET:
    { // Player Rotation
        uint8_t *current_packet = packet;
        handle_player_rotation(session, current_packet);
        break;
    }
    case PLAY_SERVERBOUND_PLAYER_MOVEMENT_PACKET:
    { // Player movement
        uint8_t *current_packet = packet;
        handle_player_movement(session, current_packet);
        break;
    }
    }
}

// needs to have a filter for the actions or different functions for each action
void player_info_packet_join(ClientSession *sourceSession, ClientSession *allSessions)
{

    Player *find_player = find_player_in_file(sourceSession->username);
    if (!find_player)
    {
        printf("[ERROR] Player not found in file for '%s'.\n", sourceSession->player.username);
        return;
    }

    Buffer buffer;
    buffer_init(&buffer, 512);

    uint8_t packet_id = PLAY_CLIENTBOUND_PLAYER_INFO_PACKET;
    uint8_t action = 0x00; // Add Player
    uint8_t num_of_players = 0x01;
    uint8_t num_of_properties = 0x01;
    uint8_t gamemode = 0x01; // Creative, hardcoded
    uint8_t is_signed = 0x00;
    uint8_t has_display_name = 0x00;
    int ping = 0; // Should be something measured, for now hardcoded

    uint8_t uuid_bytes[16];
    write_varInt_buffer(&buffer, packet_id);
    write_varInt_buffer(&buffer, action);
    write_varInt_buffer(&buffer, num_of_players);

    parse_uuid_bytes(sourceSession->player.uuid, uuid_bytes);
    buffer_append(&buffer, uuid_bytes, 16);

    size_t username_len = strlen(sourceSession->player.username);
    write_varInt_buffer(&buffer, username_len);
    buffer_append(&buffer, sourceSession->player.username, username_len);

    write_varInt_buffer(&buffer, num_of_properties);

    const char *prop_name = "textures";
    size_t prop_name_len = strlen(prop_name);
    write_varInt_buffer(&buffer, prop_name_len);
    buffer_append(&buffer, prop_name, prop_name_len);

    size_t skin_len = strlen(find_player->skinUrl);
    write_varInt_buffer(&buffer, skin_len);
    buffer_append(&buffer, find_player->skinUrl, skin_len);

    buffer_append(&buffer, &is_signed, 1);
    write_varInt_buffer(&buffer, gamemode);
    write_varInt_buffer(&buffer, ping);
    buffer_append(&buffer, &has_display_name, 1);

    prepend_packet_length(&buffer);

    // Ensure session count is valid
    if (sourceSession->sessionCount <= 0)
    {
        printf("[WARNING] No sessions to send the packet to.\n");
        buffer_free(&buffer);
        return;
    }

    if (allSessions == &sourceSession[0])
    {

        sessionCount = 1;
    }
    // Loop through sessions and send the packet
    printf("[DEBUG] Looping through %d sessions.\n", sessionCount);
    for (int i = 0; i < sessionCount; i++)
    {
        ClientSession *target = &allSessions[i];
        if (!target)
        {
            printf("[WARNING] target session %d is NULL.\n", i);
            continue;
        }

        if (target->socket == -1)
        {
            printf("[DEBUG] Skipping session %d (socket -1).\n", i);
            continue;
        }

        printf("[DEBUG] Sending packet to session %d.\n", i);
        buffer_to_sendbuffer(target, &buffer);
        sendPacket(target);
    }

    buffer_free(&buffer);
}

void player_info_packet_disconnect(ClientSession *sourceSession, ClientSession *allSessions)
{

    Player *find_player = find_player_in_file(sourceSession->username);
    if (!find_player)
    {
        printf("[ERROR] Player not found in file for '%s'.\n", sourceSession->player.username);
        return;
    }

    Buffer buffer;
    buffer_init(&buffer, 512);

    uint8_t packet_id = PLAY_CLIENTBOUND_PLAYER_INFO_PACKET;
    uint8_t action = 0x04; // remove Player
    uint8_t num_of_players = 0x01;

    uint8_t uuid_bytes[16];
    write_varInt_buffer(&buffer, packet_id);
    write_varInt_buffer(&buffer, action);
    write_varInt_buffer(&buffer, num_of_players);

    parse_uuid_bytes(sourceSession->player.uuid, uuid_bytes);
    buffer_append(&buffer, uuid_bytes, 16);

    prepend_packet_length(&buffer);

    // Ensure session count is valid
    if (sourceSession->sessionCount <= 0)
    {
        printf("[WARNING] No sessions to send the packet to.\n");
        buffer_free(&buffer);
        return;
    }
    int sessionCount = sourceSession->sessionCount;
    if (allSessions == &sourceSession[0])
    {

        sessionCount = 1;
    }
    // Loop through sessions and send the packet
    printf("[DEBUG] Looping through %d sessions.\n", sourceSession->sessionCount);
    for (int i = 0; i < sourceSession->sessionCount; i++)
    {
        ClientSession *target = &allSessions[i];
        if (!target)
        {
            printf("[WARNING] target session %d is NULL.\n", i);
            continue;
        }

        if (target->socket == -1)
        {
            printf("[DEBUG] Skipping session %d (socket -1).\n", i);
            continue;
        }

        printf("[DEBUG] Sending packet to session %d.\n", i);
        buffer_to_sendbuffer(target, &buffer);
        sendPacket(target);
    }

    buffer_free(&buffer);
}

void send_newcomer_to_existing_players(ClientSession *newSession, ClientSession *allSessions)
{
    for (int i = 0; i < newSession->sessionCount; i++)
    {
        ClientSession *other = &allSessions[i];
        if (other == newSession || other->socket == -1 || other->state != STATE_PLAY)
            continue;

        // This sends `newSession`'s info to the already-connected player
        player_info_packet_join(newSession, other);
        ;
    }
}
void send_existing_players_to_newcomer(ClientSession *newSession, ClientSession *allSessions)
{
    for (int i = 0; i < sessionCount; i++)
    {
        ClientSession *other = &allSessions[i];
        if (other == newSession || other->socket == -1 || other->state != STATE_PLAY)
            continue;

        // This sends `other`'s info to the newcomer
        player_info_packet_join(other, newSession);
    }
}

void spawn_player_packet(ClientSession *sourceSession, ClientSession *spawn_session)
{
    printf("SPAWNING PLAYER %s for player %s\n", spawn_session->player.username, sourceSession->player.username);
    if (sourceSession->state != STATE_PLAY)
    {
        printf("[WARN] Tried to send spawn packet to player not in play state\n");
        return;
    }
    if (strcmp(sourceSession->player.username, spawn_session->player.username) == 0)
    {
        printf("WEIRD SPAWN DETECTED: Same username '%s'\n", sourceSession->player.username);
        return;

    } // LAZY WORKAROUND

    Buffer buffer;
    buffer_init(&buffer, 256);

    uint8_t packet_id = PLAY_CLIENTBOUND_SPAWN_PLAYER_PACKET;
    uint8_t uuid_bytes[16];
    parse_uuid_bytes(spawn_session->player.uuid, uuid_bytes);

    uint8_t yaw_byte = (uint8_t)((spawn_session->player.yaw / 360.0f) * 256.0f);
    uint8_t pitch_byte = (uint8_t)((spawn_session->player.pitch / 360.0f) * 256.0f);

    write_varInt_buffer(&buffer, packet_id); // Packet ID as VarInt
    write_varInt_buffer(&buffer, spawn_session->player.eid);
    buffer_append(&buffer, uuid_bytes, 16);

    write_double_be(&buffer, spawn_session->player.x);
    write_double_be(&buffer, spawn_session->player.y);
    write_double_be(&buffer, spawn_session->player.z);

    buffer_append(&buffer, &yaw_byte, sizeof(uint8_t));
    buffer_append(&buffer, &pitch_byte, sizeof(uint8_t));

    prepend_packet_length(&buffer);
    buffer_to_sendbuffer(sourceSession, &buffer);
    sendPacket(sourceSession);
    buffer_free(&buffer);
}
void update_game_tick(ClientSession *sessions, int sessionCount)
{
    for (int i = 0; i < sessionCount; i++)
    {
        ClientSession *source = &sessions[i];
        if (source->socket == -1 || source->state != STATE_PLAY)
            continue;

        for (int j = 0; j < sessionCount; j++)
        {
            if (i == j)
                continue; // skip sending to self

            ClientSession *target = &sessions[j];
            if (target->socket == -1 || target->state != STATE_PLAY)
                continue;
            broadcast_movement(source, target);
        }
    }
}
