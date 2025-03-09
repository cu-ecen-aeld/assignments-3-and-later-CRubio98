#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "socketserver.h"
#include "aesdsocket.h"

bool waiting_cnn = true;

/**
 * @brief Signal handler for SIGINT and SIGTERM
 * @param signo Signal number
 */
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

/**
 * @brief Set up the actions for SIGINT and SIGTERM
 * @param action Pointer to the sigaction struct
 * @return true if the actions were set up, false otherwise
 */
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

/**
 * @brief Daemonize the process
 * @param daemon Boolean to set if the process will be daemonized
 */
void daemonize(bool daemon)
{
    // Create Process
    if (!daemon){return;}
    // Create Process
    pid_t pid = fork();
    if (pid == -1)
    {
        // Error creating child process
        syslog(LOG_ERR,"ERROR: Can not create child process");
        exit(EXIT_FAILURE);
    }
    if (pid > 0)
    {
        // we are in the parent process so exit
        syslog(LOG_INFO,"The PID of child process is = %d\n", pid);
        exit(EXIT_SUCCESS);
    }
    // We are the child
    int sid = setsid();
    if (sid < 0)
    {
        syslog(LOG_ERR, "couldn't create session");
        exit(EXIT_FAILURE);
    }
    if ((chdir("/")) < 0)
    {
        syslog(LOG_ERR, "couldnt change process work dir");
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

    if(!aesdsocket_conf_server(socket_port))
    {
        syslog(LOG_ERR, "aesdsocket server could not be configured");
        exit(EXIT_FAILURE);
    }

    daemonize(daemon);

    if(!aesdsocket_server_listen())
    {
        exit(EXIT_FAILURE);
    }

    // Start the application process
    aesdsocket_exec();

    return 0;
}
