#pragma once
#include "proc.h"
#include "myqueue.h"

void yield(struct TuxThread* t);

void block(struct queue* q, struct TuxThread* t, enum TState s);

void wake_all(struct queue* q);

void scheduler(struct TuxThread* t);

void block_here(struct queue* q);

void scheduler_add_task(struct TuxThread* p);

void scheduler_begin();

void block_on_exitq(struct TuxThread* p);