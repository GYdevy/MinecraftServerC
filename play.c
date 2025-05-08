#include "play.h"
#include <stdio.h>
#include "login.h"
#include "packet_utils.h"
#include "server.h"
#include "extract_uuid.h"
#include <string.h>
#include "movement.h"
#include <unistd.h> // For sleep function in Linux

#define HEIGHTMAP_SIZE 36

void join_game(ClientSession *session)
{
    Buffer join_packet;
    buffer_init(&join_packet, 256);

    uint8_t packet_id = 0x26; // Packet ID
    int ent_id = 214;         // random number
    uint8_t gamemode = 1;     // creative
    int dim = 0;              // overworld
    long hash = 1234567890;   // random number for hash
    uint8_t max = 5;          // ignored
    char ltype = 0;           // default
    int viewD = 0x2;          // view distance
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
    // int result = load_player_from_file("/server/players.txt", session->username, &session);
    // if (result == 0) {
    //} else {
    //     printf("[ERROR] Failed to load player data.\n");
    // }
    player_info_packet(session, session->allSessions);
    buffer_free(&join_packet);
    player_pos_look(session);
}

// Basic stone platform chunk packet.
void send_stone_platform_chunk(ClientSession *session)
{

    Buffer chunkPacket;
    buffer_init(&chunkPacket, 4096);

    int chunkX = 0;
    int chunkZ = 0;
    bool full_chunk = 1;
    int bit_mask = 1;

    // Packet ID
    write_varInt_buffer(&chunkPacket, 0x22); // VarInt for packet ID 0x22

    // Chunk Coordinates
    buffer_append(&chunkPacket, &chunkX, sizeof(int32_t));
    buffer_append(&chunkPacket, &chunkZ, sizeof(int32_t));

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

    send(session->socket, chunkPacket.data, chunkPacket.size, 0);
}


int load_player_from_file(const char *filename, const char *search_username, ClientSession *session)
{
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        printf("[ERROR] Failed to open file %s.\n", filename);
        return -1;
    }
    printf("USERNAME TO SEARCH: %s", search_username);
    char line[1024];
    while (fgets(line, sizeof(line), file))
    {
        char file_uuid[36], file_username[256], skinUrl[512];
        float x, y, z;

        int scanned = sscanf(line, "%35[^;];%255[^;];%f;%f;%f;%511[^\n]",
                             file_uuid, file_username, &x, &y, &z, skinUrl);

        if (scanned == 6)
        {
            if (strcmp(file_username, search_username) == 0)
            {
                strncpy(session->player.username, file_username, sizeof(session->player.username));
                session->player.username[sizeof(session->player.username) - 1] = '\0';

                strncpy(session->player.uuid, file_uuid, sizeof(session->player.uuid));
                session->player.uuid[sizeof(session->player.uuid) - 1] = '\0';

                session->player.x = x;
                session->player.y = y;
                session->player.z = z;

                strncpy(session->player.skinUrl, skinUrl, sizeof(session->player.skinUrl));
                session->player.skinUrl[sizeof(session->player.skinUrl) - 1] = '\0';

                fclose(file);
                printf("[INFO] Loaded player %s (UUID: %s)\n", session->player.username, session->player.uuid);
                return 0;
            }
        }
        else
        {
            printf("[WARN] Skipping malformed line: %s", line);
        }
    }

    fclose(file);
    printf("[ERROR] Player with username '%s' not found in file.\n", search_username);
    return -1;
}

// This should be a dynamic packet with x, y, z coords
void player_pos_look(ClientSession *session)
{
    Buffer player_poslook_packet;
    buffer_init(&player_poslook_packet, 512);

    uint8_t packet_id = 0x36;
    double x = session->player.x; 
    double y = session->player.y;
    double z = session->player.z;
    float yaw = session->player.yaw;
    float pitch = session->player.pitch;
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

    send_stone_platform_chunk(session);
}

void send_keep_alive(ClientSession *session, struct pollfd *fd)
{
    if (!session || session->socket == -1)
        return; // Invalid socket
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

void handle_play_state(ClientSession *session, int packet_id, uint8_t *packet, int packet_length)
{
    switch (packet_id)
    {
    case 0x05:
    {
        printf("RECEIVED CLIENT SETTINGS\n");
        break;
    }
    case 0x0B:
    {
        printf("RECEIVED CLIENT PLUGIN MESSAGE\n");
        break;
    }
    case 0x00:
    {
        printf("RECEIVED CLIENT TELEPORT CONFIRM\n");
        break;
    }
    case 0x0F:
    {
        printf("KEEP ALIVE RECEIVED\n");
        break;
    }
    case 0x03:
    {
        printf("%s: Sent a message\n", session->username);
        handle_chat_packet(session, packet, packet_length);
        break;
    }
    
    case 0x22:
    {
        printf("BEACON?\n");
        break;
    }
    //Player movement and position packets

    case 0x11: {  //Player position
        
        uint8_t *current_packet = packet; 
        handle_player_position(session,current_packet);
        break;
    }
    case 0x12: {  //Player position and rotation
        
        uint8_t *current_packet = packet; 
        handle_player_position_rotation(session,current_packet);
        break;
    }
    case 0x13: {  //Player Rotation
        uint8_t *current_packet = packet; 
        handle_player_rotation(session,current_packet);
        break;
    }
    case 0x14: {  //Player movement
        uint8_t *current_packet = packet; 
        handle_player_movement(session,current_packet);
        break;
    }
    }
}
void handle_chat_packet(ClientSession *session, uint8_t *packet, int packetLength)
{
    char *chatMessage = extractMessageFromPacket(packet, packetLength);
    if (!chatMessage)
    {
        printf("[ERROR] Failed to extract chat message.\n");
        return;
    }

    char jsonStr[512];
    int bytesWritten = snprintf(jsonStr, sizeof(jsonStr),
                                "{\"translate\":\"chat.type.text\",\"with\":[{\"text\":\"%s\"},{\"text\":\"%s\"}]}", // correct json structure blob
                                session->username, chatMessage);

    if (bytesWritten < 0 || bytesWritten >= sizeof(jsonStr))
    {
        printf("[ERROR] Failed to construct JSON string.\n");
        free(chatMessage);
        return;
    }

    broadcastChatMessage(session, session->allSessions, session->sessionCount, jsonStr);

    free(chatMessage);
}

// loop through all sessions and send the packet
void broadcastChatMessage(ClientSession *sender, ClientSession sessions[], int sessionCount, const char *message)
{
    for (int i = 0; i < sessionCount; i++)
    {
        ClientSession *target = &sessions[i];

        if (target->socket != -1)
        {
            Buffer buffer_chat_packet;
            uint8_t packet_id = 0x0F; // clientbound chat message
            buffer_init(&buffer_chat_packet, 256);

            int messageLength = strlen(message);
            uint8_t type = 0x00;

            buffer_append(&buffer_chat_packet, &packet_id, sizeof(packet_id));
            write_varInt_buffer(&buffer_chat_packet, messageLength);
            buffer_append(&buffer_chat_packet, message, messageLength);
            buffer_append(&buffer_chat_packet, &type, sizeof(type));
            prepend_packet_length(&buffer_chat_packet);

            buffer_to_sendbuffer(target, &buffer_chat_packet);
            sendPacket(target);

            buffer_free(&buffer_chat_packet);
        }
    }
}

char *extractMessageFromPacket(uint8_t *packet, int packetLength)
{
    int offset = 0;

    offset += 1;

    int messageLength = read_varint(packet, &offset);

    if (offset + messageLength > packetLength)
    {
        printf("[ERROR] Message length exceeds packet bounds: %d + %d > %d\n", offset, messageLength, packetLength);
        return NULL;
    }

    char *message = malloc(messageLength + 1);
    if (!message)
        return NULL;

    memcpy(message, packet + offset, messageLength);
    message[messageLength] = '\0';

    return message;
}



void player_info_packet(ClientSession *sourceSession, ClientSession *allSessions)
{
  
    Player *find_player = find_player_in_file(sourceSession->username);
    if (!find_player) {
        printf("[ERROR] Player not found in file for '%s'.\n", sourceSession->player.username);
        return;
    }

    Buffer buffer;
    buffer_init(&buffer, 512);

    uint8_t packet_id = 0x34;
    uint8_t action = 0x00;         // Add Player
    uint8_t num_of_players = 0x01; 
    uint8_t num_of_properties = 0x01;
    uint8_t gamemode = 0x00; // Survival, hardcoded
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
    if (sourceSession->sessionCount <= 0) {
        printf("[WARNING] No sessions to send the packet to.\n");
        buffer_free(&buffer);
        return;
    }

    // Loop through sessions and send the packet
    printf("[DEBUG] Looping through %d sessions.\n", sourceSession->sessionCount);
    for (int i = 0; i < sourceSession->sessionCount; i++) {
        ClientSession *target = &allSessions[i];
        if (!target) {
            printf("[WARNING] target session %d is NULL.\n", i);
            continue;
        }

        if (target->socket == -1) {
            printf("[DEBUG] Skipping session %d (socket -1).\n", i);
            continue;
        }

        printf("[DEBUG] Sending packet to session %d.\n", i);
        buffer_to_sendbuffer(target, &buffer);
        sendPacket(target);
    }

    // Free buffer
    printf("[DEBUG] Freeing buffer.\n");
    buffer_free(&buffer);
    printf("[DEBUG] Exiting player_info_packet.\n");
}

