#include "arch_asm.h"
#include "core.h"
#include "lps_core.h"

#include <sys/mman.h>
#include <stdlib.h>
#include <assert.h>

EXPORT struct LPSBox *
lps_box_new(struct LPSEngine *engine) 
{
    struct LPSBox *box = malloc(sizeof(struct LPSBox));

    if (!box) {
        return NULL;
    }

    size_t size = engine->opts.boxsize;
    uintptr_t base = boxmap_addspace(engine->bm, size);

    if (base == 0) {
        goto err1;
    }

    *box = (struct LPSBox) {
        .base = base,
        .size = size,
        .engine = engine,
    };

    bool ok = mm_init(&box->mm, base, size, 4 * 1024);
    if (!ok) {
        goto err2;
    }

    return box;

err2:
    boxmap_rmspace(engine->bm, base, size);
err1:
    free(box);
    return NULL;
}


static void
cbunmap(uint64_t start, size_t len, struct MMInfo info, void *udata)
{
    (void) udata, (void) info;
    void *p = mmap((void *) start, len, PROT_NONE,
        MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED, -1, 0);
    assert(p == (void *) start);
}


EXPORT uintptr_t
lps_box_mapat(struct LPSBox *box, uintptr_t addr, size_t size, int prot, int flags,
    int fd, off_t off) 
{
    // TODO: dasics support
    assert(addr >= box->base && addr + size <= box->base + box->size);

    uintptr_t m_addr = mm_mapat_cb(&box->mm, addr, size, prot, flags,
        fd, off, cbunmap, NULL);
    if (m_addr == (uintptr_t) -1) {
        return (uintptr_t) -1;
    }
    
    void *mem = mmap((void *) m_addr, size, prot, flags | MAP_FIXED, fd, off);
    if (mem == (void *) -1) {
        mm_unmap(&box->mm, m_addr, size);
        return (uintptr_t) -1;
    }

    return (uintptr_t) mem;
}

EXPORT uintptr_t
lps_box_mapany(struct LPSBox *box, size_t size, int prot, int flags, int fd,
    off_t off)
{
    uintptr_t m_addr = mm_mapany(&box->mm, size, prot, flags, fd, off);
    if (m_addr == (uintptr_t) -1) {
        return (uintptr_t) -1;
    }

    void *mem = mmap((void *) m_addr, size, prot, flags | MAP_FIXED, fd, off);
    if (mem == (void *) -1) {
        mm_unmap(&box->mm, m_addr, size);
        return (uintptr_t) -1;
    }

    return (uintptr_t) mem;
}


EXPORT int
lps_box_munmap(struct LPSBox *box, uintptr_t addr, size_t size)
{
    if (addr >= box->base && addr + size <= box->base + box->size) {
        return mm_unmap_cb(&box->mm, addr, size, cbunmap, NULL);
    }
    return -1;
}

EXPORT int
lps_box_mprotect(struct LPSBox *box, uintptr_t addr, size_t size, int prot)
{
    assert(addr >= box->base && addr + size <= box->base + box->size);

    return mprotect(addr, size, prot);
}


EXPORT struct LPSBoxInfo
lps_box_info(struct LPSBox *box)
{
    return (struct LPSBoxInfo) {
        .base = box->base,
        .size = box->size,
    };
}