#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>

enum
{
    WRITEFILE   =  1,
    WRITESTR    =  2
};

int main(int argc, char* argv[])
{
    openlog("Assigment2Log", LOG_PID, LOG_USER);

    // executable counts as argument
    if(argc < 2)
    {
        syslog(LOG_ERR,"You must need 2 arguments");
        return 1;
    }

    size_t string_size;
    ssize_t bytes_wrote;
    int fd;

    // Try to create and open the file to write
    fd= creat(argv[WRITEFILE],S_IRWXU | S_IRGRP | S_IROTH);
    if (fd == -1)
    {
        syslog(LOG_ERR,"Error FILE: %s could not be created", argv[WRITEFILE]);
        return 1;
    }

    // Writes the string
    string_size = strlen (argv[WRITESTR]);
    bytes_wrote = write (fd, argv[WRITESTR], string_size);

    syslog(LOG_DEBUG,"Writing %s in %s file", argv[WRITESTR], argv[WRITEFILE]);

    if (bytes_wrote == -1 || bytes_wrote != string_size)
    {
        syslog(LOG_ERR,"Error ocurred while writing your string");
    }

    // Close the file
    if (close (fd) == -1)
        syslog(LOG_ERR,"Error ocurred while closing the file");

    return 0;
}