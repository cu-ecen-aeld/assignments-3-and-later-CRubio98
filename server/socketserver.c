#include "socketserver.h"

bool socketserver_setup(socketserver_t* this, const char* port, bool use_IPv6, int listen_backlog)
{
    openlog("socket_setup", LOG_PID, LOG_USER);

    this->listen_backlog = listen_backlog;

    // first, load up address structs with getaddrinfo():

    memset(&this->server_info, 0, sizeof (struct addrinfo));
    if(use_IPv6){this->server_info.ai_family = AF_INET6;}
    else{ this->server_info.ai_family = AF_INET;}
    this->server_info.ai_socktype = SOCK_STREAM;
    this->server_info.ai_flags = AI_PASSIVE;     // fill in my IP for me

    // We will get complete server host info with our required settings
    struct addrinfo* res;
    getaddrinfo(NULL, port, &this->server_info, &res);

    // Get a new socket file descriptor
    this->socketfd = socket(res->ai_family,
                            res->ai_socktype,
                            res->ai_protocol);

    if(this->socketfd == -1)
    {
        syslog(LOG_ERR,"Error opening socket server fd");
        closelog();
        return false;
    }

    // Reuse Address setting
    const int tmp = 1;
    if (setsockopt(this->socketfd, SOL_SOCKET, SO_REUSEADDR, &tmp, sizeof(int)) == -1)
    {
        syslog(LOG_ERR,"Failed to set SO_REUSEADDR");
        close(this->socketfd);
        closelog();
        return false;
    }

    // bind it to the port we passed in to getaddrinfo():
    if(bind(this->socketfd,res->ai_addr,
           res->ai_addrlen) == -1)
    {
        syslog(LOG_ERR,"Error binding desired port");
        close(this->socketfd);
        closelog();
        return false;
    }

    // Free res addr info
    freeaddrinfo(res);
    closelog();
    return true;
}

bool socketserver_listen(socketserver_t* this)
{
    bool listening = true;
    if(listen(this->socketfd, this->listen_backlog) == -1)
    {
        listening=false;
    }

    return listening;
}

socketclient_t* socketserver_wait_conn(socketserver_t* this)
{
    struct sockaddr_storage client_addr;
    socklen_t client_addr_size = sizeof(client_addr);
    //int client_ip_length;

    int client_fd= accept(this->socketfd,(struct sockaddr *) &client_addr,
                             &client_addr_size);

    if(client_fd == -1)
    {
        return NULL;
    }
    /*if( this->server_info.ai_family == AF_INET6)
    {
        client_ip_length = INET6_ADDRSTRLEN;
    }
    else
    {
        client_ip_length = INET_ADDRSTRLEN;
    }*/

    socketclient_t* client = socketclient_ctor();

    if(!socketclient_setup(client, client_fd, client_addr))
    {
        return NULL;
    }

    return client;
}

bool socketserver_close_conn(socketclient_t* client)
{
    bool closed=false;
    if(socketclient_close(client)!= -1)
    {
        closed=true;
    }
    return closed;
}

int socketserver_close(socketserver_t* this)
{
    // Close the socket server file descriptor
    return close(this->socketfd);
}

ssize_t socketserver_recv(socketserver_t* this, socketclient_t* client ,char* buffer, size_t buff_size)
{
    return recv(socketclient_get_fd(client), buffer, buff_size-1,0);
}

ssize_t socketserver_send(socketserver_t* this,socketclient_t* client , char* buffer, size_t buff_size)
{
   return send(socketclient_get_fd(client), buffer, buff_size, 0);
}
