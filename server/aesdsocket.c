#include "aesdsocket.h"

bool waiting_cnn = true;

aesdsocket_t* aesdsocket_ctor(struct addrinfo* hints)
{
    aesdsocket_t* new_aesdsocket = (aesdsocket_t*) malloc(sizeof(aesdsocket_t));
    if(new_aesdsocket == NULL)
    {
        return NULL;
    }
    new_aesdsocket->res= (struct addrinfo*) malloc(sizeof(struct addrinfo));
    if(new_aesdsocket->res == NULL)
    {
        free(new_aesdsocket);
        return NULL;
    }

    // Create dynamic buffer
    new_aesdsocket->buffer = malloc(sizeof INIT_BUFF_SIZE);
    if(new_aesdsocket->res == NULL)
    {
        free(new_aesdsocket);
        free(new_aesdsocket->res);
        return NULL;
    }
    new_aesdsocket->data_size=INIT_BUFF_SIZE;
    // Get size of struct
    new_aesdsocket->peer_addr_size=sizeof new_aesdsocket->peer_addr;

    // Set Logs
    openlog("socket_adt", LOG_PID, LOG_USER);
    // We will get addrs info from hints argument
    getaddrinfo(NULL, (const char*)DEFAULT_PORT, hints, &new_aesdsocket->res);

    // Get a new socket file descriptor
    new_aesdsocket->socketfd = socket(new_aesdsocket->res->ai_family,
                                      new_aesdsocket->res->ai_socktype,
                                      new_aesdsocket->res->ai_protocol);

    if(new_aesdsocket->socketfd == -1)
    {
        syslog(LOG_ERR,"Error opening socket server fd");
        return NULL;
    }

    // Reuse Address
    const int tmp = 1;
    if (setsockopt(new_aesdsocket->socketfd, SOL_SOCKET, SO_REUSEADDR, &tmp, sizeof(int)) == -1)
    {
        syslog(LOG_ERR,"Failed to set SO_REUSEADDR");
        close(new_aesdsocket->socketfd);
        return NULL;
    }

    // bind it to the port we passed in to getaddrinfo():
    if(bind(new_aesdsocket->socketfd,new_aesdsocket->res->ai_addr,
            new_aesdsocket->res->ai_addrlen) == -1)
    {
        syslog(LOG_ERR,"Error binding desired port");
        close(new_aesdsocket->socketfd); 
        return NULL;
    }
    // Free res addr info
    freeaddrinfo(new_aesdsocket->res);

    return new_aesdsocket;
}
void aesdsocket_dtor(aesdsocket_t* this)
{
    if(!this){return;}

    // Close the socket file 
    close(this->socketfd);
    if(this->buffer){free (this->buffer);}
    if(this->res){free (this->res);}
    free(this);
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
bool aesdsocket_recv(aesdsocket_t* this)
{
    int bytes_received=recv(this->client_sfd, this->buffer, INIT_BUFF_SIZE-1,0);
    if(!(bytes_received > 0)){return false;}

    if (INIT_BUFF_SIZE != bytes_received)
    {
        this->buffer=realloc(this->buffer, bytes_received);
        if( this->buffer == NULL)
        {
            return false;
        }
        this->data_size=bytes_received;
    }
    return true;
}

int aesdsocket_send(aesdsocket_t* this)
{
   int bytes_send=send(this->client_sfd, this->buffer, this->data_size, 0);
   return bytes_send;
}

void aesdsocket_get_buffer(aesdsocket_t* this,char* output_buffer, size_t* size)
{
    output_buffer= this->buffer;
    *size= this->data_size;
}

void aesdsocket_set_buffer(aesdsocket_t* this,char* buffer, size_t size)
{
    free(this->buffer);
    this->buffer= malloc(size);

    memcpy(this->buffer,buffer,size);
    this->data_size= size;
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
    aesdsocket_t* my_socket=aesdsocket_ctor(&hints);
    if(my_socket == NULL)
    {
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

        // Receive Routine
        while(aesdsocket_recv(my_socket))
        {
            char* buffer=NULL;
            size_t buffer_size;
            aesdsocket_get_buffer(my_socket, buffer, &buffer_size);
            syslog(LOG_DEBUG,"Received %ld bytes\n", buffer_size);
            char* line_end;
            const char delim = '\n';
            ssize_t bytes_written;
            if((line_end = memchr(buffer, delim, buffer_size)) != NULL)
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
            write(data_fd,buffer,buffer_size);
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
            char buffer[INIT_BUFF_SIZE];
            memset(buffer,0,sizeof(buffer));
            while ((n = read(data_fd, buffer, sizeof(buffer))) > 0)
            {
                syslog(LOG_DEBUG, "Read %ld bytes", n);
                aesdsocket_set_buffer(my_socket, buffer, sizeof(buffer));
                if ((bytes_send=aesdsocket_send(my_socket)) == -1)
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