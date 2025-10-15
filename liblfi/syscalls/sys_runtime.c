#include <stdlib.h>

#include "syscalls/syscalls.h"
#include "runtime_call.h"
#include "schedule.h"

int sys_runtime_call(struct TuxThread* p, RuntimeCallType type) {
    switch (type) {
    case YIELD:
        yield(p);
        // unreachable
        break;
    default:
        break;
    }
    return 0;
}