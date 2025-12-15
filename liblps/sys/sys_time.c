#include "sys/sys.h"

#include <time.h>

int
sys_nanosleep(struct LPSThread *t, uintptr_t reqp, uintptr_t remp)
{
    struct TimeSpec *box_req = bufhost(t, reqp, sizeof(struct TimeSpec),
        alignof(struct TimeSpec));
    if (!box_req)
        return -LINUX_EINVAL;
    struct timespec req, rem;
    req.tv_sec = box_req->sec;
    req.tv_nsec = box_req->nsec;
    int r = nanosleep(&req, &rem);
    if (r < 0)
        return host_err(errno);

    if (remp) {
        uint8_t *remu = bufhost(t, remp, sizeof(struct TimeSpec),
            alignof(struct TimeSpec));
        if (!remu)
            return -LINUX_EFAULT;
        struct TimeSpec *box_rem = (struct TimeSpec *) remu;
        box_rem->sec = rem.tv_sec;
        box_rem->nsec = rem.tv_nsec;
    }

    return r;
}

int
sys_clock_gettime(struct LPSThread *t, linux_clockid_t clockid, uintptr_t tp)
{
    struct TimeSpec *box_ts = bufhost(t, tp, sizeof(struct TimeSpec),
        alignof(struct TimeSpec));
    if (!box_ts)
        return -LINUX_EFAULT;
    struct timespec ts;
    int err = clock_gettime(clockid, &ts);
    if (err < 0)
        return host_err(errno);
    box_ts->sec = ts.tv_sec;
    box_ts->nsec = ts.tv_nsec;
    return 0;
}