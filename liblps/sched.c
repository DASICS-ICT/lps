#include "sched.h"
#include "proc.h"
#include "log.h"

#include <stdlib.h>

EXPORT struct RRScheduler *
rrschedinit()
{
    struct RRScheduler *s = malloc(sizeof(struct RRScheduler));
    if (!s)
        return NULL;
    queue_init(&s->runq);
    queue_init(&s->exitq);
}

static struct LPSThread* 
get_next_runable(struct RRScheduler *s) {
    if (queue_is_empty(&s->runq)) {
        return NULL;       
    }
    struct list_node *node = queue_dequeue(&s->runq);
    struct LPSThread *t = list_entry(node, struct LPSThread, list);
    return t;
}

EXPORT void
rrschedadd(struct RRScheduler *s, struct LPSThread *t)
{
    queue_enqueue(&s->runq, &t->list);
}

EXPORT void
rrschedstart(struct RRScheduler *s)
{
    // Round robin scheduler
    fprintf(stderr, "[RRScheduler]: start scheduling\n");
    while(true) {
        struct LPSThread *t = get_next_runable(s);

        if (t == NULL) return;

        lps_thread_run(t);

        if (t->state == THREAD_RUNNABLE) {
            rrschedadd(s, t);
        }

        if (t->state == THREAD_EXITED) {
            fprintf(stderr, "[RRScheduler]: thread %p is finished\n", t);
        }
    }
    fprintf(stderr, "[RRScheduler]: end scheduling\n");
}