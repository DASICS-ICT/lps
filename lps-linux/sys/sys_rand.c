#include "sys/sys.h"

ssize_t
sys_getrandom(struct LPSThread *t, uintptr_t bufp, size_t buflen,
    unsigned int flags)
{
    void *buf = bufhost(t, bufp, buflen, 1);
    if (!buf)
        return -LINUX_EINVAL;
    if ((flags & (~LINUX_GRND_RANDOM & ~LINUX_GRND_NONBLOCK)) != 0)
        return -LINUX_EINVAL;
    return host_getrandom(buf, buflen, flags);
}
