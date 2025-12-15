#include "sys/sys.h"

uintptr_t
sys_ignore(struct LPSThread *t, const char *name)
{
    LOG(t->proc->engine, "%s: ignored", name);
    return 0;
}

uintptr_t
sys_nosys(struct LPSThread *t, const char *name)
{
    LOG(t->proc->engine, "%s: unsupported (ENOSYS)", name);
    return -LINUX_ENOSYS;
}
