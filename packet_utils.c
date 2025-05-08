#include "packet_utils.h"

#include <time.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "server.h"
#include <sys/time.h>


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
        uint8_t byte = value & 0x7F; // Extract lowest 7 bits
        value >>= 7;
        if (value) {
            byte |= 0x80; // Set continuation bit
        }
        buffer[len++] = byte; // Store the byte
        if (!value) break;
    }
    return len;
}

int read_varint(uint8_t *buffer, int *offset) {
    int value = 0;
    int position = 0;
    uint8_t byte;

    do {
        byte = buffer[*offset];
        value |= (byte & 0x7F) << (position * 7);
        (*offset)++;
        position++;

        if (position > 5) {
            // A VarInt must not be more than 5 bytes
            return -1;
        }
    } while (byte & 0x80);

    return value;
}


void buffer_init(Buffer *buffer, size_t initial_capacity) {
    buffer->data = (uint8_t *) malloc(initial_capacity);
    if (buffer->data == NULL) {
        printf("[ERROR] Memory allocation failed for buffer data\n");
        return;
    }
    buffer->size = 0;
    buffer->capacity = initial_capacity;
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

void buffer_to_sendbuffer(ClientSession *session, Buffer *buffer) {
    if (buffer->size > sizeof(session->sendBuffer)) {
        printf("[ERROR] Buffer size exceeds sendBuffer capacity\n");
        return;
    }

    // Copy the data from the Buffer to the sendBuffer
    memcpy(session->sendBuffer, buffer->data, buffer->size);

    // Update session's sendLength and reset sendOffset
    session->sendLength = buffer->size;
    session->sendOffset = 0;
}

// Append a big-endian value to a buffer
void buffer_append_be(Buffer *buffer, const void *value, size_t size) {
    uint8_t temp[8]; // Maximum size needed for double (8 bytes)
    memcpy(temp, value, size); // Copy raw bytes

    for (size_t i = 0; i < size / 2; i++) {
        uint8_t swap = temp[i];
        temp[i] = temp[size - 1 - i];
        temp[size - 1 - i] = swap;
    }

    buffer_append(buffer, temp, size);
}

void write_varInt_buffer(Buffer *buffer, int value) {
    printf("[DEBUG] Writing VarInt: %d\n", value);
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

// Align the buffer size to the given alignment
void buffer_align(Buffer *buffer, size_t alignment) {
    if (alignment == 0 || (alignment & (alignment - 1)) != 0) {
        // Make sure alignment is a power of 2
        printf("Error: Alignment must be a power of 2.\n");
        return;
    }

    // Calculate the next aligned address
    size_t alignment_offset = (alignment - (buffer->size % alignment)) % alignment;
    if (alignment_offset > 0) {
        // If alignment offset is non-zero, we need to pad the buffer
        size_t new_capacity = buffer->capacity + alignment_offset;
        buffer->data = realloc(buffer->data, new_capacity); // Resize buffer if needed
        buffer->capacity = new_capacity;

        // Move the current data to the new aligned position
        memmove(buffer->data + alignment_offset, buffer->data, buffer->size);
        buffer->size += alignment_offset; // Update the size after padding
    }
}

uint64_t getCurrentTimeMillis() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (uint64_t) (ts.tv_sec) * 1000 + (ts.tv_nsec / 1000000);
}
void swap_bytes(void *data, size_t size) {
    uint8_t *bytes = (uint8_t *)data;
    for (size_t i = 0; i < size / 2; ++i) {
        uint8_t temp = bytes[i];
        bytes[i] = bytes[size - i - 1];
        bytes[size - i - 1] = temp;
    }
}

// Convert from little-endian to big-endian for doubles
double convert_double_little_endian_to_big_endian(uint8_t *buf) {

    swap_bytes(buf, sizeof(double));
    
    double value;
    memcpy(&value, buf, sizeof(double));
    
    return value;
}
float convert_float_little_endian_to_big_endian(uint8_t *buf) {
    swap_bytes(buf, sizeof(float));

    float value;
    memcpy(&value, buf, sizeof(float));

    return value;
}