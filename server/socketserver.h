#ifndef SOCKETSERVER_H
#define SOCKETSERVER_H

#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include "socketclient.h"

struct aesd_server
{
    int socketfd;
    int listen_backlog;
    struct addrinfo server_info;
};
typedef struct aesd_server socketserver_t;

bool socketserver_setup(socketserver_t* this, const char* port, bool use_IPv6, int listen_backlog);

bool socketserver_listen(socketserver_t* this);

socketclient_t* socketserver_wait_conn(socketserver_t* this);

bool socketserver_close_conn(socketserver_t* this,socketclient_t* client);

int socketserver_close(socketserver_t* this);

ssize_t socketserver_recv(socketserver_t* this, socketclient_t* client ,char* buffer, size_t buff_size);

ssize_t socketserver_send(socketserver_t* this,socketclient_t* client , char* buffer, size_t buff_size);

#endif
