#include "socketserver.h"

socketserver_t* socketserver_ctor(void)
{
    socketserver_t* new_socketserver = (socketserver_t*) malloc(sizeof(socketserver_t));
    if(new_socketserver != NULL)
    {
        // Get size of struct
        new_socketserver->peer_addr_size=sizeof new_socketserver->peer_addr;
    }

    return new_socketserver;
}

void socketserver_dtor(socketserver_t* this)
{
    if(!this){return;}

    // Close the socket server file descriptor
    close(this->socketfd);
    free(this);
}

bool socketserver_setup(socketserver_t* this, const char* port, bool use_IPv6)
{
    openlog("aesdsocket_setup", LOG_PID, LOG_USER);

    // first, load up address structs with getaddrinfo():
    struct addrinfo conf_values;

    memset(&conf_values, 0, sizeof conf_values);
    if(use_IPv6){conf_values.ai_family = AF_INET6;}
    else{ conf_values.ai_family = AF_INET;}
    conf_values.ai_socktype = SOCK_STREAM;
    conf_values.ai_flags = AI_PASSIVE;     // fill in my IP for me

    // We will get addrs info from hints argument
    struct addrinfo *res;
    getaddrinfo(NULL, port, &conf_values, &res);

    // Get a new socket file descriptor
    this->socketfd = socket(res->ai_family,
                                      res->ai_socktype,
                                      res->ai_protocol);

    if(this->socketfd == -1)
    {
        syslog(LOG_ERR,"Error opening socket server fd");
        return false;
    }

    // Reuse Address
    const int tmp = 1;
    if (setsockopt(this->socketfd, SOL_SOCKET, SO_REUSEADDR, &tmp, sizeof(int)) == -1)
    {
        syslog(LOG_ERR,"Failed to set SO_REUSEADDR");
        close(this->socketfd);
        return false;
    }

    // bind it to the port we passed in to getaddrinfo():
    if(bind(this->socketfd,res->ai_addr,
            res->ai_addrlen) == -1)
    {
        syslog(LOG_ERR,"Error binding desired port");
        close(this->socketfd); 
        return false;
    }

    // Free res addr info
    freeaddrinfo(res);
    return true;
}

bool socketserver_listen(socketserver_t* this)
{
    bool listening = true;
    if(listen(this->socketfd, LISTEN_BACKLOG) == -1)
    {
        listening=false;
    }

    return listening;
}

bool socketserver_connect(socketserver_t* this,char* client_ip, size_t ip_length)
{
    this->client_sfd= accept(this->socketfd,(struct sockaddr *) &this->peer_addr,
                             &this->peer_addr_size);

    if(this->client_sfd == -1)
    {
        return false;
    }

    // Get client IP
    inet_ntop(this->peer_addr.ss_family,
              &((struct sockaddr_in*)&(this->peer_addr))->sin_addr,
              client_ip, ip_length);

    return true;
}
bool socketserver_close_connection(socketserver_t* this)
{
    bool closed=false;
    if(close(this->client_sfd)!= -1)
    {
        closed=true;
    }
    return closed;
}
ssize_t socketserver_recv(socketserver_t* this, char* buffer, size_t buff_size)
{
    return recv(this->client_sfd, buffer, buff_size-1,0);
}

ssize_t socketserver_send(socketserver_t* this, char* buffer, size_t buff_size)
{
   return send(this->client_sfd, buffer, buff_size, 0);
}
