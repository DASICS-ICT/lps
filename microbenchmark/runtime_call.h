#ifndef RUNTIME_CALL_H
#define RUNTIME_CALL_H

#include <unistd.h>
#include <sys/syscall.h>

typedef enum {
    YIELD = 0,
} RuntimeCallType;

static inline void runtime_yield() {
    syscall(500, YIELD);
}

#endif // RUNTIME_CALL_H