#ifndef AESDSOCKET_H
#define AESDSOCKET_H

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdlib.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#define LISTEN_BACKLOG      1
#define DEFAULT_PORT        "9000"

struct aesdsocket
{
    int client_sfd;
    int socketfd;
    socklen_t peer_addr_size;
    struct sockaddr_storage peer_addr;
};
typedef struct aesdsocket aesdsocket_t;

aesdsocket_t* aesdsocket_ctor(void);

void aesdsocket_dtor(aesdsocket_t* this);

bool aesdsocket_setup_server(aesdsocket_t* this, struct addrinfo hints);

bool aesdsocket_listen(aesdsocket_t* this);

bool aesdsocket_connect(aesdsocket_t* this,char* client_ip);

bool aesdsocket_close_connection(aesdsocket_t* this);

ssize_t aesdsocket_recv(aesdsocket_t* this, char* buffer, size_t buff_size);

ssize_t aesdsocket_send(aesdsocket_t* this, char* buffer, size_t buff_size);

#endif
