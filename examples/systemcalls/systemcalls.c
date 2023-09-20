/* Command execution code for CU Boulder ECEN 5713, assignment 3.
 * 
 * Shell provided by CU Boulder, ECEN 5713.
 * Modified by: Tim Bailey, tiba6275@colorado.edu
 * Date: 9/17/2023
 *
 * Referenced Linux System Programming by Robert Love, p161, as well as
 * https://stackoverflow.com/questions/13784269/redirection-inside-call
 * -to-execvp-not-working/13784315#13784315
 *
 * For educational use only.
 */

#include <stdlib.h>
#include <unistd.h>
#include <wait.h>
#include <fcntl.h>
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
    if (system(cmd) != 0) {
        perror("Command failed.");
    	return false;
    }
    return true;
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
*
* Referenced Linux System Programming by Robert Love, p. 161.
*/

bool do_exec(int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    int status;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;

    fflush(stdout);
    pid_t pid = fork();
    if (pid == 0) {
        execv(command[0], command);
        perror("Command execution failed.");
        va_end(args);
        exit(-1);
    }
    
    waitpid(pid, &status, 0);
    if (WIFEXITED(status)) {
        if (WEXITSTATUS(status) == 0) {
            printf("Child execution successful.\n");
            va_end(args);
            return true;
        } else {
            perror("Child execution failed.");
            va_end(args);
            return false;
        }
    } else {
        perror("Child exited abnormally.");
        va_end(args);
        return false;
    }
    va_end(args);
    return false;
}

/**
* @param outputfile - The full path to the file to write with command output.
*   This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*
* Referenced https://stackoverflow.com/questions/13784269/redirection-inside
* -call-to-execvp-not-working/13784315#13784315
*/
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    int status;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;

    int fd = creat(outputfile, 0644);
    if (fd == -1) {
        perror("File creation failed.");
        va_end(args);
        return false;
    } 
    
    fflush(stdout);
    pid_t pid = fork();
    if (pid == 0) {
        if (dup2(fd, 1) < 0) {
            perror("dup2 failure.");
            abort();
            close(fd);
            return false;
        }
        close(fd);
        execv(command[0], command);
        perror("Command execution failed.");
        va_end(args);
        exit(-1);
    }
    
    waitpid(pid, &status, 0);
    if (WIFEXITED(status)) {
        if (WEXITSTATUS(status) == 0) {
            printf("Child execution successful.\n");
            close(fd);
            va_end(args);
            return true;
        } else {
            perror("Child execution failed.");
            close(fd);
            va_end(args);
            return false;
        }
    } else {
        perror("Child exited abnormally.");
        close(fd);
        va_end(args);
        return false;
    }
    close(fd);
    va_end(args);
    return false;
}
