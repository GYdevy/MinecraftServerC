#include "handshake.h"
#include "packet_utils.h"
#include "socket_utils.h"
#include <stdio.h>
#include "login.h"


//TODO REFACTOR add prints after refactor
void handshake(SOCKET clientSocket) {
    int next_state = handleHandshake(clientSocket);
    Buffer packetBuffer;
    buffer_init(&packetBuffer,1024);
    if (next_state == 1) {
        packetBuffer = build_status_response();

        send(clientSocket, packetBuffer.data, packetBuffer.size, 0);
        buffer_free(&packetBuffer);
        printf("[INFO] Status request detected. Handling ping-pong sequence.\n");
        handle_ping_pong(clientSocket);
        close_ping_pong_connection(clientSocket);
    } else if (next_state == 2) {
        printf("[INFO] Real connection detected. Proceeding to login.\n");
        handle_login(clientSocket);
    } else {
        printf("[ERROR] Unknown handshake type. Closing connection.\n");
        closesocket(clientSocket);
    }
}
int handleHandshake(SOCKET clientSocket) {
    unsigned char packet_length_buffer[2];
    int recv_bytes = recv(clientSocket, packet_length_buffer, 1, 0);

    if (recv_bytes <= 0) {
        printf("[ERROR] Failed to read packet length.\n");
        return -1;
    }

    int packet_length = packet_length_buffer[0];
    if (packet_length > 128) {
        printf("[ERROR] Unexpectedly large packet size: %d\n", packet_length);
        return -1;
    }

    unsigned char handshake_packet[128];
    recv_bytes = recv(clientSocket, handshake_packet, packet_length, 0);

    if (recv_bytes != packet_length) {
        printf("[ERROR] Incomplete handshake packet. Expected %d bytes, got %d\n", packet_length, recv_bytes);
        return -1;
    }

    print_packet(handshake_packet, recv_bytes);

    int next_state = handshake_packet[recv_bytes - 1];
    printf("NEXT STATE: %d\n", next_state);

    return next_state;
}


int parsePacket(SOCKET clientSocket) {
    char buffer[512];
    int bytesReceived = 0;


        unsigned char packetSize;

        bytesReceived = recv(clientSocket, (char *)&packetSize, 1, 0);
        if (bytesReceived <= 0) {
            printf("Error reading packet size or connection closed.\n");
            return -1;
        }
        printf("Packet size: %d\n", packetSize);

        if (packetSize > sizeof(buffer)) {
            printf("Packet size is too large!\n");
            return -1;
        }

        int totalReceived = 0;
        while (totalReceived < packetSize) {
            bytesReceived = recv(clientSocket, buffer + totalReceived, packetSize - totalReceived, 0);
            if (bytesReceived <= 0) {
                printf("Error reading full packet or connection closed.\n");
                return -1;
            }
            printf("[STAGE] Client to server handshake received.\n");
            totalReceived += bytesReceived;
        }


        printf("----------------------------------------------------------------------------------"
               "\nThis should be the client to server packet indicating the ip,port and next state.\n");
        print_packet(buffer,bytesReceived);
        printf("----------------------------------------------------------------------------------\n");
    return 0;
    }
void handle_ping_pong(SOCKET clientSocket) {
    unsigned char ping_packet[12]; // Allocate buffer correctly
    int recv_bytes = recv(clientSocket, ping_packet, sizeof(ping_packet), 0);

    printf("\n----------------------------------------------------------------------------------\n"
           "[STAGE] Ping packet received. Packet id 01 + random long value\n");
    print_packet(ping_packet, recv_bytes);
    printf("[DEBUG] RECV BYTES = %d\n", recv_bytes);
    printf("----------------------------------------------------------------------------------");

    if (recv_bytes > 0) {
        printf("\n----------------------------------------------------------------------------------\n"
               "[STAGE] Handling pong response, packet size + similar contents to ping request.\n");
        Buffer buffer_ping_packet;
        buffer_init(&buffer_ping_packet,11);
        // Construct the pong response

        uint8_t length_and_id[] = {0x09,0x01};
        buffer_append(&buffer_ping_packet,length_and_id,2);
        unsigned char ping_content[8];
        memcpy(ping_content,ping_packet+4,8);
        buffer_append(&buffer_ping_packet,ping_content,8);

        send(clientSocket, buffer_ping_packet.data, 10, 0);
        printf("----------------------------------------------------------------------------------\n");
    }
}
Buffer build_status_response() {
    const char *json_string =
        "{"
        "\"version\":{\"name\":\"1.15.2\",\"protocol\":578},"
        "\"players\":{\"max\":5,\"online\":0,\"sample\":["
        "{\"name\":\"\",\"id\":\"4566e69f-c907-48ee-8d71-d7ba5aa00d20\"}"
        "]},"
        "\"description\":{\"text\":\"A Minecraft Server!\"},"
        "\"favicon\":\"data:image/png;base64,<data>\","
        "\"enforcesSecureChat\":false"
        "}";

    Buffer buffer;
    buffer_init(&buffer, 256);

    int json_length = strlen(json_string);
    Buffer json_length_buffer;
    buffer_init(&json_length_buffer, 5);
    write_varInt_buffer(&json_length_buffer, json_length);

    int total_packet_size = 1 + json_length_buffer.size + json_length;
    Buffer total_length_buffer;
    buffer_init(&total_length_buffer, 5);
    write_varInt_buffer(&total_length_buffer, total_packet_size);

    buffer_append(&buffer, total_length_buffer.data, total_length_buffer.size);
    buffer_append(&buffer, (uint8_t[]){0x00}, 1); // Packet ID
    buffer_append(&buffer, json_length_buffer.data, json_length_buffer.size);
    buffer_append(&buffer, json_string, json_length);

    buffer_free(&json_length_buffer);
    buffer_free(&total_length_buffer);

    return buffer;
}
void close_ping_pong_connection(SOCKET clientSocket) {

    printf("\n----------------------------------------------------------------------------------\n"
        "[STAGE] Starting connection closing after handshake and ping.\n");
    unsigned char buffer[2];
    int recv_bytes = recv(clientSocket, buffer, sizeof(buffer), 0);

    if (recv_bytes == 0) {
        printf("Received FIN-ACK\n");

        shutdown(clientSocket, SD_RECEIVE);
        shutdown(clientSocket, SD_SEND);
        closesocket(clientSocket);
        printf("[STAGE] Connection closed.\n");
        printf("\n----------------------------------------------------------------------------------\n");
    }
}

