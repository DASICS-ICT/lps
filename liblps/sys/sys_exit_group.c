#include "sys/sys.h"

uintptr_t
sys_exit_group(struct LPSThread *t, int code)
{
    t->state = THREAD_EXITED;
    lps_thread_exit(t);
    __builtin_unreachable();
}