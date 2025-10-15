#pragma once
#include "proc.h"

void yield(struct TuxThread* t);

void block(struct queue* q, struct TuxThread* t, enum TState s);

void wake_all(struct queue* q);

void scheduler(struct TuxThread* t);