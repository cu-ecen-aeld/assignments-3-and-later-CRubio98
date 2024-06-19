#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "socketserver.h"

#define BUFF_SIZE           2046
#define DATA_FILE           "/var/tmp/aesdsocketdata"

bool waiting_cnn = true;

static void signal_handler (int signo)
{
    if(signo == SIGINT || signo == SIGTERM)
    {
        syslog(LOG_INFO,"Caught signal, exiting");
        waiting_cnn = false;

    }
}

int main(int argc, char* argv[])
{
    bool daemon = false;
    int opt;

    // Set sigaction and handler
    struct sigaction finish_connection;
    memset(&finish_connection,0,sizeof(struct sigaction));
    finish_connection.sa_handler=signal_handler;

    if(sigaction(SIGTERM,&finish_connection,NULL) != 0 ||
       sigaction(SIGINT,&finish_connection,NULL)!= 0 )
    {
        syslog(LOG_ERR, "Sigaction Error");
        perror("Error when setting signals");
        exit(EXIT_FAILURE);
    }
    // Check for daemon mode
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
    // Set Logs
    openlog(NULL, LOG_PID, LOG_USER);

    // make a socket:
    socketserver_t* my_socket=socketserver_ctor();
    const char* socket_port="9000";
    if(my_socket == NULL)
    {
        syslog(LOG_ERR, "Socker Server instance could not be created");
        exit(EXIT_FAILURE);
    }
    if(!socketserver_setup(my_socket,socket_port, false))
    {
        syslog(LOG_ERR,"ERROR: Socket Server could not be setup");
        exit(EXIT_FAILURE);
    }

    if (daemon)
    {
        // Create Process
        pid_t pid = fork();
        if (pid == -1)
        {
            // Error creating child process
            syslog(LOG_ERR,"ERROR: Can not create child process");
            socketserver_dtor(my_socket);
            exit(EXIT_FAILURE);
        }
        if (pid > 0)
        {
            // we are in the parent process so exit
            printf("The PID of child process is = %d\n", pid);
            exit(EXIT_SUCCESS);
        }
        // We are the child
        int sid = setsid();
        if (sid < 0)
        {
            syslog(LOG_ERR, "couldn't create session");
            socketserver_dtor(my_socket);
            exit(EXIT_FAILURE);
        }
        if ((chdir("/")) < 0)
        {
            syslog(LOG_ERR, "couldnt change process work dir");
            socketserver_dtor(my_socket);
            exit(EXIT_FAILURE);
        }
        dup2(open("/dev/null", O_RDWR), STDIN_FILENO);
        dup2(STDIN_FILENO, STDOUT_FILENO);
        dup2(STDOUT_FILENO, STDERR_FILENO);
    }
    // Listen LISTEN_BACKLOG connections;
    if(!socketserver_listen(my_socket))
    {
        syslog(LOG_ERR,"Error listening");
        socketserver_dtor(my_socket);
        exit(EXIT_FAILURE);
    }
    
    syslog(LOG_DEBUG,"Starting Listening...");
    // Open aesdsocketdata
    int data_fd = open(DATA_FILE, O_RDWR | O_CREAT | O_APPEND, 0644);
    if (data_fd < 0)
    {
            perror("Failed to open aesdsocketdata");
            socketserver_dtor(my_socket);
            exit(EXIT_FAILURE);
    }

    // Loop for connection
    while(waiting_cnn)
    {
        char ip_client[INET_ADDRSTRLEN];
        if(socketserver_connect(my_socket, ip_client) == false)
        {
            perror("accept");
            continue; //Continue listening
        }
        // Notify there is a connection from a client IP
        syslog(LOG_INFO,"Accepted connection from %s\n", ip_client);
        char buffer[BUFF_SIZE]={0};
        ssize_t bytes_received;
        // Receive Routine
        while((bytes_received=socketserver_recv(my_socket, buffer, sizeof(buffer)))> 0)
        {
            syslog(LOG_DEBUG,"Received %ld bytes\n", bytes_received);
            char* line_end;
            const char delim = '\n';
            ssize_t bytes_written;
            if((line_end = memchr(buffer, delim, bytes_received)) != NULL)
            {
                size_t bytes_to_write = line_end - buffer + 1; // include the newline character
                bytes_written = write (data_fd, buffer, bytes_to_write);
                if (bytes_written == -1 || bytes_written != bytes_to_write)
                {
                    syslog(LOG_ERR,"Error ocurred while writing your string");
                }
                sync();
                break;
            }
            syslog(LOG_DEBUG,"No more delimiters found");
            write(data_fd,buffer,bytes_received);
        }
        // Send Routine
        // move to start of the file
            if (lseek(data_fd, 0, SEEK_SET) == -1)
            {
                syslog(LOG_ERR, "Cannot move to the head of the file");
                perror("Failed to move to head of the file");
                socketserver_close_connection(my_socket);
                socketserver_dtor(my_socket);
                close(data_fd);
                remove(DATA_FILE);
                exit(EXIT_FAILURE);
            }
            ssize_t n;
            ssize_t bytes_send;
            memset(buffer,0,sizeof(buffer));
            while ((n = read(data_fd, buffer, sizeof(buffer))) > 0)
            {
                syslog(LOG_DEBUG, "Read %ld bytes", n);
                if ((bytes_send=socketserver_send(my_socket,buffer,n)) == -1)
                {
                    syslog(LOG_DEBUG, "Error when sending to client socket");
                    perror("Failed to send file content into the client socket");
                    socketserver_close_connection(my_socket);
                    socketserver_dtor(my_socket);
                    close(data_fd);
                    remove(DATA_FILE);
                    exit(EXIT_FAILURE);
                }
                syslog(LOG_DEBUG, "Sent %ld bytes", bytes_send);
            }
            if (n == 0)
            {
                syslog(LOG_DEBUG, "Reached EOF");
            }
            else if (n < 0)
            {
                perror("Error reading file");
                close (data_fd);
                socketserver_close_connection(my_socket);
                socketserver_dtor(my_socket);
                remove(DATA_FILE);
                exit(EXIT_FAILURE);
            }

        // Notify closing connection in the client IP
        syslog(LOG_INFO,"Closed connection from %s\n", ip_client);
        socketserver_close_connection(my_socket);
    }

    //Close socket fd and delete ADT
    socketserver_dtor(my_socket);
    // close fd for DATAFILE
    close(data_fd);
    remove(DATA_FILE);

    return 0;
}
