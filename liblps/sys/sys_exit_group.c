#include "sys/sys.h"

uintptr_t
sys_exit_group(struct LPSThread *t, int code)
{
    t->state = THREAD_EXITED;
    lps_kswitch_from(t->ctx);
    __builtin_unreachable();
}