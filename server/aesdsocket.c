#include <time.h>
#include <signal.h>
#include "aesdsocket.h"
#include "thread_list.h"


static socketserver_t s_server;
static char buffer[BUFF_SIZE];
static pthread_mutex_t file_mtx;

//static struct timerData
//{
//    int myData;
//};

/*=================================PRIVATE FUNCTION DECLARATIONS=====================================*/

/* Thread functions */
static void timer_thread(union sigval sigval);
static void* thread_connection(void* args);
/* ---------------- */

/* AESDSOCKET App functions */
static bool aesdsocket_recv_routine(socketclient_t* client);
static bool aesdsocket_send_routine(socketclient_t* client);
static bool aesdsocket_stop();
/* --------------------------*/


/*=================================PRIVATE FUNCTION DEFINITIONS=====================================*/

static void timer_thread(union sigval sigval)
{
    //struct thread_data *td = (struct thread_data*) sigval.sival_ptr;
    char timestampBuff[64];
    time_t current_time;
    struct tm *local_time;

    time(&current_time);
    local_time = localtime(&current_time);

    // turn it into RFC 2822 formatted timestamp
    strftime(timestampBuff, sizeof(timestampBuff), "timestamp:%a, %b %d %Y %H:%M:%S %z\n", local_time);

    if(pthread_mutex_lock(&file_mtx)==0)
    {
        int data_fd = open(DATA_FILE, O_RDWR | O_CREAT | O_APPEND, 0644);
        write (data_fd, timestampBuff, sizeof(timestampBuff));
        sync();
        close(data_fd);
        pthread_mutex_unlock(&file_mtx);
    }
}

static void* thread_connection(void* args)
{
    // Catch the thread data
    thread_data_t* thread_info = (thread_data_t*)args;

    // Notify there is a connection from a client IP
    char client_ip[IP_LENGTH];
    socketclient_get_ip(thread_info->client,client_ip,IP_LENGTH);
    syslog(LOG_INFO,"Accepted connection from %s\n", client_ip);
    
    if(pthread_mutex_lock(&file_mtx) == 0)
    {
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
    }
    pthread_mutex_unlock(&file_mtx);
    thread_info->complete = true;
    // Notify closing connection in the client IP
    syslog(LOG_INFO,"Closed connection from %s\n", client_ip);
    if(!socketserver_close_conn(thread_info->client))
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
    syslog(LOG_ERR,"Receiving data from client");
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
    timer_t timerId = 0;
    struct sigevent sEvent;

    pthread_mutex_init(&file_mtx,NULL);
    int clock_id = CLOCK_MONOTONIC;
    memset(&sEvent,0,sizeof(struct sigevent));
    sEvent.sigev_notify = SIGEV_THREAD;
    //sEvent.sigev_value.sival_ptr = &td;
    sEvent.sigev_notify_function = timer_thread;
    timer_create(clock_id,&sEvent,&timerId);

    struct itimerspec its = {   .it_value.tv_sec  = 10,
        .it_value.tv_nsec = 0,
        .it_interval.tv_sec  = 10,
        .it_interval.tv_nsec = 0
    };

    (void)its;
    timer_settime(timerId, 0, &its, NULL);

    while(waiting_cnn == true)
    {
        openlog("aesdsocket_started", LOG_PID, LOG_USER);
        socketclient_t* new_client;
        if((new_client=socketserver_wait_conn(&s_server)) == NULL)
        {
            perror("accept");
            socketclient_dtor(new_client);
            continue;
        }

        //create thread and prepare linked list
        thread_data_t* thread_data= (thread_data_t*)malloc(sizeof(thread_data_t));
        thread_data->client = new_client;
        thread_data->complete = false;
        threadList_init();

        syslog(LOG_INFO, "Starting Thread...");
        int ret= pthread_create(&thread_data->thread_id,NULL,thread_connection,(void*)thread_data);
        if(ret != 0)
        {
            syslog(LOG_ERR,"Error creating thread");
            socketclient_dtor(new_client);
            continue;
        }
        syslog(LOG_INFO, "Thread instanciated...");
        threadList_insert(thread_data);
        syslog(LOG_INFO, "Thread inserted at list...");
        bool list_end = false;
        int position=0;
        bool get_state;
        while(!list_end)
        {
            syslog(LOG_INFO, "Checking Threads in list...");
            //check if any thread is complete

            get_state=threadList_getAt(position,thread_data);
            if(!get_state)
            {
                list_end = true;
                syslog(LOG_INFO,"Finished List iteration");
                break;
            }
            if (thread_data->complete)
            {
                //threadList_getAt(position,thread_data);
                pthread_join(thread_data->thread_id,NULL);
                syslog(LOG_INFO,"Thread %ld is complete", thread_data->thread_id);
                threadList_removeAt(position);
                socketclient_dtor(thread_data->client);
                free(thread_data);
            }
            else
            {
                syslog(LOG_INFO,"Thread %ld is not complete", thread_data->thread_id);
            }
            position++;
        }
    }
    timer_delete(timerId);
    closelog();
    aesdsocket_stop();
    remove(DATA_FILE);
}

