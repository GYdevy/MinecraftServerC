

#ifndef HANDSHAKE_H
#define HANDSHAKE_H

#include <stdint.h>
#include <sys/socket.h>   
#include <netinet/in.h>    

#include "server.h"
#include "packet_utils.h"




void handle_ping_pong(ClientSession *session);

void handshake(ClientSession *session, uint8_t *data, int length);

void build_send_status_response(ClientSession *session);

#endif // HANDSHAKE_H
