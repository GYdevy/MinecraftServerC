
#ifndef EXTRACT_UUID_H
#define EXTRACT_UUID_H
#include <stdint.h>
char *get_uuid(const char *username);
char *get_formatted_uuid(const char *raw_uuid);
char *get_skin_base64(const char *uuid);
void parse_uuid_bytes(const char *uuid_str, uint8_t uuid_bytes[16]);
#endif //EXTRACT_UUID_H
