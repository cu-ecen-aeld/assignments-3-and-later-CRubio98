#ifndef AESDSOCKET_H
#define AESDSOCKET_H

#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include "socketserver.h"
#include "socketclient.h"

#define BUFF_SIZE           2046
#define DATA_FILE           "/var/tmp/aesdsocketdata"
#define IP_LENGTH           16

/*=====================================FUNCTION DECLARATIONS=====================================*/

/**
 * @brief Sets the port to be used by the application server
 *
 * @param socket_port Port to be used by the aesdsocket application
 * @return true if the application can run in the desired port, false otherwise
 */
bool aesdsocket_conf_server(const char* socket_port);

/**
 * @brief Starts the server to listen for incoming connections
 *
 * @return true if the server is listening, false otherwise
 */
bool aesdsocket_server_listen(void);

/**
 * @brief Executes the aesdsocket application
 *
 */
void aesdsocket_exec(void);

#endif
