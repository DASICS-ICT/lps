#pragma once
#include "proc.h"
#include "myqueue.h"

void yield(struct TuxThread* p);

void block(struct queue* q, struct TuxThread* t, enum TState s);

void block_here(struct queue* q);

void block_on_exitq(struct TuxThread* p);

void wake_all(struct queue* q);

void scheduler_add_task(struct TuxThread* p);

void scheduler_begin();