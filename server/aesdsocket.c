#include "aesdsocket.h"

aesdsocket_t* aesdsocket_ctor(void)
{
    aesdsocket_t* new_aesdsocket = (aesdsocket_t*) malloc(sizeof(aesdsocket_t));
    if(new_aesdsocket != NULL)
    {
        // Get size of struct
        new_aesdsocket->peer_addr_size=sizeof new_aesdsocket->peer_addr;
    }

    return new_aesdsocket;
}

void aesdsocket_dtor(aesdsocket_t* this)
{
    if(!this){return;}

    // Close the socket server file descriptor
    close(this->socketfd);
    free(this);
}

bool aesdsocket_setup_server(aesdsocket_t* this, struct addrinfo hints)
{
    openlog("aesdsocket_setup", LOG_PID, LOG_USER);
    // We will get addrs info from hints argument
    struct addrinfo *res;
    getaddrinfo(NULL, DEFAULT_PORT, &hints, &res);

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

bool aesdsocket_listen(aesdsocket_t* this)
{
    bool listening = true;
    if(listen(this->socketfd, LISTEN_BACKLOG) == -1)
    {
        listening=false;
    }

    return listening;
}

bool aesdsocket_connect(aesdsocket_t* this,char* client_ip)
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
              client_ip, INET_ADDRSTRLEN);

    return true;
}
bool aesdsocket_close_connection(aesdsocket_t* this)
{
    bool closed=false;
    if(close(this->client_sfd)!= -1)
    {
        closed=true;
    }
    return closed;
}
ssize_t aesdsocket_recv(aesdsocket_t* this, char* buffer, size_t buff_size)
{
    return recv(this->client_sfd, buffer, buff_size-1,0);
}

ssize_t aesdsocket_send(aesdsocket_t* this, char* buffer, size_t buff_size)
{
   return send(this->client_sfd, buffer, buff_size, 0);
}
