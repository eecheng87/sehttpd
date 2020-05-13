#include "threadpool.h"

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
/* entry in work queue */
struct threadpool_task_t {
    void (*function)(void *);
    void *argument;
};

/* pack real thread `pthread_t` */
/*
    `pop_index` and `push_index` are
    both gaurantee contention-free.
*/
struct threadpool_thread_t {
    pthread_t thread;         /* real POSIX thread              */
    threadpool_task_t *queue; /* work queue belong each thread  */
    int size;                 /* size of queue                  */
    int pop_index;            /* next index for pop             */
    int push_index;           /* next index for push            */
    int task_count;           /* shared var between enq and deq */
};

struct threadpool_t {
    threadpool_thread_t *threads; /* threads set ( array )     */
    int thread_count;
    int shutdown; /* flag                      */
};

threadpool_t *threadpool_create(int thread_count, int queue_size)
{
    threadpool_t *pool;

    if (thread_count <= 0 || thread_count > MAX_THREADS || queue_size <= 0 ||
        queue_size > MAX_QUEUE) {
        return NULL;
    }

    if ((pool = (threadpool_t *) malloc(sizeof(threadpool_t))) == NULL) {
        goto err;
    }

    /* Initialize */
    pool->thread_count = 0;
    pool->shutdown = 0;

    /* Allocate thread and task queue */
    pool->threads = (threadpool_thread_t *) malloc(sizeof(threadpool_thread_t) *
                                                   thread_count);

    if (!pool->threads) {
        goto err;
    }

    /* Start worker threads */
    for (int i = 0; i < thread_count; i++) {
        if (pthread_create(&(pool->threads[i].thread), NULL, threadpool_thread,
                           (void *) (pool->threads + i)) != 0) {
            threadpool_destroy(pool);
            return NULL;
        }
        /* Initialize thread_t's member */
        pool->threads[i].queue = (threadpool_task_t *) malloc(
            sizeof(threadpool_task_t) * queue_size);
        pool->thread_count++;
        pool->threads[i].pop_index = pool->threads[i].push_index = 0;
        pool->threads[i].task_count = 0;
        pool->threads[i].size = queue_size;
    }

    return pool;

err:
    if (pool) {
        threadpool_free(pool);
    }
    return NULL;
}

int threadpool_destroy(threadpool_t *pool)
{
    int i, err = 0;

    if (pool == NULL) {
        return threadpool_invalid;
    }
    do {
        /* Already shutting down */
        if (pool->shutdown) {
            err = threadpool_shutdown;
            break;
        }

        /* Join all worker thread */
        for (i = 0; i < pool->thread_count; i++) {
            if (pthread_join(pool->threads[i].thread, NULL) != 0) {
                err = threadpool_thread_failure;
            }
        }
    } while (0);

    /* Only if everything went well do we deallocate the pool */
    if (!err) {
        threadpool_free(pool);
    }
    return err;
}

int threadpool_free(threadpool_t *pool)
{
    if (pool == NULL) {
        return -1;
    }

    if (pool->threads) {
        free(pool->threads);
    }
    free(pool);
    return 0;
}


void *threadpool_thread(void *arg)
{
    /* thread default routine */
    /*
        Choose task in queue of pool
        Desperately consume task in
        queue belong this thread
    */
    // threadpool_thread_t *thread = (threadpool_thread_t *) arg;

    threadpool_thread_t *thread = (threadpool_thread_t *) arg;

/* Scheme 1 */
#if 0
    while (1) {
        threadpool_task_t *next_task = threadpool_qpop(thread);
        while(next_task){
            (next_task->function)(next_task->argument);
            next_task = threadpool_qpop(thread);
        }
        sched_yield();
    }
#endif
    /* Scheme 2 */
    for (;;) {
        /* Grab task from queue belong this thread */
        threadpool_task_t *next_task = threadpool_qpop(thread);
        /* Call worker */
        if (next_task) {
            (next_task->function)(next_task->argument);
        }
        /* Performance improvement */
        /* Improve 5k requests     */
        sched_yield();
    }
}

threadpool_thread_t *round_robin_schedule(threadpool_t *pool)
{
    /*
        cur_index is shared variable
        and only initialize one time
    */
    static int cur_index = -1;
    assert(pool && pool->thread_count > 0);
    cur_index = (cur_index + 1) % pool->thread_count;
    return &(pool->threads[cur_index]);
}

int threadpool_qpush(threadpool_t *pool, void (*task)(void *), void *arg)
{
    /*
        decide proper queue to insert and
        push task into queue of thread
    */
    threadpool_thread_t *dest = round_robin_schedule(pool);
    return dispatch(dest, task, arg);
}

int dispatch(threadpool_thread_t *dest, void (*task)(void *), void *arg)
{
    /*
        lock-free enqueue
        work as producer
    */
    do {
        /* There isn't contention in push_index */
        int p = dest->push_index;

        /* check whether full */
        if (__sync_bool_compare_and_swap(&(dest->task_count), dest->size,
                                         dest->size)) {
            printf("Queue is full\n");
            return -1;
        }
        /* insert task into queue */
        dest->queue[dest->push_index].function = task;
        dest->queue[dest->push_index].argument = arg;
        dest->push_index = (p + 1) % dest->size;

        /* task_count is shared variable for qpop and qpush */
        __sync_fetch_and_add(&(dest->task_count), 1);
        break;

    } while (1);
    return 0;
}

threadpool_task_t *threadpool_qpop(threadpool_thread_t *dest)
{
    /*
        lock-free dequeue
        work as consumer
    */
    do {
        /* There isn't contention in pop_index */
        /* check whether empty */
        int p = dest->pop_index;
        if (__sync_bool_compare_and_swap(&(dest->task_count), 0, 0)) {
            // printf("Queue is empty\n");
            return NULL;
        }
        dest->pop_index = (p + 1) % dest->size;
        /* task_count is shared variable for qpop and qpush */
        __sync_fetch_and_sub(&(dest->task_count), 1);
        return dest->queue + p;

    } while (1);
    return NULL;
}