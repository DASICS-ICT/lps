#include "align.h"
#include "sys/sys.h"
#include "bound.h"

#include <sys/mman.h>

uintptr_t
sys_brk(struct LPSThread *t, uintptr_t addr)
{
    struct LPSProc *p = t->proc;
    // LOCK_WITH_DEFER(&p->lk_brk, lk_brk);

    uintptr_t brkp = p->brkbase + p->brksize;
    if (addr != 0) {
        if (!ptrcheck(t, addr))
            return -1;
        brkp = addr;
    }
    size_t brkmaxsize = t->proc->engine->opts.brksize;

    if (brkp < p->brkbase)
        brkp = p->brkbase;
    if (brkp >= p->brkbase + brkmaxsize)
        brkp = p->brkbase + brkmaxsize;

    size_t newsize = brkp - p->brkbase;
    assert(newsize < brkmaxsize);

    if (newsize == p->brksize)
        return brkp;

    const int MAP_FLAGS = LINUX_MAP_PRIVATE | LINUX_MAP_ANONYMOUS;
    const int MAP_PROT = PROT_READ | PROT_WRITE;

    uintptr_t pagesize = 4 * 1024;

    if (brkp >= p->brkbase + p->brksize) {
        // LOCK_WITH_DEFER(&p->lk_box, lk_box);
        uintptr_t map;
        if (p->brksize == 0) {
            map = lps_box_mapat(p->box, p->brkbase, newsize, MAP_PROT,
                MAP_FLAGS, -1, 0);
        } else {
            uintptr_t next = ceilp(p->brkbase + p->brksize, pagesize);
            map = lps_box_mapat(p->box, next, newsize - p->brksize, MAP_PROT,
                MAP_FLAGS, -1, 0);
        }
        if (map == (uintptr_t) -1)
            return -1;
    }
    p->brksize = newsize;
    LOG(p->engine, "sys_brk(%lx) = %lx", addr, p->brkbase + p->brksize);
    return p->brkbase + p->brksize;
}


uintptr_t
sys_mmap(struct LPSThread *t, uintptr_t addrp, size_t length, int prot,
    int flags, int fd, off_t off)
{
    if (length == 0)
        return -LINUX_EINVAL;
    size_t pagesize = 4 * 1024;
    length = ceilp(length, pagesize);

    // Any mmap flag outside this list is rejected.
    const int illegal_mask = ~LINUX_MAP_ANONYMOUS & ~LINUX_MAP_PRIVATE &
        ~LINUX_MAP_NORESERVE & ~LINUX_MAP_DENYWRITE & ~LINUX_MAP_FIXED;
    if ((flags & illegal_mask) != 0) {
        LOG(t->proc->engine,
            "invalid mmap flag: not one of MAP_ANONYMOUS, MAP_PRIVATE, MAP_FIXED");
        return -LINUX_EINVAL;
    }

    uintptr_t i_addrp = addrp;

    int r;
    if ((flags & LINUX_MAP_FIXED) != 0) {
        addrp = truncp(addrp, pagesize);
        if (!ptrcheck(t, addrp))
            return -1;
        r = proc_mapat(t->proc, addrp, length, prot, flags, fd, off);
    } else {
        r = proc_mapany(t->proc, length, prot, flags, fd, off, &addrp);
    }
    if (r < 0) {
        LOG(t->proc->engine, "sys_mmap((%lx), %ld, %d, %d, %d, %ld) = %d",
            i_addrp, length, prot, flags, fd, (long) off, r);
        return r;
    }
    uintptr_t ret = addrp;
    LOG(t->proc->engine, "sys_mmap(%lx (%lx), %ld, %d, %d, %d, %ld) = %lx",
        addrp, i_addrp, length, prot, flags, fd, (long) off, ret);
    return ret;
}


int
sys_mprotect(struct LPSThread *t, uintptr_t addrp, size_t length, int prot)
{
    if (!ptrcheck(t, addrp)) {
        return -1;
    }
    return lps_box_mprotect(t->proc->box, addrp, length, prot);
}

int
sys_munmap(struct LPSThread *t, uintptr_t addrp, size_t length)
{
    if (!ptrcheck(t, addrp))
        return -1;
    LOG(t->proc->engine, "sys_munmap(%lx, %ld)", addrp, length);
    return proc_unmap(t->proc, addrp, length);
}