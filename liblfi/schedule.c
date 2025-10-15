#include "proc.h"
#include "myqueue.h"

QUEUE_INIT(g_runq);

void yield(struct TuxThread* t) {
    lfi_ctx_pause(t->p_ctx, 0);
}

void block(struct queue* q, struct TuxThread* t, enum TState s) {
    t->t_state = s;
    queue_enqueue(q, &t->list);
    yield(t);
}

void wake_all(struct queue* q) {
    while (!queue_is_empty(q)) {
        struct list_node *node = queue_dequeue(q);
        struct TuxThread *t = list_entry(node, struct TuxThread, list);

        t->t_state = THREAD_RUNNABLE;
        queue_enqueue(&g_runq, &t->list);
    }
}

struct TuxThread* runnable_proc() {
    while (true) {
        if (!queue_is_empty(&g_runq)) {
            struct list_node *node = queue_dequeue(&g_runq);
            struct TuxThread *t = list_entry(node, struct TuxThread, list);

            return t;
        }

        // wating?
    }
}

EXPORT void scheduler(struct TuxThread* t) {
    t->t_state = THREAD_RUNNABLE;
    queue_enqueue(&g_runq, &t->list);

    struct TuxThread* main_thread = t;

    while(true) {
        struct TuxThread* p = runnable_proc();

        int code = lfi_tux_proc_run(p);

        printf("scheduler: thread %p exit with code %d", p, code);

        if (main_thread->t_state == THREAD_EXITED) {
            return;
        }

        if (p->t_state == THREAD_RUNNABLE) {
            queue_enqueue(&g_runq, &p->list);
        }
    }
}