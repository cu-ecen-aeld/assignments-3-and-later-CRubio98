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

/**
 * @brief Create a new socket client
 *
 * @return socketclient_t* Pointer to the new socket client
 */
socketclient_t* socketclient_ctor(void);

/**
 * @brief Destroy a socket client
 *
 * @param this Pointer to the socket client to be destroyed
 */
void socketclient_dtor(socketclient_t* this);

/**
 * @brief Configures a ADT socketclient with the socket file descriptor and the peer address
 *
 * @param this Pointer to the socket client
 * @param socketfd File descriptor of the socket
 * @param peer_addr Address of the peer
 * @return true if the setup was successful, false otherwise
 */
bool socketclient_setup(socketclient_t* this,int socketfd, struct sockaddr_storage peer_addr);

/**
 * @brief Close the socket client conection
 *
 * @param this Pointer to the socket client
 * @return int 0 if the socket was closed, -1 otherwise
 */
int socketclient_close(socketclient_t* this);

/**
 * @brief Get the file descriptor of the socket client
 *
 * @param this Pointer to the socket client
 * @return int File descriptor of the socket client
 */
int socketclient_get_fd(socketclient_t* this);

/**
 * @brief Get the size of the address of the socket client
 *
 * @param this Pointer to the socket client
 * @return socklen_t Size of the address of the socket client
 */
socklen_t socketclient_get_addr_size(socketclient_t* this);

/**
 * @brief Get the IP of the socket client
 *
 * @param this Pointer to the socket client
 * @param ip Pointer to the buffer where the IP will be stored
 * @param ip_length Length of the buffer
 */
void socketclient_get_ip(socketclient_t* this, char* ip, size_t ip_length);

#endif // SOCKET_CLIENT_H
