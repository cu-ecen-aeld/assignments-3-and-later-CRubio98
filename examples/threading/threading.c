#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{

    // hint: use a cast like the one below to obtain thread arguments from your parameter
    struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    const int ms_to_us= 1000;

    DEBUG_LOG("Sleeping %d miliseconds", thread_func_args->wait_to_obtain_ms);
    usleep((thread_func_args->wait_to_obtain_ms)*ms_to_us);

    DEBUG_LOG("Locking Mutex");
    pthread_mutex_lock(thread_func_args->mutex);

    DEBUG_LOG("Sleeping %d miliseconds", thread_func_args->wait_to_release_ms);
    usleep((thread_func_args->wait_to_release_ms)*ms_to_us);

    DEBUG_LOG("Unlocked Mutex");
    pthread_mutex_unlock(thread_func_args->mutex);

    thread_func_args->thread_complete_success = true;

    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    // Allocate memory for thread_data*
    struct thread_data* data = (struct thread_data*) malloc(sizeof(struct thread_data));

    // Set struct fields
    data->wait_to_obtain_ms= wait_to_obtain_ms;
    data->wait_to_release_ms=wait_to_release_ms;
    data->mutex=mutex;
    data->thread_complete_success= false;



    //create thread
    int  ret= pthread_create (thread, NULL, threadfunc, (void*) data);
    if (ret) {
        errno = ret;
        perror("pthread_create");
        return false;
    }

    return true;
}

