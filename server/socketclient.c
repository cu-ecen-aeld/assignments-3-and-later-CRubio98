#include "socketclient.h"

socketclient_t* socketclient_ctor(size_t ip_length)
{
    socketclient_t* new_socketclient= (socketclient_t*) malloc(sizeof(socketclient_t));
    if(new_socketclient == NULL)
    {
        return NULL;
    }

    new_socketclient->ip_length=ip_length;
    new_socketclient->ip=(char*) malloc(ip_length+1);
    if(new_socketclient->ip == NULL)
    {
        free(new_socketclient);
        return NULL;
    }

    return new_socketclient;

}

void socketclient_dtor(socketclient_t* this)
{
    if(!this){return;}
    free(this->ip);
    free(this);
}

bool socketclient_setup(socketclient_t* this,int socketfd, struct sockaddr_storage peer_addr)
{
    this->socketfd = socketfd;
    this->peer_addr = peer_addr;
    // Get client IP
    inet_ntop(this->peer_addr.ss_family,
              &((struct sockaddr_in*)&(this->peer_addr))->sin_addr,
              this->ip, this->ip_length);

    return true;
}

int socketclient_close(socketclient_t* this)
{
    // Close the socket client file descriptor
    return close(this->socketfd);
}

int socketclient_get_fd(socketclient_t* this)
{
    return this->socketfd;
}

socklen_t socketclient_get_addr_size(socketclient_t* this)
{
    return sizeof(this->peer_addr);
}

void socketclient_get_ip(socketclient_t* this, char* ip, size_t ip_length)
{
    strncpy(ip,this->ip,ip_length);
    ip[ip_length]='\0'; //TODO: Check if this is necessary
}
