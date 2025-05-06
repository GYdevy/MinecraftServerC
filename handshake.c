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

int parsePacket(int clientSocket)
{
    char buffer[512];
    int bytesReceived = 0;

    unsigned char packetSize;

    bytesReceived = recv(clientSocket, (char *)&packetSize, 1, 0);
    if (bytesReceived <= 0)
    {
        printf("Error reading packet size or connection closed.\n");
        return -1;
    }
    printf("Packet size: %d\n", packetSize);

    if (packetSize > sizeof(buffer))
    {
        printf("Packet size is too large!\n");
        return -1;
    }

    int totalReceived = 0;
    while (totalReceived < packetSize)
    {
        bytesReceived = recv(clientSocket, buffer + totalReceived, packetSize - totalReceived, 0);
        if (bytesReceived <= 0)
        {
            printf("Error reading full packet or connection closed.\n");
            return -1;
        }
        totalReceived += bytesReceived;
    }

    return 0;
}

void handle_ping_pong(int clientSocket)
{
    unsigned char ping_packet[12]; // Allocate buffer correctly
    int recv_bytes = recv(clientSocket, ping_packet, sizeof(ping_packet), 0);

    if (recv_bytes > 0)
    {
        Buffer buffer_ping_packet;
        buffer_init(&buffer_ping_packet, 11);

        uint8_t length_and_id[] = {0x09, 0x01};
        buffer_append(&buffer_ping_packet, length_and_id, 2);
        unsigned char ping_content[8];
        memcpy(ping_content, ping_packet + 4, 8);
        buffer_append(&buffer_ping_packet, ping_content, 8);

        send(clientSocket, buffer_ping_packet.data, 10, 0);
        close(clientSocket);
    }
}

// status response data when client refreshes server list.
// written stupidly, should be made to extract current state and send to client.
void build_send_status_response(int clientSocket)
{
    const char *json_string =
        "{"
        "\"version\":{\"name\":\"1.15.2\",\"protocol\":578},"
        "\"players\":{\"max\":5,\"online\":0,\"sample\":["
        "{\"name\":\"\",\"id\":\"4566e69f-c907-48ee-8d71-d7ba5aa00d20\"}"
        "]},"
        "\"description\":{\"text\":\"A Minecraft Server!\"},"
        "\"enforcesSecureChat\":false"
        "}";

    Buffer buffer;
    buffer_init(&buffer, 256);

    int json_length = strlen(json_string);

    // Create buffers for lengths
    Buffer json_length_buffer, total_length_buffer;
    buffer_init(&json_length_buffer, 5);
    buffer_init(&total_length_buffer, 5);

    write_varInt_buffer(&json_length_buffer, json_length);

    int total_packet_size = 1 + json_length_buffer.size + json_length;
    write_varInt_buffer(&total_length_buffer, total_packet_size);

    // Build the packet
    buffer_append(&buffer, total_length_buffer.data, total_length_buffer.size);
    buffer_append(&buffer, (uint8_t[]){0x00}, 1); // Packet ID
    buffer_append(&buffer, json_length_buffer.data, json_length_buffer.size);
    buffer_append(&buffer, json_string, json_length);

    // Free temp buffers
    buffer_free(&json_length_buffer);
    buffer_free(&total_length_buffer);

    struct pollfd pfd = {.fd = clientSocket, .events = POLLOUT};
    int poll_result = poll(&pfd, 1, 1000); // Timeout = 1000ms

    if (poll_result > 0 && (pfd.revents & POLLOUT))
    {
        send(clientSocket, (char *)buffer.data, buffer.size, 0);
        printf("[INFO] Sent status response (%d bytes)\n", buffer.size);
    }
    else
    {
        printf("[ERROR] Failed to send status response\n");
    }

    buffer_free(&buffer);
}
