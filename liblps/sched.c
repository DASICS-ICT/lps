#include "sched.h"
#include "log.h"
#include "proc.h"

#include <stdlib.h>

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
rrschedstart()
{
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