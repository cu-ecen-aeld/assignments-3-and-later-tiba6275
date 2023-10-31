/*
 * A basic socket server program.
 *
 * The server writes data received from clients to a file and then
 * reads it back to the client upon receiving a newline (`\n`).
 * The server can also be run as a daemon by passing the `-d` option.
 *
 * Author: Tim Bailey, tiba6275@colorado.edu
 * For ECEN5713, Assignment 6.
 
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
#include <pthread.h>
#include <time.h>

#define SERVER_PORT 9000
#define BUFFER_SIZE 1024

#define USE_AESD_CHAR_DEVICE 1

#if USE_AESD_CHAR_DEVICE == 1
    #define fdir "/dev/aesdchar"
#else
    #define fdir "/var/tmp/aesdsocketdata"
#endif

int sockfd;
pthread_mutex_t mutex;
typedef struct thread_node {
    pthread_t tid;
    struct thread_node *next;
} node_t;

node_t *head = NULL;
pthread_mutex_t thread_list_mutex = PTHREAD_MUTEX_INITIALIZER;

// Signal handler to catch SIGINT and SIGTERM for graceful shutdown.
void signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        close(sockfd);
        #if USE_AESD_CHAR_DEVICE != 1
            remove(fdir);
        #endif
        syslog(LOG_INFO, "Caught signal, exiting");
        closelog();
        exit(0);
    } else if (signal == SIGALRM) {
        #if USE_AESD_CHAR_DEVICE != 1
            char timestamp_str[100];
            time_t current_time = time(NULL);
            struct tm *local_time = localtime(&current_time);
            strftime(timestamp_str, sizeof(timestamp_str), "timestamp:%a, %d %b %Y %H:%M:%S %z\n", local_time);
            
            pthread_mutex_lock(&mutex);
            FILE *fp = fopen(fdir, "a");
            if (fp == NULL) {
                perror("fopen: Failed opening file.");
            } else {
                fprintf(fp, "%s", timestamp_str);
                fclose(fp);
            }
            pthread_mutex_unlock(&mutex);
        #endif
    }
}

void *connection_handler(void *socket_desc) {
    int connfd = *(int *)socket_desc;
    char *buffer = calloc(BUFFER_SIZE, sizeof(char));
    
    while (recv(connfd, buffer, BUFFER_SIZE, 0) > 0) {
        pthread_mutex_lock(&mutex);
        FILE *fp = fopen(fdir, "a");
        if (fp == NULL) {
            perror("fopen: Failed opening file.");
            pthread_mutex_unlock(&mutex);
            free(buffer);
            close(connfd);
            return NULL;
        } else {
            fprintf(fp, "%s", buffer);
            fclose(fp);
        }
        pthread_mutex_unlock(&mutex);
        
        if (strchr(buffer, '\n') != NULL) {
            FILE *fp_out = fopen(fdir, "r");
            if (fp_out == NULL) {
                perror("fpopen: Failed opening file.");
                free(buffer);
                close(connfd);
                return NULL;
            } else {
                uint32_t bytes_read = 0;
                while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, fp_out)) > 0) {
                    send(connfd, buffer, bytes_read, 0);
                }
                fclose(fp_out);
            }
        }
        memset(buffer, 0, BUFFER_SIZE);
    }
    free(buffer);
    close(connfd);
    return NULL;
}

void add_thread(pthread_t tid) {
    pthread_mutex_lock(&thread_list_mutex);

    node_t *new_node = (node_t *)malloc(sizeof(node_t));
    new_node->tid = tid;
    new_node->next = head;
    head = new_node;

    pthread_mutex_unlock(&thread_list_mutex);
}

void cleanup_threads() {
    pthread_mutex_lock(&thread_list_mutex);

    node_t *current = head;
    while (current) {
        pthread_join(current->tid, NULL);
        node_t *to_free = current;
        current = current->next;
        free(to_free);
    }

    pthread_mutex_unlock(&thread_list_mutex);
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
    
    pthread_mutex_init(&mutex, NULL);
    
    // Initialize syslog for logging.
    openlog("aesdsocket", LOG_CONS | LOG_PID, LOG_USER);
    
    // Register the signal handler.
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGALRM, signal_handler);

    // Timer setup
    timer_t timer_id;
    struct sigevent sev;
    memset(&sev, 0, sizeof(struct sigevent));
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGALRM;
    timer_create(CLOCK_REALTIME, &sev, &timer_id);

    struct itimerspec timer_spec;
    memset(&timer_spec, 0, sizeof(struct itimerspec));
    timer_spec.it_interval.tv_sec = 10;
    timer_spec.it_value.tv_sec = 10;
    timer_settime(timer_id, 0, &timer_spec, NULL);

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
        int *new_sock = malloc(sizeof(int));
        int connfd = accept(sockfd, (struct sockaddr*)&client, &client_len);
        if (connfd < 0) {
            perror("accept: Failed connecting to client.");
            free(new_sock);
            continue;
        }

        // Log the client IP address.
        struct sockaddr_in addr;
        socklen_t addr_len = sizeof(addr);
        getpeername(connfd, (struct sockaddr *)&addr, &addr_len);
        char *client_ip = inet_ntoa(addr.sin_addr);
        syslog(LOG_INFO, "Accepted connection from %s", client_ip);
        
        *new_sock = connfd;

        pthread_t new_thread;
        if (pthread_create(&new_thread, NULL, connection_handler, new_sock) != 0) {
            perror("pthread_create failed");
            close(connfd);
            free(new_sock);
            continue;
        }

        add_thread(new_thread);

    }
        
    node_t *current = head;
    while (current) {
        pthread_join(current->tid, NULL);
        node_t *to_free = current;
        current = current->next;
        free(to_free);
    }
    
    // Cleanup
    pthread_mutex_destroy(&mutex);
    timer_delete(timer_id);
    close(sockfd);
    #if USE_AESD_CHAR_DEVICE != 1
        remove(fdir);
    #endif
    closelog();
    exit(0);
    return 0;
}
