#ifndef SOCKETSERVER_H
#define SOCKETSERVER_H

#include <arpa/inet.h>
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

#define LISTEN_BACKLOG      1

struct aesd_server
{
    int client_sfd;
    int socketfd;
    socklen_t peer_addr_size;
    struct sockaddr_storage peer_addr;
};
typedef struct aesd_server socketserver_t;

socketserver_t* socketserver_ctor(void);

void socketserver_dtor(socketserver_t* this);

bool socketserver_setup(socketserver_t* this, const char* port, bool use_IPv6);

bool socketserver_listen(socketserver_t* this);

bool socketserver_connect(socketserver_t* this,char* client_ip);

bool socketserver_close_connection(socketserver_t* this);

ssize_t socketserver_recv(socketserver_t* this, char* buffer, size_t buff_size);

ssize_t socketserver_send(socketserver_t* this, char* buffer, size_t buff_size);

#endif
