#include "server.h"
#include <string.h>
#include <stdio.h>
#define USERNAME_LEN 16
#define MAX_KNOWN_PLAYERS 5

int is_known_player_by_name(ClientSession *session, const char *username)
{
    for (int i = 0; i < session->knownCount; i++)
    {
        if (strncmp(session->knownPlayers[i], username, USERNAME_LEN) == 0)
        {
            return 1;
        }
    }
    return 0;
}

void add_known_player_by_name(ClientSession *session, const char *username)
{
    if (session->knownCount < MAX_KNOWN_PLAYERS)
    {
        strncpy(session->knownPlayers[session->knownCount], username, USERNAME_LEN);
        session->knownPlayers[session->knownCount][USERNAME_LEN - 1] = '\0'; // safety null-terminator
        session->knownCount++;
    }
}

void remove_known_player_by_name(ClientSession *session, const char *username)
{
    for (int i = 0; i < session->knownCount; i++)
    {
        if (strncmp(session->knownPlayers[i], username, USERNAME_LEN) == 0)
        {
            // Shift the rest down
            for (int j = i; j < session->knownCount - 1; j++)
            {
                strncpy(session->knownPlayers[j], session->knownPlayers[j + 1], USERNAME_LEN);
            }
            session->knownCount--;
            break;
        }
    }
}
void is_player_in_view(ClientSession *sourceSession, ClientSession *allSessions)
{

    for (int i = 0; i < sourceSession->sessionCount; i++)
    {

        ClientSession *target = &allSessions[i];

        if (target->socket != -1 && sourceSession != target && target->state == STATE_PLAY)
        {
            double dx = sourceSession->player.x - target->player.x;
            double dz = sourceSession->player.z - target->player.z;
            double distSq = dx * dx + dz * dz;
            if (distSq < 100) // hardcoded for now
            {

                if (!is_known_player_by_name(sourceSession, target->username))
                {
                    spawn_player_packet(sourceSession, target);
                    add_known_player_by_name(sourceSession, target->username);
                }
            }

            else
            {
                // TODO
                if (is_known_player_by_name(sourceSession, target->username))
                {
                    // destroy_entity_packet(sourceSession, target->player.eid);
                    // remove_known_player_by_name(sourceSession, target->username);
                }
            }
        }
    }
}
int load_player_from_file(const char *filename, const char *search_username, ClientSession *session)
{
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        printf("[ERROR] Failed to open file %s.\n", filename);
        return -1;
    }
    char line[1024];
    while (fgets(line, sizeof(line), file))
    {
        char file_uuid[36], file_username[256], skinUrl[512];
        float x, y, z;

        int scanned = sscanf(line, "%35[^;];%255[^;];%f;%f;%f;%511[^\n]",
                             file_uuid, file_username, &x, &y, &z, skinUrl);

        if (scanned == 6)
        {
            if (strcmp(file_username, search_username) == 0)
            {
                strncpy(session->player.username, file_username, sizeof(session->player.username));
                session->player.username[sizeof(session->player.username) - 1] = '\0';

                strncpy(session->player.uuid, file_uuid, sizeof(session->player.uuid));
                session->player.uuid[sizeof(session->player.uuid) - 1] = '\0';

                session->player.x = x;
                session->player.y = y;
                session->player.z = z;

                strncpy(session->player.skinUrl, skinUrl, sizeof(session->player.skinUrl));
                session->player.skinUrl[sizeof(session->player.skinUrl) - 1] = '\0';

                fclose(file);
                printf("[INFO] Loaded player %s (UUID: %s)\n", session->player.username, session->player.uuid);
                return 0;
            }
        }
        else
        {
            printf("[WARN] Skipping malformed line: %s", line);
        }
    }

    fclose(file);
    printf("[ERROR] Player with username '%s' not found in file.\n", search_username);
    return -1;
}

void normalize_yaw(ClientSession *session)
{
    while (session->player.yaw < 0.0f)
        session->player.yaw += 360.0f;
    while (session->player.yaw >= 360.0f)
        session->player.yaw -= 360.0f;
}