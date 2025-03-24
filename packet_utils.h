
#include <stdint.h>
#include <winsock2.h>

#ifndef PACKET_UTILS_H
#define PACKET_UTILS_H

typedef struct {
    uint8_t *data;
    size_t size;
    size_t capacity;
} Buffer;

int write_varint(int value, unsigned char *buffer);
int parseVarInt(const unsigned char *buffer, int *bytesRead);
void print_packet(const unsigned char *packet, int packet_size);
int get_packetLength(SOCKET clientSocket);
int write_string(unsigned char *buffer, const char *string);
int write_varint_t(uint8_t *buffer, int value);
void buffer_append(Buffer *buf, const void *data, size_t data_size);
void buffer_init(Buffer *buf, size_t initial_capacity);
void buffer_free(Buffer *buf);
void write_varInt_buffer(Buffer *buffer, int value);
#endif
