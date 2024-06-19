#include "aesdsocket.h"

bool waiting_cnn = true;

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

    // Close the socket file 
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
ssize_t aesdsocket_recv(aesdsocket_t* this, char* buffer, size_t buff_size)
{
    return recv(this->client_sfd, buffer, buff_size-1,0);
}

ssize_t aesdsocket_send(aesdsocket_t* this, char* buffer, size_t buff_size)
{
   return send(this->client_sfd, buffer, buff_size, 0);
}

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

    // first, load up address structs with getaddrinfo():
    struct addrinfo hints;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;  // use IPv4
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;     // fill in my IP for me

    // make a socket:
    aesdsocket_t* my_socket=aesdsocket_ctor();
    if(my_socket == NULL)
    {
        syslog(LOG_ERR, "Socker Server instance could not be created");
        exit(EXIT_FAILURE);
    }
    if(!aesdsocket_setup_server(my_socket, hints))
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
            aesdsocket_dtor(my_socket);
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
            aesdsocket_dtor(my_socket);
            exit(EXIT_FAILURE);
        }
        if ((chdir("/")) < 0)
        {
            syslog(LOG_ERR, "couldnt change process work dir");
            aesdsocket_dtor(my_socket);
            exit(EXIT_FAILURE);
        }
        dup2(open("/dev/null", O_RDWR), STDIN_FILENO);
        dup2(STDIN_FILENO, STDOUT_FILENO);
        dup2(STDOUT_FILENO, STDERR_FILENO);
    }
    // Listen LISTEN_BACKLOG connections;
    if(!aesdsocket_listen(my_socket))
    {
        syslog(LOG_ERR,"Error listening");
        aesdsocket_dtor(my_socket);
        exit(EXIT_FAILURE);
    }
    
    syslog(LOG_DEBUG,"Starting Listening...");
    // Open aesdsocketdata
    int data_fd = open(DATA_FILE, O_RDWR | O_CREAT | O_APPEND, 0644);
    if (data_fd < 0)
    {
            perror("Failed to open aesdsocketdata");
            aesdsocket_dtor(my_socket);
            exit(EXIT_FAILURE);
    }

    // Loop for connection
    while(waiting_cnn)
    {
        char ip_client[INET_ADDRSTRLEN];
        if(aesdsocket_connect(my_socket, ip_client) == false)
        {
            perror("accept");
            continue; //Continue listening
        }
        // Notify there is a connection from a client IP
        syslog(LOG_INFO,"Accepted connection from %s\n", ip_client);
        char buffer[BUFF_SIZE]={0};
        ssize_t bytes_received;
        // Receive Routine
        while((bytes_received=aesdsocket_recv(my_socket, buffer, sizeof(buffer)))> 0)
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
                aesdsocket_dtor(my_socket);
                remove(DATA_FILE);
                exit(EXIT_FAILURE);
            }
            ssize_t n;
            ssize_t bytes_send;
            memset(buffer,0,sizeof(buffer));
            while ((n = read(data_fd, buffer, sizeof(buffer))) > 0)
            {
                syslog(LOG_DEBUG, "Read %ld bytes", n);
                if ((bytes_send=aesdsocket_send(my_socket,buffer,n)) == -1)
                {
                    syslog(LOG_DEBUG, "Error when sending to client socket");
                    perror("Failed to send file content into the client socket");
                    aesdsocket_dtor(my_socket);
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
                aesdsocket_dtor(my_socket);
                remove(DATA_FILE);
                exit(EXIT_FAILURE);
            }

        // Notify closing connection in the client IP
        syslog(LOG_INFO,"Closed connection from %s\n", ip_client);
        aesdsocket_dtor(my_socket);
    }

    //Close socket fd
    aesdsocket_dtor(my_socket);
    // close fd for DATAFILE
    close(data_fd);
    remove(DATA_FILE);

    return 0;
}
