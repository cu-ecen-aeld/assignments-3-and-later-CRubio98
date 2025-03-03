#include "aesdsocket.h"
#include "thread_list.h"


static socketserver_t s_server;
static char buffer[BUFF_SIZE];
static pthread_mutex_t file_mtx;

/*=================================PRIVATE FUNCTION DECLARATIONS=====================================*/

/* Thread functions */
static void* thread_connection(void* args);
/* ---------------- */

/* AESDSOCKET App functions */
static bool aesdsocket_recv_routine(socketclient_t* client);
static bool aesdsocket_send_routine(socketclient_t* client);
static bool aesdsocket_stop();
/* --------------------------*/


/*=================================PRIVATE FUNCTION DEFINITIONS=====================================*/


static void* thread_connection(void* args)
{
    // Catch the thread data
    thread_data_t* thread_info = (thread_data_t*)args;

    // Notify there is a connection from a client IP
    char client_ip[IP_LENGTH];
    socketclient_get_ip(thread_info->client,client_ip,IP_LENGTH);
    syslog(LOG_INFO,"Accepted connection from %s\n", client_ip);
    
    pthread_mutex_lock(&file_mtx);
    // Receive Routine
    aesdsocket_recv_routine(thread_info->client);

    // Send Routine
    if(aesdsocket_send_routine(thread_info->client) == false)
    {
        pthread_mutex_unlock(&file_mtx);
        syslog(LOG_ERR, "Error sending data to client");
        socketserver_close_conn(thread_info->client);
        return (void*)false;
    }

    pthread_mutex_unlock(&file_mtx);  
    thread_info->complete = true;

    // Notify closing connection in the client IP
    syslog(LOG_INFO,"Closed connection from %s\n", client_ip);
    if(socketserver_close_conn(thread_info->client))
    {
        syslog(LOG_ERR, "Error closing connection");
        return (void*)false;
    }
    closelog();
    return (void*)true;
}

static bool aesdsocket_recv_routine(socketclient_t* client)
{
    // Open aesdsocketdata
    int data_fd = open(DATA_FILE, O_RDWR | O_CREAT | O_APPEND, 0644);
    ssize_t bytes_received;

    if (data_fd < 0)
    {
        return false;
    }
    while((bytes_received=socketserver_recv(&s_server, client, buffer, BUFF_SIZE))> 0)
    {
        char* line_end;
        const char delim = '\n';
        //ssize_t bytes_written;
        if((line_end = memchr(buffer, delim, bytes_received)) != NULL)
        {
            size_t bytes_to_write = line_end - buffer + 1; // include the newline character
            write (data_fd, buffer, bytes_to_write);
            sync();
            break;
        }
        write(data_fd, buffer, bytes_received);
    }
    close(data_fd);
    return true;
}

static bool aesdsocket_send_routine(socketclient_t* client)
{
    // Open aesdsocketdata to read
    int data_fd = open(DATA_FILE, O_RDONLY, 0644);
    if (data_fd < 0)
    {
        return false;
    }
    ssize_t bytes_read;
    ssize_t bytes_send;
    while ((bytes_read = read(data_fd, buffer, BUFF_SIZE)) > 0)
    {
        if ((bytes_send=socketserver_send(&s_server, client, buffer,bytes_read)) == -1)
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

static bool aesdsocket_stop()
{
    bool closed=false;
    openlog("aesdsocket_stop", LOG_PID, LOG_USER);
    // Notify closing server
    syslog(LOG_INFO,"Turning Down Server Socket");
    if(socketserver_close(&s_server))
    {
        closed=true;
    }
    closelog();
    return closed;
}

/*=================================PUBLIC FUNCTION DEFINITIONS=====================================*/

bool aesdsocket_conf_server(const char* socket_port)
{

    // Start the socket server
    int backlog = 1;
    if(!socketserver_setup(&s_server,socket_port, false,backlog))
    {
        return false;
    }
    return true;
}

bool aesdsocket_server_listen()
{
    // Listen LISTEN_BACKLOG connections;
    bool listening=true;
    openlog("aesdsocket_listen", LOG_PID, LOG_USER);
    syslog(LOG_DEBUG,"Starting Listening...");
    if(!socketserver_listen(&s_server))
    {
       syslog(LOG_ERR, "Socket server could not listen");
       listening=false;
    }
    closelog();
    return listening;
}

void aesdsocket_exec()
{   
    extern bool waiting_cnn;
    openlog("aesdsocket_started", LOG_PID, LOG_USER);
    
    while(waiting_cnn == true)
    {
        socketclient_t* new_client;
        if((new_client=socketserver_wait_conn(&s_server)) == NULL)
        {
            perror("accept");
            socketclient_dtor(new_client);
            continue;
        }

        //create thread and prepare linked list
        thread_data_t thread_data;
        thread_data.client = new_client;
        thread_data.complete = false;
        threadList_init();

        syslog(LOG_INFO, "Starting Thread...");
        int ret= pthread_create(&thread_data.thread_id,NULL,thread_connection,NULL);
        if(ret != 0)
        {
            syslog(LOG_ERR,"Error creating thread");
            socketclient_dtor(new_client);
            continue;
        }
        threadList_insert(&thread_data);

        bool finished_threads = false;
        while(!finished_threads)
        {
            //check if any thread is complete
            int position=threadList_searchState(true);
            if (position != -1)
            {
                threadList_getAt(position,&thread_data);
                pthread_join(thread_data.thread_id,NULL);
                syslog(LOG_INFO,"Thread %ld is complete", thread_data.thread_id);
                threadList_removeAt(position);
            }
            else
            {
                finished_threads = true;
            }
        }
    }
    closelog();
    aesdsocket_stop();
    remove(DATA_FILE);
}

