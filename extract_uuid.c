#include "extract_uuid.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "packet_utils.h"
#include <stdint.h>
#include "curl/curl.h"
#define SERVER "https://api.mojang.com"
#define PORT 443  //HTTPS PORT

size_t write_callback(void *ptr, size_t size, size_t nmemb, char *data) {
    size_t total_size = size * nmemb;
    strncat(data, ptr, total_size); // Append response to data buffer
    return total_size;
}

//honestly useless for the project but seeing a real skin is cooler
char *get_uuid(const char *username) {
    char url[512];
    char response[512] = "";

    // Build the full URL for the GET request
    snprintf(url, sizeof(url), "%s/users/profiles/minecraft/%s", SERVER, username);

    // Initialize libcurl
    curl_global_init(CURL_GLOBAL_DEFAULT); // Initialize global curl environment
    CURL *curl = curl_easy_init(); // Initialize a curl session

    if (curl) {
        // Set the URL
        curl_easy_setopt(curl, CURLOPT_URL, url);

        // Set the write callback function to handle the response
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, response); // Pass the response buffer

        // Perform the request
        CURLcode res = curl_easy_perform(curl);

        // Check for errors
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        } 

        curl_easy_cleanup(curl);
    }


    char *uuid = NULL;

    if (strlen(response) > 12) {
        uuid = malloc(37);

        if (uuid) {
            strncpy(uuid, &response[12], 36);
            uuid[36] = '\0'; // Null terminate
        }
    }


    curl_global_cleanup();

    return uuid;
}
char *get_skin_base64(const char *uuid){

    char url[512];
    char response[32767] = "";
    snprintf(url, sizeof(url), "https://sessionserver.mojang.com/session/minecraft/profile/%s", uuid);
    curl_global_init(CURL_GLOBAL_DEFAULT); // Initialize global curl environment
    CURL *curl = curl_easy_init(); // Initialize a curl session

    if (curl) {
        // Set the URL
        curl_easy_setopt(curl, CURLOPT_URL, url);

        // Set the write callback function to handle the response
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, response); // Pass the response buffer

        // Perform the request
        CURLcode res = curl_easy_perform(curl);
        
        // Check for errors
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        } 

        curl_easy_cleanup(curl);
    }
    char *base64_skin = NULL;

    const char *key = "\"value\"";
    char *pos = strstr(response, key);
    if (pos) {
        // Move pointer to after "value":
        pos += strlen(key);
        
        // Skip whitespace and colon
        while (*pos && (*pos == ' ' || *pos == '\t' || *pos == '\n' || *pos == ':')) pos++;

        // Skip opening quote
        if (*pos == '"') pos++;
        else {
            fprintf(stderr, "No opening quote after value key\n");
            return NULL;
        }

        // Find closing quote
        char *end = pos;
        while (*end && *end != '"') end++;

        if (*end != '"') {
            fprintf(stderr, "No closing quote for value string\n");
            return NULL;
        }

        size_t length = end - pos;
        base64_skin = malloc(length + 1);
        if (base64_skin) {
            strncpy(base64_skin, pos, length);
            base64_skin[length] = '\0';  // Null-terminate
        }
    } else {
        fprintf(stderr, "\"value\" key not found in response\n");
    }

    return base64_skin;


}

//format uuid form xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx to have the hyphens. this mc version wants it
char *get_formatted_uuid(const char *raw_uuid) {
    Buffer buffer;
    buffer_init(&buffer, 64); // Start with a reasonable capacity

    buffer_append(&buffer, raw_uuid, 8);
    buffer_append(&buffer, "-", 1); // Add the hyphen

    buffer_append(&buffer, &raw_uuid[8], 4);
    buffer_append(&buffer, "-", 1); // Add the hyphen

    buffer_append(&buffer, &raw_uuid[12], 4);
    buffer_append(&buffer, "-", 1); // Add the hyphen

    buffer_append(&buffer, &raw_uuid[16], 4);
    buffer_append(&buffer, "-", 1); // Add the hyphen

    buffer_append(&buffer, &raw_uuid[20], 12);

    buffer_append(&buffer, "\0", 1);

    // Convert to string
    char *formatted_uuid = malloc(buffer.size + 1);
    memcpy(formatted_uuid, buffer.data, buffer.size);
    formatted_uuid[buffer.size] = '\0'; // Null-terminate the string

    buffer_free(&buffer);

    return formatted_uuid;
}
void parse_uuid_bytes(const char *uuid_str, uint8_t uuid_bytes[16]) {
    for (int i = 0; i < 16; i++) {
        char byte_str[3] = { uuid_str[i * 2], uuid_str[i * 2 + 1], 0 }; // grab 2 hex chars
        uuid_bytes[i] = (uint8_t) strtol(byte_str, NULL, 16);
    }
}