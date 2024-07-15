#include "aesdsocket.h"

aesdsocket_t* aesdsocket_ctor(size_t buffer_size)
{
    aesdsocket_t* new_aesdsocket= (aesdsocket_t*) malloc(sizeof(aesdsocket_t));
    if(new_aesdsocket == NULL)
    {
        return NULL;
    }

    new_aesdsocket->buff_size=buffer_size;

    new_aesdsocket->buffer= (char*) malloc(buffer_size);
    if(new_aesdsocket->buffer == NULL)
    {
        free(new_aesdsocket);
        return NULL;
    }

    return new_aesdsocket;
}

void aesdsocket_dtor(aesdsocket_t* this)
{
    if(!this){return;}

    free(this->buffer);
    free(this);
    remove(DATA_FILE);
}

bool aesdsocket_conf_server(aesdsocket_t* this, const char* socket_port)
{

    // Start the socket server
    int backlog = 1;
    if(!socketserver_setup(&this->server,socket_port, false,backlog))
    {
        return false;
    }
    return true;
}

bool aesdsocket_server_listen(aesdsocket_t* this)
{
    // Listen LISTEN_BACKLOG connections;
    bool listening=true;
    openlog("aesdsocket_listen", LOG_PID, LOG_USER);
    syslog(LOG_DEBUG,"Starting Listening...");
    if(!socketserver_listen(&this->server))
    {
       syslog(LOG_ERR, "Socket server could not listen");
       listening=false;
    }
    closelog();
    return listening;
}

bool aesdsocket_recv_routine(aesdsocket_t* this,socketclient_t* client)
{
    // Open aesdsocketdata
    int data_fd = open(DATA_FILE, O_RDWR | O_CREAT | O_APPEND, 0644);
    ssize_t bytes_received;

    if (data_fd < 0)
    {
        return false;
    }
    while((bytes_received=socketserver_recv(&this->server, client, this->buffer, this->buff_size))> 0)
    {
        char* line_end;
        const char delim = '\n';
        //ssize_t bytes_written;
        if((line_end = memchr(this->buffer, delim, bytes_received)) != NULL)
        {
            size_t bytes_to_write = line_end - this->buffer + 1; // include the newline character
            ///bytes_written = 
            write (data_fd, this->buffer, bytes_to_write);
            //if (bytes_written == -1 || bytes_written != bytes_to_write)
            //{
            //    syslog(LOG_ERR,"Error ocurred while writing your string");
            //}
            sync();
            break;
        }
        write(data_fd,this->buffer,bytes_received);
    }
    close(data_fd);
    return true;
}

bool aesdsocket_send_routine(aesdsocket_t* this,socketclient_t* client)
{
    // Open aesdsocketdata to read
    int data_fd = open(DATA_FILE, O_RDONLY, 0644);
    if (data_fd < 0)
    {
        return false;
    }
    ssize_t bytes_read;
    ssize_t bytes_send;
    while ((bytes_read = read(data_fd, this->buffer, this->buff_size)) > 0)
    {
        if ((bytes_send=socketserver_send(&this->server, client, this->buffer,bytes_read)) == -1)
        {
            close(data_fd);
            return false;
        }
    }
    if (bytes_read < 0)
    {
        close(data_fd);
        return false;
    }
    close(data_fd);
    return true;
}

bool aesdsocket_start_process(aesdsocket_t* this)
{
    openlog("aesdsocket_started", LOG_PID, LOG_USER);
    socketclient_t* new_client;
    if((new_client=socketserver_wait_conn(&this->server)) == NULL)
    {
        perror("accept");
        socketclient_dtor(new_client);
        return false;
    }
    // Notify there is a connection from a client IP
    char client_ip[IP_LENGTH];
    socketclient_get_ip(new_client,client_ip,IP_LENGTH);
    syslog(LOG_INFO,"Accepted connection from %s\n", client_ip);

     // Receive Routine
     aesdsocket_recv_routine(this, new_client);
     // Send Routine
     if(aesdsocket_send_routine(this,new_client) == false)
     {
         syslog(LOG_ERR, "Error sending data to client");
         socketserver_close_conn(&this->server,new_client);
         socketclient_dtor(new_client);
         return false;
         //socketserver_close_connection(my_socket);
         //socketserver_dtor(my_socket);
         //remove(DATA_FILE);
         //exit(EXIT_FAILURE);
     }
    // Notify closing connection in the client IP
    syslog(LOG_INFO,"Closed connection from %s\n", client_ip);
    if(socketserver_close_conn(&this->server,new_client))
    {
        syslog(LOG_ERR, "Error closing connection");
        socketclient_dtor(new_client);
        return false;
    }
    closelog();
    socketclient_dtor(new_client);
    return true;
}

bool aesdsocket_stop(aesdsocket_t* this)
{
    bool closed=false;
    openlog("aesdsocket_stop", LOG_PID, LOG_USER);
    // Notify closing server
    syslog(LOG_INFO,"Turning Down Server Socket");
    if(socketserver_close(&this->server))
    {
        closed=true;
    }
    closelog();
    return closed;
}

