#include "proc.h"
#include "myqueue.h"
#include "pal/platform.h"

static QUEUE_INIT(g_runq);
static QUEUE_INIT(g_exitq);

static struct KContext sched_ctx;

#define asm __asm__

extern void kswitch(struct LFIContext* ctx, struct KContext *old, struct KContext *new)
    asm("kswitch");

static void kswitch_from_sched(struct TuxThread* p) {
    lfi_set_myctx(p->p_ctx);
    kswitch(p->p_ctx, &sched_ctx, &p->p_ctx->k_ctx);
}

static void kswitch_to_sched(struct TuxThread* p) {
    kswitch(NULL, &p->p_ctx->k_ctx, &sched_ctx);
}

void yield(struct TuxThread* p) {
    // lfi_ctx_pause(t->p_ctx, 1); // 1 means pause
    kswitch_to_sched(p);
}

void block_on_exitq(struct TuxThread* p) {
    p->t_state = THREAD_EXITED;
    queue_enqueue(&g_exitq, &p->list);
    yield(p);
}

void block(struct queue* q, struct TuxThread* t, enum TState s) {
    t->t_state = s;
    queue_enqueue(q, &t->list);
    yield(t);
}

void block_here(struct queue* q) {
    struct TuxThread* p = (struct TuxThread*) lfi_ctx_data(lfi_get_myctx());
    p->t_state = THREAD_BOLCKED;

    queue_enqueue(q, &p->list);
    yield(p);
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

        // fprintf(stderr, "scheduler: thread %p exit with code %d\n", p, code);

        if (main_thread->t_state == THREAD_EXITED) {
            return;
        }

        if (p->t_state == THREAD_RUNNABLE) {
            queue_enqueue(&g_runq, &p->list);
        }
    }
}

// 简单轮转调度器，队列空时结束

EXPORT void scheduler_add_task(struct TuxThread* p) {
    queue_enqueue(&g_runq, &p->list);
}

static struct TuxThread* get_next_runable() {
    if (queue_is_empty(&g_runq)) {
        return NULL;       
    }
    struct list_node *node = queue_dequeue(&g_runq);
    struct TuxThread *p = list_entry(node, struct TuxThread, list);

    return p;
}

EXPORT void scheduler_begin() {
    // 简单轮转调度器，队列空时结束
    fprintf(stderr, "[scheduler]: begin scheduling\n");
    while(true) {
        struct TuxThread* p = get_next_runable();

        if (p == NULL) return;

        kswitch_from_sched(p);

        if (p->t_state == THREAD_RUNNABLE) {
            queue_enqueue(&g_runq, &p->list);
        }

        if (p->t_state == THREAD_EXITED) {
            fprintf(stderr, "[scheduler]: thread %p exited\n", p);
        }
    }
    fprintf(stderr, "[scheduler]: end scheduling\n");
}