/*
 * A basic socket server program.
 *
 * The server writes data received from clients to a file and then
 * reads it back to the client upon receiving a newline (`\n`).
 * The server can also be run as a daemon by passing the `-d` option.
 *
 * Author: Tim Bailey, tiba6275@colorado.edu
 * For ECEN5713, Assignment 5.
 
 * Reference: http://www.cs.cmu.edu/~prs/15-441-F10/lectures/r01-sockets.pdf, 
 * additional guidance for templates, some logic, code cleanup, & comments
 * from ChatGPT, additional guidance for some structures from class lectures.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <signal.h>
#include <syslog.h>

#define SERVER_PORT 9000
#define BUFFER_SIZE 1024

int sockfd;

// Signal handler to catch SIGINT and SIGTERM for graceful shutdown.
void signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        close(sockfd);
        remove("/var/tmp/aesdsocketdata");
        syslog(LOG_INFO, "Caught signal, exiting");
        closelog();
        exit(0);
    }
}

int main(int argc, char *argv[]) {
    // Logic to run the process as a daemon if `-d` argument is passed.
    pid_t pid, sid;
    if ((argc > 1) && (strcmp(argv[1], "-d") == 0)) {
        pid = fork();
        if (pid < 0) {
            perror("fork: Failed to create child.");
            exit(-1);
        } else if (pid > 0) {
            exit(0);
        }
        
        sid = setsid();
        if (sid < 0) {
            perror("setsid: Failed to acquire sid.");
            exit(-1);
        }
        
        if (chdir("/") < 0) {
            perror("chdir: Failed to change directory.");
            exit(-1);
        }
    }
    
    // Initialize syslog for logging.
    openlog("aesdsocket", LOG_CONS | LOG_PID, LOG_USER);
    
    // Register the signal handler.
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Create the socket.
    sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket: Failed to create socket.");
        exit(-1);
    }

    // Enable address reuse.
    const int enable = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        perror("setsockopt: (SO_REUSEADDR) failed.");
        close(sockfd);
        exit(-1);
    }

    // Bind socket to a specified port.
    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT);
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(sockfd, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("bind: Failed to bind name to socket.");
        close(sockfd);
        exit(-1);
    }

    // Listen for client connections.
    if (listen(sockfd, 5) < 0) {
        perror("listen: Failed to listen for connections.");
        close(sockfd);
        exit(-1);
    }

    // Main loop to accept client connections.
    while (1) {
        struct sockaddr_in client;
        socklen_t client_len = sizeof(client);
        int connfd = accept(sockfd, (struct sockaddr*)&client, &client_len);
        if (connfd < 0) {
            perror("accept: Failed connecting to client.");
            continue;
        }

        // Log the client IP address.
        struct sockaddr_in addr;
        socklen_t addr_len = sizeof(addr);
        getpeername(connfd, (struct sockaddr *)&addr, &addr_len);
        char *client_ip = inet_ntoa(addr.sin_addr);
        syslog(LOG_INFO, "Accepted connection from %s", client_ip);
        
        // Buffer to store data received from client.
        char* buffer = calloc(BUFFER_SIZE, sizeof(char));

        // Receive data from the client.
        while (recv(connfd, buffer, BUFFER_SIZE, 0) > 0) {
            // Write data to file.
            FILE *fp = fopen("/var/tmp/aesdsocketdata", "a");
            if (fp == NULL) {
                perror("fopen: Failed opening file.");
                free(buffer);
                close(connfd);
                exit(-1);
            } else {
                fprintf(fp, "%s", buffer);
                fclose(fp);
            }

            // If newline is detected, read the file and send back to the client.
            if (strchr(buffer, '\n') != NULL) {
                FILE *fp_out = fopen("/var/tmp/aesdsocketdata", "r");
                if (fp_out == NULL) {
                    perror("fopen: Failed opening file.");
                    free(buffer);
                    close(connfd);
                    exit(-1);
                } else {
                    uint32_t bytes_read = 0;
                    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, fp_out)) > 0) {
                        send(connfd, buffer, bytes_read, 0);
                    }                
                }
                fclose(fp_out);
            }
            memset(buffer, 0, BUFFER_SIZE);
        }
        free(buffer);
        close(connfd);
        syslog(LOG_INFO, "Closed connection from %s", client_ip);
    }
    
    close(sockfd);
    remove("/var/tmp/aesdsocketdata");
    closelog();
    exit(0);
    return 0;
}
