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

bool aesdsocket_conf_server(const char* socket_port);
bool aesdsocket_server_listen(void);
void aesdsocket_exec(void);

#endif
