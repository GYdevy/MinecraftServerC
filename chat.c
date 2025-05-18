#include "server.h"
#include "packet_utils.h"
#include "chat.h"


void send_player_disconnected(ClientSession *session, ClientSession sessions[])
{
    if (session->state != STATE_PLAY)
        return;

    char jsonStr[512];
    snprintf(jsonStr, sizeof(jsonStr),
             "{\"text\":\"%s left the game.\",\"color\":\"yellow\"}",
             session->player.username);

    for (int i = 0; i < sessionCount; i++)
    {
        if (&sessions[i] == session || sessions[i].state != STATE_PLAY || sessions[i].socket == -1)
            continue;

        Buffer buffer_chat_packet;
        buffer_init(&buffer_chat_packet, 256);

        uint8_t packet_id = 0x0F; // chat message
        buffer_append(&buffer_chat_packet, &packet_id, sizeof(packet_id));

        write_varInt_buffer(&buffer_chat_packet, strlen(jsonStr));
        buffer_append(&buffer_chat_packet, jsonStr, strlen(jsonStr));

        uint8_t type = 0x01; // system message
        buffer_append(&buffer_chat_packet, &type, sizeof(type));

        prepend_packet_length(&buffer_chat_packet);
        buffer_to_sendbuffer(&sessions[i], &buffer_chat_packet);
        sendPacket(&sessions[i]);
        
        buffer_free(&buffer_chat_packet);
    }
}
void handle_chat_packet(ClientSession *session, uint8_t *packet, int packetLength)
{
    char *chatMessage = extractMessageFromPacket(packet, packetLength);
    if (!chatMessage)
    {
        printf("[ERROR] Failed to extract chat message.\n");
        return;
    }

    char jsonStr[512];
    int bytesWritten = snprintf(jsonStr, sizeof(jsonStr),
                                "{\"translate\":\"chat.type.text\",\"with\":[{\"text\":\"%s\"},{\"text\":\"%s\"}]}", // correct json structure blob
                                session->username, chatMessage);

    if (bytesWritten < 0 || bytesWritten >= sizeof(jsonStr))
    {
        printf("[ERROR] Failed to construct JSON string.\n");
        free(chatMessage);
        return;
    }

    broadcastChatMessage(session, session->allSessions, session->sessionCount, jsonStr);
    printf("%s: %s\n", session->username,chatMessage);
    free(chatMessage);
}

// loop through all sessions and send the packet
void broadcastChatMessage(ClientSession *sender, ClientSession sessions[], int sessionCount, const char *message)
{
    for (int i = 0; i < sessionCount; i++)
    {
        ClientSession *target = &sessions[i];

        if (target->socket != -1)
        {
            Buffer buffer_chat_packet;
            uint8_t packet_id = 0x0F; // clientbound chat message
            buffer_init(&buffer_chat_packet, 256);

            int messageLength = strlen(message);
            uint8_t type = 0x00;

            buffer_append(&buffer_chat_packet, &packet_id, sizeof(packet_id));
            write_varInt_buffer(&buffer_chat_packet, messageLength);
            buffer_append(&buffer_chat_packet, message, messageLength);
            buffer_append(&buffer_chat_packet, &type, sizeof(type));
            prepend_packet_length(&buffer_chat_packet);

            buffer_to_sendbuffer(target, &buffer_chat_packet);
            sendPacket(target);

            buffer_free(&buffer_chat_packet);
        }
    }
}

char *extractMessageFromPacket(uint8_t *packet, int packetLength)
{
    int offset = 0;

    offset += 1;

    int messageLength = read_varint(packet, &offset);

    if (offset + messageLength > packetLength)
    {
        printf("[ERROR] Message length exceeds packet bounds: %d + %d > %d\n", offset, messageLength, packetLength);
        return NULL;
    }

    char *message = malloc(messageLength + 1);
    if (!message)
        return NULL;

    memcpy(message, packet + offset, messageLength);
    message[messageLength] = '\0';

    return message;
}
void send_player_joined_message(ClientSession *newSession, ClientSession *allSessions)
{
    char jsonStr[512];
    int bytesWritten = snprintf(jsonStr, sizeof(jsonStr),
                                "{\"text\":\"%s has joined the game.\",\"color\":\"yellow\"}",
                                newSession->player.username);

    if (bytesWritten < 0 || bytesWritten >= sizeof(jsonStr))
    {
        printf("[ERROR] Failed to construct join message JSON.\n");
        return;
    }

    for (int i = 0; i < newSession->sessionCount; i++)
    {
        ClientSession *target = &allSessions[i];

        if (target->socket == -1 || target->state != STATE_PLAY)
            continue;

        Buffer buffer_chat_packet;
        uint8_t packet_id = 0x0F; // Clientbound Chat Message
        buffer_init(&buffer_chat_packet, 256);

        write_varInt_buffer(&buffer_chat_packet, packet_id);
        write_varInt_buffer(&buffer_chat_packet, strlen(jsonStr));
        buffer_append(&buffer_chat_packet, jsonStr, strlen(jsonStr));

        uint8_t type = 0x01; //system message
        buffer_append(&buffer_chat_packet, &type, sizeof(type));

        prepend_packet_length(&buffer_chat_packet);
        buffer_to_sendbuffer(target, &buffer_chat_packet);
        sendPacket(target);
        printf("%s has joined the game.\n",newSession->player.username);
        buffer_free(&buffer_chat_packet);
    }
}