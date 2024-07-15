#ifndef AESDSOCKET_H
#define AESDSOCKET_H

#include <stdbool.h>
#include <stdlib.h>
#include "socketserver.h"
#include "socketclient.h"

#define DATA_FILE           "/var/tmp/aesdsocketdata"
#define IP_LENGTH           16
struct aesd_socket
{
    char* buffer;
    size_t buff_size;
    socketserver_t server;

};
typedef struct aesd_socket aesdsocket_t;

aesdsocket_t* aesdsocket_ctor(size_t buffer_size);

void aesdsocket_dtor(aesdsocket_t* this);

bool aesdsocket_conf_server(aesdsocket_t* this, const char* socket_port);

bool aesdsocket_server_listen(aesdsocket_t* this);

bool aesdsocket_recv_routine(aesdsocket_t* this,socketclient_t* client);

bool aesdsocket_send_routine(aesdsocket_t* this,socketclient_t* client);

bool aesdsocket_start_process(aesdsocket_t* this);

bool aesdsocket_stop(aesdsocket_t* this);

#endif
