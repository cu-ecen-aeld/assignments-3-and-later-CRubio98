#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#define LISTEN_BACKLOG  1
#define BUFF_SIZE       60
#define DATA_FILE       "/var/tmp/aesdsocketdata"

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
    char client_ip[INET_ADDRSTRLEN];
    int socketfd, operative_sfd, opt;
    socklen_t peer_addr_size;
    struct sockaddr_storage peer_addr;
    struct addrinfo hints;
    struct addrinfo *res;

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

    // first, load up address structs with getaddrinfo():
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;  // use IPv4
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;     // fill in my IP for me

    getaddrinfo(NULL, "9000", &hints, &res);

    // make a socket:
    socketfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if(socketfd == -1)
    {
        syslog(LOG_ERR,"Error opening socket server fd");
        exit(EXIT_FAILURE);
    }
    // Reuse Addres
    const int tmp = 1;
    if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &tmp, sizeof(int)) == -1)
    {
        perror("Failed to set SO_REUSEADDR");
        close(socketfd);
        exit(EXIT_FAILURE);
    }
    // bind it to the port we passed in to getaddrinfo():
    if(bind(socketfd, res->ai_addr, res->ai_addrlen) == -1)
    {
        syslog(LOG_ERR,"Error binding desired port");
        close(socketfd); 
        exit(EXIT_FAILURE);
    }
    // Free res addr info
    freeaddrinfo(res);

    if (daemon)
    {
        // Create Process
        pid_t pid = fork();
        if (pid == -1)
        {
            // Error creating child process
            syslog(LOG_ERR,"ERROR: Can not create child process");
            close(socketfd);
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
            close(socketfd);
            exit(EXIT_FAILURE);
        }
        if ((chdir("/")) < 0)
        {
            syslog(LOG_ERR, "couldnt change process work dir");
            close(socketfd);
            exit(EXIT_FAILURE);
        }
    }
    // Listen LISTEN_BACKLOG connections;
    if(listen(socketfd, LISTEN_BACKLOG) == -1)
    {
        syslog(LOG_ERR,"Error listening");
        close(socketfd); 
        exit(EXIT_FAILURE);
    }
    
    syslog(LOG_DEBUG,"Starting Listening...");
    // Open aesdsocketdata
    int data_fd = open(DATA_FILE, O_RDWR | O_CREAT | O_APPEND, 0644);
    if (data_fd < 0)
    {
            perror("Failed to open aesdsocketdata");
            close(socketfd); 
            exit(EXIT_FAILURE);
    }

    // Loop for connection
    while(waiting_cnn)
    {
        peer_addr_size=sizeof peer_addr;
        operative_sfd= accept(socketfd,(struct sockaddr *) &peer_addr, &peer_addr_size);
        if(operative_sfd == -1)
        {
            perror("accept");
            continue;
        }

        // Get client IP
        inet_ntop(peer_addr.ss_family,
            &((struct sockaddr_in*)&peer_addr)->sin_addr,
            client_ip, sizeof client_ip);

        // Notify there is a connection from a client IP
        syslog(LOG_INFO,"Accepted connection from %s\n", client_ip);

        char buffer[BUFF_SIZE]={0};
        ssize_t bytes_received;

        // Receive Routine
        while((bytes_received=recv(operative_sfd, buffer, sizeof(buffer)-1,0))> 0)
        {
            syslog(LOG_DEBUG,"Received %ld bytes\n", bytes_received);
            char* line_start= (char*)malloc(bytes_received);
            if(line_start==NULL){
                syslog(LOG_ERR,"Error. Allocation was unsuccessful\n");
                exit(EXIT_FAILURE);
            }
            memcpy(line_start,buffer,bytes_received);
            char* line_end;
            const char delim = '\n';
            ssize_t bytes_written;
            if((line_end = memchr(line_start, delim, bytes_received)) != NULL)
            {
                size_t bytes_to_write = line_end - line_start + 1; // include the newline character
                bytes_written = write (data_fd, line_start, bytes_to_write);
                if (bytes_written == -1 || bytes_written != bytes_to_write)
                {
                    syslog(LOG_ERR,"Error ocurred while writing your string");
                }
                sync();
                free(line_start);
                break;
            }
            syslog(LOG_DEBUG,"No more delimiters found");
            write(data_fd,line_start,bytes_received);
            free(line_start);
        }
        // Send Routine
        // move to start of the file
            if (lseek(data_fd, 0, SEEK_SET) == -1)
            {
                syslog(LOG_ERR, "Cannot move to the head of the file");
                perror("Failed to move to head of the file");
                close(data_fd);
                remove(DATA_FILE);
                exit(EXIT_FAILURE);
            }
            ssize_t n;
            ssize_t bytes_send;
            memset(buffer,0,sizeof(buffer));
            while ((n = read(data_fd, buffer, sizeof(buffer)-1)) > 0)
            {
                syslog(LOG_DEBUG, "Read %ld bytes", n);
                syslog(LOG_DEBUG, "Read %ld bytes results in string: %s", n, buffer);
                if ((bytes_send=send(operative_sfd, buffer, n, 0)) == -1)
                {
                    syslog(LOG_DEBUG, "Error when sending to client socket");
                    perror("Failed to send file content into the client socket");
                    close(operative_sfd);
                    close(data_fd);
                    remove(DATA_FILE);
                    exit(EXIT_FAILURE);
                }
                syslog(LOG_DEBUG, "Sent %ld bytes", bytes_send);
                memset(buffer,0,sizeof(buffer));
            }
            if (n == 0)
            {
                syslog(LOG_DEBUG, "Reached EOF");
            }
            else if (n < 0)
            {
                perror("Error reading file");
                close (data_fd);
                close(socketfd);
                remove(DATA_FILE);
                exit(EXIT_FAILURE);
            }

        // Notify closing connection in the client IP
        syslog(LOG_INFO,"Closed connection from %s\n", client_ip);
        close(operative_sfd);
    }

    //Close socket fd
    close(socketfd);
    // close fd for DATAFILE
    close(data_fd);
    remove(DATA_FILE);

    return 0;
}