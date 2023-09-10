/* Writer code for CU Boulder ECEN 5713, assignment 2.
 * Author: Tim Bailey, tiba6275@colorado.edu
 * Date: 9/9/2023
 *
 * For educational use only.
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>

#define NUM_PARAMS 2

/* Writes an input string to a file. Assumes directory is created by caller.
 * Exits and logs on file creation error.
 * Params: 
 *   writefile - string location of file
 *   writestr - string to write to file
 * Returns:
 *   exits with 1 on file creation error
 */
void write(char *writefile, char *writestr) {
    FILE *fp;
    fp = fopen(writefile, "w");
    if (fp != NULL) {
    	fprintf(fp, "%s", writestr);
    	fclose(fp);
    } else {
    	syslog(LOG_ERR, "Error creating file.");
    	exit(1);
    }
    printf("%s written to %s.\n", writestr, writefile);
    syslog(LOG_DEBUG, "%s written to %s.", writestr, writefile);
}

/* Writes arg2 to location arg1. Syslog debug and errors to LOG_USER. */
int main(int argc, char *argv[]) {
    openlog("AESD A2", LOG_CONS | LOG_PID, LOG_USER);
    
    if (argc != (NUM_PARAMS + 1)) {
    	syslog(LOG_ERR, "Incorrect number of parameters, should be %d.", NUM_PARAMS);
        return(1);
    }

    write(argv[1], argv[2]);

    closelog();
}
