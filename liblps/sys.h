#pragma once

#include "arch_sys.h"
#include "proc.h"

#define LPS_SYS_runtime_call 500

_Static_assert(LPS_SYS_runtime_call > LINUX_SYS_LAST, "LPS_SYS_runtime_call invalid");

#define SYS(SYSNAME, expr)    \
    case LINUX_SYS_##SYSNAME: \
        handled = true;       \
        r = expr;             \
        break;

#define LPS(SYSNAME, expr)  \
    case LPS_SYS_##SYSNAME: \
        handled = true;     \
        r = expr;           \
        break;

// Generic syscall handler.
uintptr_t
syshandle(struct LPSThread *t, uintptr_t sysno, uintptr_t a0, uintptr_t a1,
    uintptr_t a2, uintptr_t a3, uintptr_t a4, uintptr_t a5);
