#ifndef AESDSOCKET_H
#define AESDSOCKET_H

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#define LISTEN_BACKLOG      1
#define INIT_BUFF_SIZE      1024
#define DATA_FILE           "/var/tmp/aesdsocketdata"
#define DEFAULT_PORT        9000

struct aesdsocket
{
    socklen_t peer_addr_size;
    struct sockaddr_storage peer_addr;
    struct addrinfo *res;
    int socketfd;
    int client_sfd;
    char* buffer;
    size_t data_size;
};
typedef struct aesdsocket aesdsocket_t;

aesdsocket_t* aesdsocket_ctor(struct addrinfo* hints);
void aesdsocket_dtor(aesdsocket_t* this);
bool aesdsocket_listen(aesdsocket_t* this);
bool aesdsocket_connect(aesdsocket_t* this,char* client_ip);
bool aesdsocket_recv(aesdsocket_t* this);
int aesdsocket_send(aesdsocket_t* this);
void aesdsocket_set_buffer(aesdsocket_t* this,char* buffer, size_t size);
void aesdsocket_get_buffer(aesdsocket_t* this,char* output_buffer, size_t* size);
//bool socket_initialize();
#endif