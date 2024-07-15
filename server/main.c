#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "socketserver.h"
#include "aesdsocket.h"

#define BUFF_SIZE           2046

bool waiting_cnn = true;

static void signal_handler (int signo)
{
    if(signo == SIGINT || signo == SIGTERM)
    {
        openlog("Signal_handler", LOG_PID, LOG_USER);
        syslog(LOG_INFO,"Caught signal, exiting");
        closelog();
        waiting_cnn = false;

    }
}

bool setup_actions(struct sigaction* action)
{
    action->sa_handler=signal_handler;

    if(sigaction(SIGTERM,action,NULL) != 0 ||
       sigaction(SIGINT,action,NULL)!= 0 )
    {
        syslog(LOG_ERR, "Sigaction Error");
        return false;
    }
    return true;
}

void daemonize(bool daemon,aesdsocket_t* socket_app)
{
    // Create Process
    if (!daemon){return;}
    // Create Process
    pid_t pid = fork();
    if (pid == -1)
    {
        // Error creating child process
        syslog(LOG_ERR,"ERROR: Can not create child process");
        aesdsocket_dtor(socket_app);
        exit(EXIT_FAILURE);
    }
    if (pid > 0)
    {
        // we are in the parent process so exit
        syslog(LOG_INFO,"The PID of child process is = %d\n", pid);
        aesdsocket_dtor(socket_app);
        exit(EXIT_SUCCESS);
    }
    // We are the child
    int sid = setsid();
    if (sid < 0)
    {
        syslog(LOG_ERR, "couldn't create session");
        aesdsocket_dtor(socket_app);
        exit(EXIT_FAILURE);
    }
    if ((chdir("/")) < 0)
    {
        syslog(LOG_ERR, "couldnt change process work dir");
        aesdsocket_dtor(socket_app);
        exit(EXIT_FAILURE);
    }
    dup2(open("/dev/null", O_RDWR), STDIN_FILENO);
    dup2(STDIN_FILENO, STDOUT_FILENO);
    dup2(STDOUT_FILENO, STDERR_FILENO);
    
}

int main(int argc, char* argv[])
{
    bool daemon = false;
    int opt;
    // Set Logs
    openlog("aesdsocket-main", LOG_PID, LOG_USER);
    // Set sigaction and handler
    struct sigaction finish_connection;
    memset(&finish_connection,0,sizeof(struct sigaction));
    setup_actions(&finish_connection);

    while ((opt = getopt(argc, argv, "d")) != -1)
    {
        switch (opt) {
            case 'd':
                daemon = true;
                break;
            default:
                fprintf(stderr, "Usage: %s [-d] (daemon)\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    // make a socket server:
    syslog(LOG_DEBUG, "Starting App");
    const char* socket_port="9000";
    aesdsocket_t* socket_app=aesdsocket_ctor(BUFF_SIZE);
    if(socket_app == NULL)
    {
        syslog(LOG_ERR, "Aesdsocket app could not be created");
        exit(EXIT_FAILURE);
    }

    if(!aesdsocket_conf_server(socket_app,socket_port))
    {
        syslog(LOG_ERR, "aesdsocket server could not be configured");
        exit(EXIT_FAILURE);
    }

    daemonize(daemon,socket_app);

    if(!aesdsocket_server_listen(socket_app))
    {
        aesdsocket_dtor(socket_app);
        exit(EXIT_FAILURE);
    }

    // Loop for connection
    while(waiting_cnn)
    {
        aesdsocket_start_process(socket_app);
    }
    aesdsocket_stop(socket_app);
    // Destroy Socket App
    aesdsocket_dtor(socket_app);
    return 0;
}
