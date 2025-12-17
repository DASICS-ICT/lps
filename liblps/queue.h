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

    return elem;
}

#define QUEUE_INIT(name) \
    struct Queue name = { { NULL }, &(name.dummy) }