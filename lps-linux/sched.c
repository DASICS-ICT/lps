#define _GNU_SOURCE
#include "psched.h"
#include "log.h"
#include "proc.h"
#include "lock.h"

#include <stdlib.h>

#include <sched.h>
#include <unistd.h>
#include <pthread.h>

// global queue for RRScheduler
static QUEUE_INIT(runq);
static QUEUE_INIT(exitq);

static struct LPSThread* 
get_next_runable() {
    struct List *e = queue_dequeue(&runq);
    if (!e)
        return NULL;
    return LIST_CONTAINER(struct LPSThread, elem, e);
}

EXPORT void
rrschedadd(struct LPSThread *t)
{
    queue_enqueue(&runq, &t->elem);
}

EXPORT void
rrschedstart(bool utimer)
{
    if (utimer) {
        lps_utimer_init();
    }
    // Round robin scheduler
    fprintf(stderr, "[RRScheduler]: start scheduling\n");
    while(true) {
        struct LPSThread *t = get_next_runable();
        if (t == NULL) return;

        lps_thread_run(t);
        if (t->state == THREAD_RUNNABLE) {
            queue_enqueue(&runq, &t->elem);
        }

        if (t->state == THREAD_EXITED) {
            fprintf(stderr, "[RRScheduler]: thread %p is finished\n", t);
        }
    }
    fprintf(stderr, "[RRScheduler]: end scheduling\n");
}

void
rrschedblock(struct Queue *q)
{
    struct LPSThread *t = lps_ctx_data(lps_cur_ctx());
    t->state = THREAD_BOLCKED;
    queue_enqueue(q, &t->elem);
    lps_thread_exit(t);
}

void
rrschedwake(struct Queue *q)
{
    struct List *e;
    while ((e = queue_dequeue(q)) != NULL) {
        struct LPSThread *t = LIST_CONTAINER(struct LPSThread, elem, e);
        t->state = THREAD_RUNNABLE;
        queue_enqueue(&runq, &t->elem);
    }
}


struct Scheduler {
    pthread_t thread_handle;  // pthread 句柄
    int core_id;              // 绑定的核心ID
    struct RingQueue *runq;    // 指向自己的本地队列
};

// Muti-Core Scheduler
#define MAX_CORES 16
struct Scheduler *mcsched[MAX_CORES];
struct RingQueue *mcsched_grunq;
int mcsched_ncore;

_Thread_local struct Scheduler *my_sched;

#define QUEUESIZE 1024

void
mcsched_loop(void *arg)
{
    struct Scheduler *sched = (struct Scheduler *)arg;

    my_sched = sched;

    void *buf[QUEUESIZE];

    size_t task_count = 0;

    // 1. check local queue
    // 2. check global queue
    // 3. steal from other queues
    while (1) {
        void *e;
        // 1. check local queue
        e = ring_pop_one(my_sched->runq);
        if (e == NULL) {
            // 2. check global queue
            int count = ring_count(mcsched_grunq) / mcsched_ncore + 1;
            count = ring_pop(mcsched_grunq, buf, count);

            if (count != 0) {
                fprintf(stderr, "[MCScheduler(%d: %p)]: get %d works from global queue\n", 
                    my_sched->core_id, my_sched, count);
                assert(ring_push(my_sched->runq, buf, count) == count);
                e = ring_pop_one(my_sched->runq);
            } else {
                // 3. steal from other queues
                for (int i = 0; i < mcsched_ncore; ++i) {
                    if (i == my_sched->core_id) continue;
                    e = ring_pop_back(mcsched[i]->runq);
                    if (e != NULL) {
                        fprintf(stderr, "[MCScheduler(%d: %p)]: steal one work(%p) from local queue(%d)\n", 
                            my_sched->core_id, my_sched, e, i);
                    }
                } 
            }
        }

        if (e != NULL) {
            struct LPSThread *t = (struct LPSThread *)e;

            lps_thread_run(t);

            if (t->state == THREAD_RUNNABLE) {
                ring_push_one(my_sched->runq, (void *)t);
            }
            if (t->state == THREAD_EXITED) {
                // fprintf(stderr, "[MCScheduler(%d: %p)]: thread %p is finished\n", 
                //     my_sched->core_id, my_sched, t);
                task_count++;
            }

        } else {
            // fprintf(stderr, "[MCScheduler(%d: %p)]: no work!\n", my_sched->core_id, my_sched);
            // Sleep for a while to avoid busy-waiting
            if (task_count) {
                fprintf(stderr, "[MCScheduler(%d: %p)]: finished %d works from last sleep!\n", 
                    my_sched->core_id, my_sched, task_count);
                task_count = 0;
            }
            usleep(10000); // Sleep for 10 ms
        }
    } 
}

void
mcshed_start()
{
    // init per-core scheduler
    for (int i = 0; i < mcsched_ncore; i++) {
        pthread_create(&mcsched[i]->thread_handle, NULL, (void* (*)(void*))mcsched_loop, mcsched[i]);

        // bind worker thread to core
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(i, &cpuset);
        if (pthread_setaffinity_np(mcsched[i]->thread_handle, sizeof(cpu_set_t), &cpuset) != 0) {
            perror("pthread_setaffinity_np failed");
        }
        fprintf(stderr, "[MCScheduler]: Worker thread %d(%p) pinned to core %d.\n", i, mcsched[i], i);
    }
}

void
mcsched_init()
{
    fprintf(stderr, "[MCScheduler]: init...\n");
    mcsched_ncore = 2;

    // init global queue
    mcsched_grunq = ring_create(QUEUESIZE);
    assert(mcsched_grunq);

    // init local queue
    for (int i = 0; i < mcsched_ncore; i++) {
        mcsched[i] = malloc(sizeof(struct Scheduler));
        assert(mcsched[i]);

        mcsched[i]->core_id = i;
        mcsched[i]->runq = ring_create(QUEUESIZE);
    }

    // init utimer: TODO
}

void
mcsched_join()
{
    for (int i = 0; i < mcsched_ncore; i++) {
        pthread_join(mcsched[i]->thread_handle, NULL);
    }
}

int
mcsched_add(struct LPSThread *t)
{
    return ring_push_one(mcsched_grunq, (void *)t);
}