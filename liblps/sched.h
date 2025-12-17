#pragma once

#include "lps_linux.h"
#include "queue.h"

struct RRScheduler {
    struct queue runq;
    struct queue exitq;
};

struct RRScheduler *
rrschedinit();

void
rrschedadd(struct RRScheduler *s, struct LPSThread *t);

void
rrschedstart(struct RRScheduler *s);