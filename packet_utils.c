#include "packet_utils.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



//TODO REFACTOR THIS SHIT
int write_varint(int value, unsigned char *buffer) {
    int bytes_written = 0;
    do {
        unsigned char temp = value & 0x7F;
        value >>= 7;
        if (value) temp |= 0x80;
        buffer[bytes_written++] = temp;
    } while (value);
    return bytes_written;
}
// Function to encode a VarInt
int write_varint_t(uint8_t *buffer, int value) {
    int len = 0;
    while (1) {
        uint8_t byte = value & 0x7F;  // Extract lowest 7 bits
        value >>= 7;
        if (value) {
            byte |= 0x80;  // Set continuation bit
        }
        buffer[len++] = byte;  // Store the byte
        if (!value) break;
    }
    return len;
}



int parseVarInt(const unsigned char *buffer, int *bytesRead) {
    int value = 0;
    int position = 0;
    unsigned char currentByte;

    do {
        currentByte = buffer[position];
        value |= (currentByte & 0x7F) << (position * 7);
        position++;
        if (position > 5) {
            printf("VarInt is too long!\n");
            return -1;
        }
    } while ((currentByte & 0x80) != 0);

    *bytesRead = position;
    return value;
}

void print_packet(const unsigned char *packet, int packet_size) {
    printf("Packet Data (%d bytes): ", packet_size);
    for (int i = 0; i < packet_size; i++) {
        printf("%02X ", packet[i]);
    }
    printf("\n");
}

int get_packetLength(SOCKET clientSocket) {
    unsigned char packet_length_buffer[2]; // Buffer to hold packet length
    int recv_bytes = recv(clientSocket, packet_length_buffer, 1, 0); // Read 1st byte (length)
    int packet_length = packet_length_buffer[0];
    return packet_length;
}


void buffer_init(Buffer *buf, size_t initial_capacity) {
    buf->data = malloc(initial_capacity);
    buf->size = 0;
    buf->capacity = initial_capacity;
}

void buffer_append(Buffer *buf, const void *data, size_t data_size) {
    if (buf->size + data_size > buf->capacity) {
        size_t new_capacity = buf->capacity * 2 + data_size;
        uint8_t *new_data = realloc(buf->data, new_capacity);
        if (!new_data) {
            perror("Failed to allocate memory");
            exit(1);
        }
        buf->data = new_data;
        buf->capacity = new_capacity;
    }
    memcpy(buf->data + buf->size, data, data_size);
    buf->size += data_size;
}

void buffer_free(Buffer *buf) {
    free(buf->data);
    buf->data = NULL;
    buf->size = buf->capacity = 0;
}

void write_varInt_buffer(Buffer *buffer, int value) {
    uint8_t byte;
    while (1) {
        byte = value & 0x7F;
        value >>= 7;
        if (value) {
            byte |= 0x80;
        }
        buffer_append(buffer, &byte, 1);
        if (!value) {
            break;
        }
    }
}