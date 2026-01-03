#pragma once

#include "list.h"

struct Queue {
    struct List dummy; // dummy head
    struct List *tail;
};

static void
queue_init(struct Queue *q)
{
    list_init(&q->dummy);
    q->tail = &q->dummy;
}

static void
queue_enqueue(struct Queue *q, struct List *elem)
{
    q->tail->next = elem;
    elem->prev = q->tail;
    elem->next = &q->dummy;
    q->dummy.prev = elem;

    q->tail = elem;
}

static int 
queue_is_empty(struct Queue *q)
{
    return q->tail == &q->dummy;
}

static struct List *
queue_dequeue(struct Queue *q) {
    if (queue_is_empty(q))
        return NULL;
    struct List *elem = q->dummy.next;
    q->dummy.next = elem->next;
    elem->next->prev = &q->dummy;

    if (q->tail == elem)
        q->tail = &q->dummy;

    return elem;
}

#define QUEUE_INIT(name) \
    struct Queue name = { { NULL }, &(name.dummy) }

// Ring Buffer Queue Structure with data type (void *)
struct RingQueue;

struct RingQueue *
ring_create(size_t cap);

int
ring_count(struct RingQueue *q);

int
ring_push_one(struct RingQueue *q, void *val);

int
ring_push(struct RingQueue *q, void **elems, int count);

// pop an elem from head
void *
ring_pop_one(struct RingQueue *q);

// pop an elem from tail, used by work steal
void *
ring_pop_back(struct RingQueue *q);

// get a bunch of elem from head, sugest number by count
int
ring_pop(struct RingQueue *q, void **elems, int count);

