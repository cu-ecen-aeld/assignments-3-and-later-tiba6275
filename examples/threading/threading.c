/* Thread handling for ECEN 5713, A4 Part 1.
 * Modified by Tim Bailey, tiba6275@colorado.edu
 * Referenced ECEN 5713 lecture slides, which referenced
 * Linux System Programmer.
 * For educational use only.
 */

#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

/* Waits, obtains, waits, and releases mutex.
 * @param thread_param pointer to arguments for the thread. Struct information
 * in threading.h.
 */
void* threadfunc(void* thread_param)
{

    // Thread arguments from the parameter
    struct thread_data* thread_func_args = thread_param;
    
    // Wait
    usleep(thread_func_args->wait_to_obtain_ms * 1000);

    // Obtain mutex
    int lock = pthread_mutex_lock(thread_func_args->mutex);
    if (lock != 0) {
        thread_func_args->thread_complete_success = false;
        ERROR_LOG("pthread_mutex_lock failed with %d\n", lock);
        return thread_param;
    }

    // Wait
    usleep(thread_func_args->wait_to_release_ms * 1000);

    // Release mutex
    int unlock = pthread_mutex_unlock(thread_func_args->mutex);
    if (unlock != 0) {
        thread_func_args->thread_complete_success = false;
        ERROR_LOG("pthread_mutex_unlock failed with %d\n", unlock);
        return thread_param;
    }

    thread_func_args->thread_complete_success = true;
    return thread_param;
}

    /* Allocates memory for thread_data and sets struct variables, creates thread. Frees allocated memory if
     * thread creation fails, otherwise should be freed when thread is joined elsewhere. 
     * @param *thread pointer to thread
     * @param *mutex pointer to thread mutex
     * @param wait_to_obtain_ms time to wait to unlock in ms
     * @param wait_to_release_ms time to wait to release mutex in ms
     * @return false if error is encountered, otherwise true
     */
bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    // Allocate memory for thread data
    struct thread_data* data = malloc(sizeof(struct thread_data));

    if (data == NULL) {
        ERROR_LOG("Malloc failed to allocate.");
        return false;
    }

    // Setup mutex and wait arguments
    data->mutex = mutex;
    data->wait_to_obtain_ms = wait_to_obtain_ms;
    data->wait_to_release_ms = wait_to_release_ms;
    
    // Create thread and pass thread_data to created thread
    int create = pthread_create(thread, NULL, threadfunc, data);
    if (create != 0) {
        ERROR_LOG("pthread_create failed with %d\n", create);
        free(data);
        return false;
    }
    return true;
}

