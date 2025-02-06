/***********************************************************************
* @file  threading.c
* @version 0
* @brief  Implementation of threading file
*
* @author Iyona Lynn Noronha, iyonalynn.noronha@Colorado.edu
*
* @institution University of Colorado Boulder (UCB)
* @course   ECEN 5713 - Advanced Embedded Software Development
* @instructor Dan Walkes
*
* Revision history:
*   0 Initial release.
*
*/

#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param) {
    struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    
    if (!thread_func_args) {
        ERROR_LOG("Invalid thread data");
        return NULL;
    }
    
    // Sleep before obtaining mutex
    usleep(thread_func_args->wait_to_obtain_ms * 1000);
    
    // Obtain mutex
    if (pthread_mutex_lock(thread_func_args->mutex) != 0) {
        ERROR_LOG("Failed to lock mutex");
        thread_func_args->thread_complete_success = false;
        return thread_func_args;
    }
    
    // Sleep while holding mutex
    usleep(thread_func_args->wait_to_release_ms * 1000);
    
    // Release mutex
    if (pthread_mutex_unlock(thread_func_args->mutex) != 0) {
        ERROR_LOG("Failed to unlock mutex");
        thread_func_args->thread_complete_success = false;
        return thread_func_args;
    }
    
    thread_func_args->thread_complete_success = true;
    return thread_func_args;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex, int wait_to_obtain_ms, int wait_to_release_ms) {
    if (!thread || !mutex || wait_to_obtain_ms < 0 || wait_to_release_ms < 0) {
        ERROR_LOG("Invalid arguments passed to start_thread_obtaining_mutex");
        return false;
    }
    
    struct thread_data *thread_data = malloc(sizeof(struct thread_data));
    if (!thread_data) {
        ERROR_LOG("Failed to allocate memory for thread data");
        return false;
    }
    
    thread_data->mutex = mutex;
    thread_data->wait_to_obtain_ms = wait_to_obtain_ms;
    thread_data->wait_to_release_ms = wait_to_release_ms;
    thread_data->thread_complete_success = false;
    
    if (pthread_create(thread, NULL, threadfunc, thread_data) != 0) {
        ERROR_LOG("Failed to create thread");
        free(thread_data);
        return false;
    }
    
    return true;
}
