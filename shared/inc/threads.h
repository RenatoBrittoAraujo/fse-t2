#ifndef SHARED_THREADS_H
#define SHARED_THREADS_H 1

#include <stdlib.h>
#include <pthread.h>
#include <shared/inc/time.h>

#define STANDARD_THREAD_SIZE 10000

struct ThreadState
{
    pthread_t thread_id;
    int __created; // read-only for thread
    int __running; // read-only for thread caller

    void (*routine)(struct ThreadState *ts, void *args); // You have to type all void*
    void *(*optional_before)(struct ThreadState *ts, void *args);
    void *(*optional_after)(struct ThreadState *ts, void *args);

    void *args; // arguments sent to function as void*
    // but please notice that function receives a ThreadCallParams*

    size_t response_size;

    // keep in mind the thread is a 2 way relationship
    // if you need more, same rules apply for each relationship
};
typedef struct ThreadState ThreadState;

struct ThreadCallParams
{
    ThreadState *ts;
    void *param;
};
typedef struct ThreadCallParams ThreadCallParams;

// may set message size to -1 for standard frame size
ThreadState *create_thread_state(size_t message_size);

void start_thread(ThreadState *ts);
void *wait_thread_response(ThreadState *ts);
int force_thread_end(ThreadState *ts);
void *wait_thread_response_with_deadline(ThreadState *ts, time_mcs deadline);

#endif