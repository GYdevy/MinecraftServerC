#include "handshake.h"
#include "packet_utils.h"
#include "socket_utils.h"
#include <stdio.h>
#include <string.h>
#include "login.h"
#include "server.h"

void handshake(ClientSession *session, uint8_t *data, int length)
{
    int offset = 1;
    int protocolVersion = read_varint(data, &offset);
    char serverAddress[256];

    int addressLength = read_varint(data, &offset);
    if (addressLength > 255)
    {
        printf("[ERROR] Server address too long.\n");
        return;
    }
    memcpy(serverAddress, data + offset, addressLength);
    serverAddress[addressLength] = '\0';
    offset += addressLength;

    uint16_t serverPort = (data[offset] << 8) | data[offset + 1];
    offset += 2;

    int nextState = read_varint(data, &offset);

    if (nextState == 1)
    {
        session->state = STATE_STATUS;
    }
    else if (nextState == 2)
    {
        session->state = STATE_LOGIN;
    }
    else
    {
        printf("[ERROR] Invalid next state: %d\n", nextState);
    }
}


void handle_ping_pong(ClientSession *session)
{
    unsigned char ping_packet[12]; // Allocate buffer correctly
    int recv_bytes = recv(session->socket, ping_packet, sizeof(ping_packet), 0);

    if (recv_bytes > 0)
    {
        Buffer buffer_ping_packet;
        buffer_init(&buffer_ping_packet, 11);

        uint8_t length_and_id[] = {0x09, STATUS_CLIENTBOUND_PONG_PACKET};
        buffer_append(&buffer_ping_packet, length_and_id, 2);
        unsigned char ping_content[8];
        memcpy(ping_content, ping_packet + 4, 8);
        buffer_append(&buffer_ping_packet, ping_content, 8);

        send(session->socket, buffer_ping_packet.data, 10, 0);
        //close(session->socket);
    }
}

//maybe todo show player names when hovering over the ping
void build_send_status_response(ClientSession *session)
{
    const char *json_string =
        "{"
        "\"version\":{\"name\":\"1.15.2\",\"protocol\":578},"
        "\"players\":{\"max\":5,\"online\":%d,\"sample\":["
        "{\"name\":\"\",\"id\":\"4566e69f-c907-48ee-8d71-d7ba5aa00d20\"}"
        "]},"
        "\"description\":{\"text\":\"GYdevy's Disaster!\"},"
        "\"enforcesSecureChat\":false"
        "}";

    char json[512];
    int actualCount = sessionCount - 1; // too lazy to start changing stuff this is a workaround
    snprintf(json, sizeof(json), json_string, actualCount);

    Buffer buffer;
    buffer_init(&buffer, 256);

    int json_length = strlen(json);
    // Build the length buffers
    Buffer json_length_buffer, total_length_buffer;
    buffer_init(&json_length_buffer, 5);
    buffer_init(&total_length_buffer, 5);

    write_varInt_buffer(&json_length_buffer, json_length);

    int total_packet_size = 1 + json_length_buffer.size + json_length;
    write_varInt_buffer(&total_length_buffer, total_packet_size);

    buffer_append(&buffer, total_length_buffer.data, total_length_buffer.size);
    buffer_append(&buffer, (uint8_t[]){STATUS_CLIENTBOUND_RESPONSE_PACKET}, 1); // Packet ID for Status Response
    buffer_append(&buffer, json_length_buffer.data, json_length_buffer.size);
    buffer_append(&buffer, json, json_length);
    // Free length buffers
    buffer_free(&json_length_buffer);
    buffer_free(&total_length_buffer);

    buffer_to_sendbuffer(session,&buffer);
    sendPacket(session);
    buffer_free(&buffer);
}
