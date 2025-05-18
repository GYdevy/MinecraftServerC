#include "server.h"
#include "packet_utils.h"
#include <math.h>
#include <stdbool.h>
void handle_player_position(ClientSession *session, uint8_t *packet)
{
    packet += 1;
    double x = convert_double_little_endian_to_big_endian(packet);
    packet += 8;
    double y = convert_double_little_endian_to_big_endian(packet);
    packet += 8;
    double z = convert_double_little_endian_to_big_endian(packet);
    packet += 8;

    uint8_t onGround = *packet;

    update_last_pos(session);

    session->player.x = x;
    session->player.y = y;
    session->player.z = z;

    session->player.onGround = onGround;
}
void handle_player_position_rotation(ClientSession *session, uint8_t *packet)
{
    packet += 1;
    double x = convert_double_little_endian_to_big_endian(packet);
    packet += 8;
    double y = convert_double_little_endian_to_big_endian(packet);
    packet += 8;
    double z = convert_double_little_endian_to_big_endian(packet);
    packet += 8;
    float yaw = convert_float_little_endian_to_big_endian(packet);
    packet += 4;
    float pitch = convert_float_little_endian_to_big_endian(packet);
    packet += 4;

    uint8_t onGround = *packet;

    update_last_pos(session);
    update_last_rot(session);
    session->player.x = x;
    session->player.y = y;
    session->player.z = z;
    session->player.yaw = yaw;
    session->player.pitch = pitch;
    normalize_yaw(session);
    session->player.onGround = onGround;
}
void handle_player_rotation(ClientSession *session, uint8_t *packet)
{
    packet += 1;
    float yaw = convert_float_little_endian_to_big_endian(packet);
    packet += 4;
    float pitch = convert_float_little_endian_to_big_endian(packet);
    packet += 4;

    uint8_t onGround = *packet;

    update_last_rot(session);
    session->player.yaw = yaw;
    session->player.pitch = pitch;
    normalize_yaw(session);
    session->player.onGround = onGround;
}
void handle_player_movement(ClientSession *session, uint8_t *packet)
{
    packet += 1;
    uint8_t onGround = *packet;
    session->player.lastOnGround = session->player.onGround;
    session->player.onGround = onGround;
}

// used for delta calculations
void update_last_pos(ClientSession *session)
{

    session->player.lastX = session->player.x;
    session->player.lastY = session->player.y;
    session->player.lastZ = session->player.z;
    session->player.lastOnGround = session->player.onGround;
}
update_last_rot(ClientSession *session)
{

    session->player.lastPitch = session->player.pitch;
    session->player.lastYaw = session->player.yaw;
    session->player.lastOnGround = session->player.onGround;
}

void broadcast_movement(ClientSession *source, ClientSession *target)
{
    // Calculate position deltas (fixed-point format: 4096 = 32 * 128)
    double dx = source->player.x - source->player.lastX;
    double dy = source->player.y - source->player.lastY;
    double dz = source->player.z - source->player.lastZ;

    int16_t deltaX = (int16_t)(dx * 4096);
    int16_t deltaY = (int16_t)(dy * 4096);
    int16_t deltaZ = (int16_t)(dz * 4096);
    uint16_t big_deltaX = convert_short_little_endian_to_big_endian(deltaX);
    uint16_t big_deltaY = convert_short_little_endian_to_big_endian(deltaY);
    uint16_t big_deltaZ = convert_short_little_endian_to_big_endian(deltaZ);
    // Calculate rotation deltas
    float deltaYaw = source->player.yaw - source->player.lastYaw;
    float deltaPitch = source->player.pitch - source->player.lastPitch;

    bool moved = (big_deltaX != 0 || big_deltaY != 0 || big_deltaZ != 0);
    bool rotated = (fabs(deltaYaw) > 0.01f || fabs(deltaPitch) > 0.01f);

    if (fabs(deltaYaw) > 0.001f)
    {
        send_entity_head_rotation_packet(source, target, source->player.yaw);
    }

    if (!moved && !rotated)
        return;

    // If movement is too large for a relative move, teleport instead
    if (abs(deltaX) > 32767 || abs(deltaY) > 32767 || abs(deltaZ) > 32767)
    {
        printf("[DEBUG] Large movement detected, should teleport\n");
        // send_entity_teleport_packet(source, target);
    }
    else if (moved && rotated)
    {
        send_entity_position_rotation_packet(source, target, big_deltaX, big_deltaY, big_deltaZ);
    }
    else if (moved)
    {
        send_entity_position_packet(source, target, big_deltaX, big_deltaY, big_deltaZ);
    }
    else if (rotated)
    {
        send_entity_rotation_packet(source, target);
    }

    // Update last known values
    source->player.lastX = source->player.x;
    source->player.lastY = source->player.y;
    source->player.lastZ = source->player.z;
    source->player.lastYaw = source->player.yaw;
    source->player.lastPitch = source->player.pitch;
}

void send_entity_position_packet(ClientSession *source, ClientSession *target, short deltaX, short deltaY, short deltaZ)
{
    Buffer buffer;
    buffer_init(&buffer, 256);

    uint8_t packet_id = 0x29; // Entity Position packet ID
    buffer_append(&buffer, &packet_id, sizeof(packet_id));
    write_varInt_buffer(&buffer, source->player.eid);
    buffer_append(&buffer, &deltaX, sizeof(deltaX));
    buffer_append(&buffer, &deltaY, sizeof(deltaY));
    buffer_append(&buffer, &deltaZ, sizeof(deltaZ));
    buffer_append(&buffer, &source->player.onGround, sizeof(uint8_t));

    prepend_packet_length(&buffer);
    buffer_to_sendbuffer(target, &buffer);

    sendPacket(target);
    buffer_free(&buffer);
}
void send_entity_head_rotation_packet(ClientSession *source, ClientSession *target, float yaw_degrees)
{
    Buffer buffer;
    buffer_init(&buffer, 16);

    uint8_t packet_id = 0x3C; // Entity Head Look packet ID
    normalize_yaw(source);
    uint8_t yaw_byte = (uint8_t)((source->player.yaw / 360.0f) * 256.0f);

    buffer_append(&buffer, &packet_id, sizeof(packet_id));
    write_varInt_buffer(&buffer, source->player.eid);    // Entity ID
    buffer_append(&buffer, &yaw_byte, sizeof(yaw_byte)); // Head yaw byte

    prepend_packet_length(&buffer);

    buffer_to_sendbuffer(target, &buffer);
    sendPacket(target);

    buffer_free(&buffer);
}

void send_entity_rotation_packet(ClientSession *source, ClientSession *target)
{
    Buffer buffer;
    buffer_init(&buffer, 256);
    uint8_t yaw_byte = (uint8_t)((source->player.yaw / 360.0f) * 256.0f);
    uint8_t pitch_byte = (uint8_t)((source->player.pitch / 360.0f) * 256.0f);
    uint8_t packet_id = 0x2B; // Entity Rotation packet ID
    buffer_append(&buffer, &packet_id, sizeof(packet_id));
    write_varInt_buffer(&buffer, source->player.eid);
    buffer_append(&buffer, &yaw_byte, sizeof(yaw_byte));
    buffer_append(&buffer, &pitch_byte, sizeof(pitch_byte));
    buffer_append(&buffer, &source->player.onGround, sizeof(uint8_t));

    prepend_packet_length(&buffer);
    buffer_to_sendbuffer(target, &buffer);

    sendPacket(target);
    buffer_free(&buffer);
}
void send_entity_position_rotation_packet(ClientSession *source, ClientSession *target, short deltaX, short deltaY, short deltaZ)
{
    Buffer buffer;
    buffer_init(&buffer, 256);
    uint8_t yaw_byte = (uint8_t)((source->player.yaw / 360.0f) * 256.0f);
    uint8_t pitch_byte = (uint8_t)((source->player.pitch / 360.0f) * 256.0f);
    uint8_t packet_id = 0x2A; // Entity Position and Rotation packet ID
    buffer_append(&buffer, &packet_id, sizeof(packet_id));
    write_varInt_buffer(&buffer, source->player.eid);
    buffer_append(&buffer, &deltaX, sizeof(deltaX));
    buffer_append(&buffer, &deltaY, sizeof(deltaY));
    buffer_append(&buffer, &deltaZ, sizeof(deltaZ));
    buffer_append(&buffer, &yaw_byte, sizeof(yaw_byte));
    buffer_append(&buffer, &pitch_byte, sizeof(pitch_byte));
    buffer_append(&buffer, &source->player.onGround, sizeof(uint8_t));

    prepend_packet_length(&buffer);
    buffer_to_sendbuffer(target, &buffer);

    sendPacket(target);
    buffer_free(&buffer);
}
void send_entity_movement(ClientSession *source, ClientSession *target)
{
    Buffer buffer;
    buffer_init(&buffer, 64);
    uint8_t packet_id = 0x2C; // Entity Movement
    buffer_append(&buffer, &packet_id, sizeof(packet_id));
    write_varInt_buffer(&buffer, source->player.eid);
    prepend_packet_length(&buffer);
    buffer_to_sendbuffer(target, &buffer);
    sendPacket(target);
    buffer_free(&buffer);
}

void send_player_arm_swing(ClientSession *session, ClientSession sessions[])
{
    Buffer buffer_anim;
    buffer_init(&buffer_anim, 16);

    uint8_t packet_id = 0x06;
    buffer_append(&buffer_anim, &packet_id, sizeof(packet_id));

    write_varInt_buffer(&buffer_anim, session->player.eid);
    uint8_t animation = 0x00; //  main hand
    buffer_append(&buffer_anim, &animation, sizeof(animation));

    prepend_packet_length(&buffer_anim);

    // Send to all other clients
    for (int i = 0; i < sessionCount; i++)
    {

        if (&sessions[i] == session || sessions[i].state != STATE_PLAY)
            continue;

        buffer_to_sendbuffer(&sessions[i], &buffer_anim);
        sendPacket(&sessions[i]);
    }

    buffer_free(&buffer_anim);
}