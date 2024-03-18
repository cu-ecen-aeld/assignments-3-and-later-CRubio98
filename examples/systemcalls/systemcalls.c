#include "systemcalls.h"

/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
*/
bool do_system(const char *cmd)
{
    int status = system(cmd);

    if (status != -1)
    {
        int exitstatus=WEXITSTATUS(status);
        if(WIFEXITED(status) && exitstatus==0)
        {
            printf("command executed with no errors\n");
            return true;
        }
        printf("command failed with %d error\n", exitstatus);
    }
    else
    {
        printf("system call could not be performed\n");
    }

    return false;
}

/**
* @param count -The numbers of variables passed to the function. The variables are command to execute.
*   followed by arguments to pass to the command
*   Since exec() does not perform path expansion, the command to execute needs
*   to be an absolute path.
* @param ... - A list of 1 or more arguments after the @param count argument.
*   The first is always the full path to the command to execute with execv()
*   The remaining arguments are a list of arguments to pass to the command in execv()
* @return true if the command @param ... with arguments @param arguments were executed successfully
*   using the execv() call, false if an error occurred, either in invocation of the
*   fork, waitpid, or execv() command, or if a non-zero return value was returned
*   by the command issued in @param arguments with the specified arguments.
*/

bool do_exec(int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    va_end(args);
    const int path = 0;

    // Validate absolute path:
    if(*command[path] != '/')
    {
        printf("ERROR: Path must be absolute\n");
        return false;
    }

    int status;
    pid_t pid=fork();
    if (pid == -1)
    {
        // Error creating child process
        printf("ERROR: Can not create child process\n");
        return false;
    }
    // We are in the child process
    else if (pid == 0)
    {
        execv (command[path], command);
        // If execv returns means the command failed
        exit (-1);
    }
    // Wait for child termination
    if (waitpid (pid, &status, 0) == -1)
        return false;

    return WIFEXITED(status) && (WEXITSTATUS(status) == 0);
}

/**
* @param outputfile - The full path to the file to write with command output.
*   This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*/
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    const int path = 0;

    va_end(args);

    // Validate absolute path:
    if(*command[path] != '/')
    {
        printf("ERROR: Path must be absolute\n");
        return false;
    }

    //Open the redirection file
    int fd = open(outputfile, O_WRONLY|O_TRUNC|O_CREAT, 0644);
    if (fd < 0)
    {
        perror("open");
        return false;
    }

    int status;
    pid_t pid=fork();
    if (pid == -1)
    {
        // Error creating child process
        printf("ERROR: Can not create child process\n");
        close(fd);
        return false;
    }
    // We are in the child process
    else if (pid == 0)
    {
        // Redirect stdout to our file descriptor aka our outputfile
        if (dup2(fd, STDOUT_FILENO) < 0)
        {
            perror("dup2");
            return false;
        }
        close(fd);

        // now we can perform execv as normally
        execv (command[path], command);
        // If execv returns means the command failed
        exit (-1);
    }
    // Wait for child termination
    if (waitpid (pid, &status, 0) == -1)
        return false;

    return WIFEXITED(status) && (WEXITSTATUS(status) == 0);

    return true;
}
