#include "login.h"
#include "handshake.h"
#include "packet_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "extract_uuid.h"
#include "play.h"
#include <sys/stat.h>
#define PLAYER_DIR "/server/players"

//TODO refactor whole file
void handle_login(ClientSession *session, uint8_t *packet) {
    int response_length;
    char *uuid = NULL; 
    char *username = NULL; 
    extract_username_and_uuid(packet, &username, &uuid);
    strncpy(session->username, username, sizeof(session->username) - 1);
    session->username[sizeof(session->username) - 1] = '\0';

    char *formatted_uuid = get_formatted_uuid(uuid);
    uuid[32] = '\0';
    unsigned char *response = create_login_response_data(username, formatted_uuid, &response_length);
    if (response) {
        send_login_success(session, uuid,formatted_uuid, username);
        join_game(session);
        free(response);
    }
}

void send_login_success(ClientSession *session, const char *uuid ,const char *formatted_uuid, const char *username) {
    Buffer response;
    buffer_init(&response, 64);

    uint8_t packet_id = LOGIN_CLIENTBOUND_LOGIN_SUCCESS_PACKET;
    write_varInt_buffer(&response, packet_id);

    size_t uuid_len = strlen(formatted_uuid);
    write_varInt_buffer(&response, uuid_len); // Length of UUID string
    buffer_append(&response, formatted_uuid, uuid_len); 

    // Encode Username as a string 
    int name_len = strlen(username);
    if (name_len > 16) {
        name_len = 16;
    }
    write_varInt_buffer(&response, name_len); 
    buffer_append(&response, username, name_len); 
    prepend_packet_length(&response);
    char *base_64_skin = get_skin_base64(uuid);
    add_player_to_file(session,uuid,username,base_64_skin);
    send(session->socket, response.data, response.size, 0);
    buffer_free(&response);
}




int create_players_directory() {
    struct stat st = {0};
    if (stat(PLAYER_DIR, &st) == -1) {
        if (mkdir(PLAYER_DIR, 0755) == -1) {
            printf("[ERROR] Could not create 'players' directory.\n");
            return -1;
        }
        printf("[INFO] Created 'players' directory.\n");
    }

    return 0;
}

int save_player_to_file(ClientSession *session) {
    char player_file_path[512];
    snprintf(player_file_path, sizeof(player_file_path), "%s/%s.txt", PLAYER_DIR, session->player.username);

    FILE *file = fopen(player_file_path, "w");
    if (!file) {
        perror("[ERROR] Could not open player file for writing");
        return -1;
    }

    fprintf(file, "UUID: %s\n", session->player.uuid);
    fprintf(file, "Username: %s\n", session->player.username);
    fprintf(file, "Skin URL: %s\n", session->player.skinUrl);
    fprintf(file, "EID: %d\n", session->player.eid);
    fprintf(file, "Position: %.3f, %.3f, %.3f\n", session->player.x, session->player.y, session->player.z);
    fprintf(file, "Yaw: %.3f\n", session->player.yaw);
    fprintf(file, "Pitch: %.3f\n", session->player.pitch);
    fprintf(file, "Flags: 0x%02X\n", session->player.flags);
    fprintf(file, "OnGround: %d\n", session->player.onGround);

    fclose(file);
    return 0;
}


int add_player_to_file(ClientSession *session, const char *uuid, const char *username, const char *skinUrl) {
    if (create_players_directory() != 0) {
        printf("[ERROR] Failed to create player directory.\n");
        return -1;
    }

    char player_file_path[512];
    snprintf(player_file_path, sizeof(player_file_path), "%s/%s.txt", PLAYER_DIR, username);

    FILE *player_file = fopen(player_file_path, "r");
    if (player_file) {
        Player existing_player = {0};
        char line[1024];
        int fields = 0;

        while (fgets(line, sizeof(line), player_file)) {
            line[strcspn(line, "\n")] = 0;

            if (sscanf(line, "UUID: %36s", existing_player.uuid) == 1) fields++;
            else if (sscanf(line, "Username: %31s", existing_player.username) == 1) fields++;
            else if (strncmp(line, "Skin URL: ", 10) == 0) {
                strncpy(existing_player.skinUrl, line + 10, sizeof(existing_player.skinUrl) - 1);
                existing_player.skinUrl[sizeof(existing_player.skinUrl) - 1] = '\0';
                fields++;
            }
            else if (sscanf(line, "EID: %d", &existing_player.eid) == 1) fields++;
            else if (sscanf(line, "Position: %lf,%lf,%lf", &existing_player.x, &existing_player.y, &existing_player.z) == 3) fields++;
            else if (sscanf(line, "Yaw: %f", &existing_player.yaw) == 1) fields++;
            else if (sscanf(line, "Pitch: %f", &existing_player.pitch) == 1) fields++;
            else if (sscanf(line, "Flags: 0x%02X", &existing_player.flags) == 1) fields++;
            else if (sscanf(line, "OnGround: %d", &existing_player.onGround) == 1) fields++;
        }
        fclose(player_file);

        if (fields == 9) {  // Updated for 9 fields now
            fetch_player_into_session(session, &existing_player);
            printf("[INFO] Loaded existing player '%s' into session.\n", existing_player.username);
            return 0;
        } else {
            printf("[ERROR] Corrupted player file '%s'. Only %d/9 fields found.\n", username, fields);
            return -1;
        }
    }

    // New player creation
    printf("[INFO] Player file does not exist, creating new player '%s'.\n", username);
    printf("[INFO] GENERATING EID\n");

    int eid = generate_eid();
    printf("[INFO] GENERATED EID: %d\n", eid);

    player_file = fopen(player_file_path, "w");
    if (!player_file) {
        perror("[ERROR] Could not create player file for writing");
        return -1;
    }

    fprintf(player_file, "UUID: %s\n", uuid);
    fprintf(player_file, "Username: %s\n", username);
    fprintf(player_file, "Skin URL: %s\n", skinUrl);
    fprintf(player_file, "EID: %d\n", eid);
    fprintf(player_file, "Position: %.3f, %.3f, %.3f\n", 5.0, 17.0, 5.0);
    fprintf(player_file, "Yaw: 0.0\n");
    fprintf(player_file, "Pitch: 0.0\n");
    fprintf(player_file, "Flags: 0x00\n");
    fprintf(player_file, "OnGround: 1\n");  // Default to onGround = 1
    fclose(player_file);

    strncpy(session->player.username, username, sizeof(session->player.username));
    session->player.username[sizeof(session->player.username) - 1] = '\0';

    strncpy(session->player.uuid, uuid, sizeof(session->player.uuid));
    session->player.uuid[sizeof(session->player.uuid) - 1] = '\0';

    strncpy(session->player.skinUrl, skinUrl, sizeof(session->player.skinUrl));
    session->player.skinUrl[sizeof(session->player.skinUrl) - 1] = '\0';

    session->player.eid = eid;
    session->player.x = 5;
    session->player.y = 17;
    session->player.z = 5;
    session->player.yaw = 0.0f;
    session->player.pitch = 0.0f;
    session->player.flags = 0x00;
    session->player.onGround = 1;

    printf("[INFO] Added player '%s' to file.\n", username);
    return 1;  // New player created
}



Player* find_player_in_file(const char* username) {
    static Player foundPlayer = {0};

    if (create_players_directory() != 0) {
        printf("[ERROR] Could not create players directory.\n");
        return NULL;
    }
    char player_file_path[512];
    snprintf(player_file_path, sizeof(player_file_path), "%s/%s.txt", PLAYER_DIR, username);  //server/players/username.txt
    printf("PATH TO PLAYER FILE: %s\n", player_file_path);
    
    FILE* file = fopen(player_file_path, "r");
    if (!file) {
        printf("[ERROR] Could not open player file '%s' for reading.\n", player_file_path);
        return NULL;
    }

    char line[512];
    char uuid[37], file_username[32], skinUrl[256];
    double x, y, z;
    int eid;
    float yaw, pitch;
    uint8_t flags;
    int onGround;

    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = 0;

        if (strncmp(line, "UUID: ", 6) == 0) {
            sscanf(line + 6, "%36s", uuid);  
        } else if (strncmp(line, "Username: ", 10) == 0) {
            sscanf(line + 10, "%31s", file_username);  
        } else if (strncmp(line, "Skin URL: ", 10) == 0) {
            strncpy(skinUrl, line + 10, sizeof(skinUrl) - 1);  
            skinUrl[sizeof(skinUrl) - 1] = '\0';
        } else if (strncmp(line, "EID: ", 5) == 0) {
            sscanf(line + 5, "%d", &eid);  // Extract EID
        } else if (strncmp(line, "Position: ", 10) == 0) {
            sscanf(line + 10, "%lf,%lf,%lf", &x, &y, &z);  
        } else if (strncmp(line, "Yaw: ", 5) == 0) {
            sscanf(line + 5, "%f", &yaw);
        } else if (strncmp(line, "Pitch: ", 7) == 0) {
            sscanf(line + 7, "%f", &pitch);
        } else if (strncmp(line, "Flags: ", 7) == 0) {
            sscanf(line + 7, "%hhX", &flags);
        } else if (strncmp(line, "OnGround: ", 10) == 0) {
            sscanf(line + 10, "%d", &onGround);
        }
    }

    if (strlen(uuid) > 0 && strcmp(file_username, username) == 0) {
        strncpy(foundPlayer.uuid, uuid, sizeof(foundPlayer.uuid));
        strncpy(foundPlayer.username, file_username, sizeof(foundPlayer.username));
        strncpy(foundPlayer.skinUrl, skinUrl, sizeof(foundPlayer.skinUrl));
        foundPlayer.x = x;
        foundPlayer.y = y;
        foundPlayer.z = z;
        foundPlayer.eid = eid;
        foundPlayer.yaw = yaw;
        foundPlayer.pitch = pitch;
        foundPlayer.flags = flags;
        foundPlayer.onGround = onGround;

        fclose(file);
        return &foundPlayer;  
    }

    fclose(file);
    return NULL;  
}

void fetch_player_into_session(ClientSession *session, Player *player) {
    strncpy(session->player.username, player->username, sizeof(session->player.username));
    session->player.username[sizeof(session->player.username) - 1] = '\0';

    strncpy(session->player.uuid, player->uuid, sizeof(session->player.uuid));
    session->player.uuid[sizeof(session->player.uuid) - 1] = '\0';

    session->player.x = player->x;
    session->player.y = player->y;
    session->player.z = player->z;
    session->player.eid = player->eid;
    session->player.yaw = player->yaw;
    session->player.pitch = player->pitch;
    session->player.flags = player->flags;
    session->player.onGround = player->onGround;
}



unsigned char *create_login_response_data(const char *username, const char *uuid, int *total_length) {
    if (!username || !uuid || !total_length) return NULL;

    int username_length = strlen(username);
    int uuid_length = strlen(uuid); 

    unsigned char prefixed_array_length_buf[5];
    int prefixed_array_length = write_varint(0, prefixed_array_length_buf);

    // Calculate payload length
    unsigned char buffer[5];
    int payload_length = calculate_payload_length(username_length, prefixed_array_length, buffer);

    // Prefix for total packet length
    unsigned char length_prefix[5];
    int length_prefix_size = write_varint(payload_length, length_prefix);
    *total_length = length_prefix_size + payload_length;

    // Allocate memory for the response
    unsigned char *response = (unsigned char *) malloc(*total_length);
    if (!response) {
        return NULL; // Memory allocation failed
    }

    return response;
}

int extract_username_and_uuid(unsigned char *packet, unsigned char **username, unsigned char **uuid) {
    int username_length = packet[1];
    *username = (unsigned char *) malloc(username_length + 1);
    if (!*username) {
        perror("malloc failed");
        return -1;
    }

    // Extract username from the packet
    memcpy(*username, &packet[2], username_length);
    (*username)[username_length] = '\0';

    // Get the UUID by passing the username (as a string)
    *uuid = (unsigned char *) get_uuid((char *) *username);

    if (!*uuid) {
        perror("UUID extraction failed");
        free(*username);
        return -1;
    }

    return username_length; // Return the username length
}

// Calculating the payload length (could be a helper for constructing responses)
int calculate_payload_length(int username_length, int prefixed_array_length, unsigned char *buffer) {
    int username_length_size = write_varint(username_length, buffer);

    // Compute payload length (including UUID, username length, username, and prefixed array)
    return 16 + username_length_size + username_length + prefixed_array_length;
}

void prepend_packet_length(Buffer *buf) {
    uint8_t varint[5];
    size_t varint_size = write_varint_t(varint, (uint32_t) buf->size);

    size_t total_size = buf->size + varint_size;
    uint8_t *new_data = malloc(total_size);
    if (!new_data) {
        perror("Failed to allocate memory for prepending length");
        exit(1);
    }

    memcpy(new_data, varint, varint_size);
    memcpy(new_data + varint_size, buf->data, buf->size);

    free(buf->data);
    buf->data = new_data;
    buf->size = total_size;
}


int generate_eid() {
    return ((int)((uint32_t)rand() << 16 | rand())) & 0x7FFFFFFF;
}
