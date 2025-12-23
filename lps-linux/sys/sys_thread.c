#include "sys/sys.h"

int
sys_set_tid_address(struct LPSThread *t, uintptr_t ctid)
{
    if (!ptrcheck(t, ctid)) {
        return -LINUX_EINVAL;
    }
    t->ctidp = ctid;
    return t->tid;
}

int
sys_gettid(struct LPSThread *t)
{
    return t->tid;
}