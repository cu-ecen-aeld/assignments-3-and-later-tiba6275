// Writer code for assignment 2
// Author: Tim Bailey

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>

#define NUM_PARAMS 2

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

int main(int argc, char *argv[]) {
    openlog("Logger", LOG_CONS | LOG_PID, LOG_USER);
    
    if (argc != (NUM_PARAMS + 1)) {
    	syslog(LOG_ERR, "Incorrect number of parameters, should be %d.", NUM_PARAMS);
        return(1);
    }

    write(argv[1], argv[2]);

    closelog();
}
