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

/**
 * @brief Create a Unix Socket Server and stores the information in socketserver_t struct
 *
 * @return true if the server was created, false otherwise
 */
bool socketserver_setup(socketserver_t* this, const char* port, bool use_IPv6, int listen_backlog);

/**
 * @brief Start listening for incoming connections
 *
 * @param this Pointer to the socket server
 * @return true if the server is listening, false otherwise
 */
bool socketserver_listen(socketserver_t* this);

/**
 * @brief Wait for a connection from a client and retrieves a new client object with the connection information
 *
 * @param this Pointer to the socket server
 * @return socketclient_t* Pointer to the new client
 */
socketclient_t* socketserver_wait_conn(socketserver_t* this);

/**
 * @brief Close the connection with the client
 *
 * @param client Pointer to the client to be closed
 * @return true if the connection was closed, false otherwise
 */
bool socketserver_close_conn(socketclient_t* client);

/**
 * @brief Close the socket server
 *
 * @param this Pointer to the socket server
 * @return int 0 if the server was closed, -1 otherwise
 */
int socketserver_close(socketserver_t* this);

/**
 * @brief Receive data from the client
 *
 * @param this Pointer to the socket server
 * @param client Pointer to the client
 * @param buffer Pointer to the buffer where the data will be stored
 * @param buff_size Size of the buffer
 * @return ssize_t Number of bytes received
 */
ssize_t socketserver_recv(socketserver_t* this, socketclient_t* client ,char* buffer, size_t buff_size);

/**
 * @brief Send data to the client
 *
 * @param this Pointer to the socket server
 * @param client Pointer to the client
 * @param buffer Pointer to the buffer with the data to be sent
 * @param buff_size Size of the buffer
 * @return ssize_t Number of bytes sent
 */
ssize_t socketserver_send(socketserver_t* this,socketclient_t* client , char* buffer, size_t buff_size);

#endif
