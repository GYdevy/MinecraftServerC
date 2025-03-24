#include "login.h"
#include "handshake.h"
#include "packet_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "extract_uuid.h"
#include "play.h"

//TODO refactor whole file
void handle_login(SOCKET clientSocket) {
    int packet_length = get_packetLength(clientSocket);
    unsigned char *packet = receive_login(clientSocket, &packet_length);
    int response_length;
    char *uuid = nullptr;
    char *username = nullptr;
    extract_username_and_uuid(packet, &username, &uuid);
    char *formatted_uuid = get_formatted_uuid(uuid);

    unsigned char *response = create_login_response_data(packet, &response_length);
    if (response) {
        printf("[DEBUG] RESPONSE SIZE: %d bytes\n", response_length);
        send_login_success(clientSocket, formatted_uuid, username);
        join_game(clientSocket);
        send_keepalive(clientSocket);
        free(response);
    }
}

void send_login_success(SOCKET clientSocket, const char *uuid, const char *username) {
    Buffer response;
    buffer_init(&response, 64);

    uint8_t packet_id = 0x02;
    write_varInt_buffer(&response, packet_id);

    size_t uuid_len = strlen(uuid);
    write_varInt_buffer(&response, uuid_len);  // Length of UUID string
    buffer_append(&response, uuid, uuid_len);  // Append UUID

    // Encode Username as a string (max 16 characters)
    int name_len = strlen(username);
    if (name_len > 16) {
        name_len = 16;
    }
    write_varInt_buffer(&response, name_len);  // Length of username string
    buffer_append(&response, username, name_len);  // username string

    prepend_packet_length(&response);

    send(clientSocket, response.data, response.size, 0);

    buffer_free(&response);
}



unsigned char *receive_login(SOCKET clientSocket, int *out_packet_length) {
    printf("out_packet_length: %d\n", *out_packet_length);
    unsigned char *login_packet = (unsigned char *) malloc(*out_packet_length);
    if (!login_packet) {
        perror("malloc failed");
        return NULL;
    }

    int recv_bytes = recv(clientSocket, login_packet, *out_packet_length, 0);
    if (recv_bytes <= 0) {
        free(login_packet);
        return NULL;
    }

    return login_packet;
}


unsigned char *create_login_response_data(unsigned char *packet, int *total_length) {
    unsigned char *username = NULL;
    unsigned char *uuid = NULL;

    int username_length = extract_username_and_uuid(packet, &username, &uuid);
    if (username_length == -1) {
        return NULL;
    }


    unsigned char prefixed_array_length_buf[5];
    int prefixed_array_length = write_varint(0, prefixed_array_length_buf);

    unsigned char buffer[5];
    int payload_length = calculate_payload_length(username_length, prefixed_array_length, buffer);

    unsigned char length_prefix[5];
    int length_prefix_size = write_varint(payload_length, length_prefix);
    *total_length = length_prefix_size + payload_length;

    unsigned char *response = construct_response(length_prefix, length_prefix_size, uuid, username, username_length,
                                                 prefixed_array_length_buf, prefixed_array_length, *total_length);

    free(username);
    free(uuid);

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

//useless shit to refactor
int calculate_payload_length(int username_length, int prefixed_array_length, unsigned char *buffer) {
    int username_length_size = write_varint(username_length, buffer);

    // Compute payload length (including UUID, username length, username, and prefixed array)
    return 16 + username_length_size + username_length + prefixed_array_length;
}
//TODO REFACTOR
unsigned char *construct_response(unsigned char *length_prefix, int length_prefix_size, unsigned char *uuid,
                                  unsigned char *username, int username_length,
                                  unsigned char *prefixed_array_length_buf, int prefixed_array_length,
                                  int total_length) {
    unsigned char *response = (unsigned char *) malloc(total_length);
    if (!response) {
        perror("malloc failed");
        return NULL;
    }

    memcpy(response, length_prefix, length_prefix_size);
    response[length_prefix_size] = 0x02; // Packet ID

    memcpy(&response[length_prefix_size + 1], uuid, 16);

    int username_length_size = write_varint(username_length, &response[length_prefix_size + 17]);

    memcpy(&response[length_prefix_size + 17 + username_length_size], username, username_length);

    memcpy(&response[length_prefix_size + 17 + username_length_size + username_length], prefixed_array_length_buf,
           prefixed_array_length);

    return response;
}




void prepend_packet_length(Buffer *buf) {
    uint8_t varint[5];  // Max VarInt size
    // We prepend the length of the entire packet, which is the buffer size + the length of the varint.
    size_t varint_size = write_varint_t(varint, (uint32_t)buf->size);

    size_t total_size = buf->size + varint_size;
    uint8_t *new_data = malloc(total_size);
    if (!new_data) {
        perror("Failed to allocate memory for prepending length");
        exit(1);
    }

    // Copy VarInt length prefix and packet data
    memcpy(new_data, varint, varint_size);
    memcpy(new_data + varint_size, buf->data, buf->size);

    free(buf->data);
    buf->data = new_data;
    buf->size = total_size;
}