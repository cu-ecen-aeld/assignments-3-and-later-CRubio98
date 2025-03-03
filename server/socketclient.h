#ifndef SOCKET_CLIENT_H
#define SOCKET_CLIENT_H
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#define CLIENT_IP           17
struct aesd_client
{
  int socketfd;
  struct sockaddr_storage peer_addr;
  char ip[CLIENT_IP];
};
typedef struct aesd_client socketclient_t;

socketclient_t* socketclient_ctor(void);

void socketclient_dtor(socketclient_t* this);

bool socketclient_setup(socketclient_t* this,int socketfd, struct sockaddr_storage peer_addr);

int socketclient_close(socketclient_t* this);

int socketclient_get_fd(socketclient_t* this);

socklen_t socketclient_get_addr_size(socketclient_t* this);

void socketclient_get_ip(socketclient_t* this, char* ip, size_t ip_length);


#endif // SOCKET_CLIENT_H