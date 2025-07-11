#pragma once
#include <stdint.h>
#include <stddef.h>  // <-- Add this line to include size_t

#include "server.h"

#ifndef PACKET_UTILS_H
#define PACKET_UTILS_H

typedef struct {
    uint8_t *data;
    size_t size;
    size_t capacity;
} Buffer;

int write_varint(int value, unsigned char *buffer);

int read_varint(uint8_t *buffer, int *offset);

int write_varint_t(uint8_t *buffer, int value);

void buffer_append(Buffer *buf, const void *data, size_t data_size);

void buffer_init(Buffer *buf, size_t initial_capacity);

void buffer_free(Buffer *buf);

void write_varInt_buffer(Buffer *buffer, int value);

uint64_t getCurrentTimeMillis();

void buffer_to_sendbuffer(ClientSession *session, Buffer *buffer);

void buffer_append_be(Buffer *buffer, const void *value, size_t size);

void buffer_align(Buffer *buffer, size_t alignment);

void swap_bytes(void *data, size_t size);

double convert_double_little_endian_to_big_endian(uint8_t *buf);
float convert_float_little_endian_to_big_endian(uint8_t *buf);
void write_double_be(Buffer *buffer, double value);
u_int16_t convert_short_little_endian_to_big_endian(uint16_t value);
#endif
