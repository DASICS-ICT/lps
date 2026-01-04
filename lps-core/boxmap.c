#include "boxmap.h"

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

static uintptr_t
truncp(uintptr_t addr, size_t align)
{
    return addr - (addr % align);
}

static uintptr_t
ceilp(uintptr_t addr, size_t align)
{
    uintptr_t rem = addr % align;
    if (rem == 0) {
        return addr;
    }
    return addr + (align - rem);
}

static size_t
gb(size_t x)
{
    return x * 1024 * 1024 * 1024;
}

static size_t
tb(size_t x)
{
    return x * 1024 * 1024 * 1024 * 1024;
}

struct BoxMap *
boxmap_new(struct BoxMapOptions opts)
{
    struct BoxMap *map = calloc(sizeof(struct BoxMap), 1);
    if (!map)
        return NULL;
    map->opts = opts;
    return map;
}

void
boxmap_delete(struct BoxMap *map)
{
    for (size_t i = 0; i < map->nregions; i++) {
        munmap(map->regions[i].base, map->regions[i].size);
        extalloc_delete(map->regions[i].alloc);
    }

    free(map);
}

uint64_t
boxmap_size(struct BoxMap *map)
{
    size_t total = 0;
    for (size_t i = 0; i < map->nregions; i++) {
        total += map->regions[i].size;
    }
    return total;
}

uint64_t
boxmap_active(struct BoxMap *map)
{
    size_t total = 0;
    for (size_t i = 0; i < map->nregions; i++) {
        total += map->regions[i].active;
    }
    return total;
}

// Attempt to reserve as much virtual address space as possible, starting with
// 'size'. Returns 0 if it is not able to reserve at least 'threshold'.
static size_t
reserve(size_t size, size_t threshold, void **base)
{
    void *p;
    do {
        p = mmap(NULL, size, PROT_NONE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
        if (p == (void *) -1) {
            size /= 2;
        }
        if (size < threshold)
            return 0;
    } while (p == (void *) -1);
    *base = p;
    return size;
}

static bool
addregion(struct BoxMap *map, void *base, size_t size)
{
    if (map->nregions >= ADDR_REGION_MAX) {
        return false;
    }

    // Since mmap gives us something page-aligned, we need to find a region
    // within it that is properly chunk-aligned.
    uintptr_t alignbase = ceilp((uintptr_t) base, map->opts.chunksize);
    size_t alignsize = truncp(alignbase +
                               (size - (alignbase - (uintptr_t) base)),
                           map->opts.chunksize) -
        alignbase;

    struct ExtAlloc *alloc = extalloc_new(alignbase, alignsize,
        map->opts.chunksize);
    if (!alloc)
        return false;

    void *region = mmap((void *) alignbase, alignsize, PROT_NONE,
        MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED, -1, 0);
    if (region != (void *) alignbase) {
        free(alloc);
        return false;
    }

    map->regions[map->nregions++] = (struct AddrRegion) {
        .base = (void *) alignbase,
        .size = alignsize,
        .alloc = alloc,
    };

    return true;
}

bool
boxmap_reserve(struct BoxMap *map, size_t size)
{
    size_t total = size;
    size_t min = size;
    size_t totalgot = 0;

    if (size == 0) {
        total = tb(256);
        size = tb(255);
        min = gb(32);
    }
    size_t i_size = size;

    int i;
    for (i = 0; i < ADDR_REGION_MAX; i++) {
        void *base;
        size_t got = reserve(size, min, &base);
        if (!got)
            break;
        totalgot += got;
        total = total - got;
        size = total;
        if (!addregion(map, base, got))
            return false;
        if (totalgot >= i_size)
            break;
    }
    if (totalgot < i_size) {
        return false;
    }
    return true;
}

static bool
isfull(struct BoxMap *map)
{
    for (size_t i = 0; i < map->nregions; i++) {
        if (!extalloc_is_full(map->regions[i].alloc))
            return false;
    }
    return true;
}

// This function can only be called if the engine is not full.
static uintptr_t
allocslot(struct BoxMap *map, size_t size)
{
    for (size_t i = 0; i < map->nregions; i++) {
        if (!extalloc_is_full(map->regions[i].alloc)) {
            map->regions[i].active++;
            return extalloc_alloc(map->regions[i].alloc, size);
        }
    }
    __builtin_unreachable();
}

static void
deleteslot(struct BoxMap *map, uintptr_t base, size_t size)
{
    for (size_t i = 0; i < map->nregions; i++) {
        uintptr_t vabase = (uintptr_t) map->regions[i].base;
        if (base >= vabase && base < vabase + map->regions[i].size) {
            extalloc_free(map->regions[i].alloc, base, size);
            map->regions[i].active--;
        }
    }
}

uintptr_t
boxmap_addspace(struct BoxMap *map, size_t size)
{
    if (isfull(map)) {
        return 0;
    }

    return allocslot(map, size);
}

void
boxmap_rmspace(struct BoxMap *map, uintptr_t space, size_t size)
{
    deleteslot(map, space, size);
}


struct ExtAlloc *
extalloc_new(uintptr_t base, size_t size, size_t chunksize)
{
    assert(base % chunksize == 0);
    assert(size % chunksize == 0);
    base /= chunksize;
    size /= chunksize;

    struct ExtAlloc *a = malloc(sizeof(struct ExtAlloc));
    if (!a)
        return NULL;
    uint8_t *bitvec = calloc(size / 8 + 1, 1);
    if (!bitvec) {
        free(a);
        return NULL;
    }
    *a = (struct ExtAlloc) {
        .base = base,
        .size = size,
        .bitvec = bitvec,
        .chunksize = chunksize,
    };
    return a;
}

static size_t
bit(uint8_t *bitvec, size_t bit)
{
    size_t byte = bit / 8;
    size_t bit_off = 7 - (bit % 8);
    return (bitvec[byte] >> bit_off) & 1;
}

static ssize_t
bitvec_find_zeroes(uint8_t *bitvec, size_t bitvec_size, size_t n)
{
    if (n <= 0 || bitvec_size == 0)
        return -1;

    size_t total_bits = bitvec_size * 8;
    size_t count = 0;

    for (size_t i = 0; i < total_bits; i++) {
        if (bit(bitvec, i) == 0) {
            count++;
            if (count == n)
                return i - n + 1;
        } else {
            count = 0;
        }
    }

    return -1;
}

static void
bitvec_set(uint8_t *bitvec, size_t start, size_t length, int val)
{
    if (length == 0)
        return;

    for (size_t i = 0; i < length; i++) {
        size_t bit = start + i;
        size_t byte = bit / 8;
        size_t bit_off = 7 - (bit % 8);
        if (val)
            bitvec[byte] |= (1 << bit_off);
        else
            bitvec[byte] &= ~(1 << bit_off);
    }
}

bool
extalloc_is_full(struct ExtAlloc *a)
{
    for (size_t i = 0; i < a->size; i++) {
        if (bit(a->bitvec, i) != 1)
            return false;
    }
    return true;
}

uintptr_t
extalloc_alloc(struct ExtAlloc *a, size_t n)
{
    assert(n % a->chunksize == 0);
    n /= a->chunksize;
    ssize_t idx = bitvec_find_zeroes(a->bitvec, a->size, n);
    if (idx == -1)
        return 0;
    bitvec_set(a->bitvec, idx, n, 1);
    return (a->base + idx) * a->chunksize;
}

void
extalloc_allocat(struct ExtAlloc *a, uintptr_t at, size_t n)
{
    assert(at % a->chunksize == 0);
    assert(n % a->chunksize == 0);
    at /= a->chunksize;
    n /= a->chunksize;
    bitvec_set(a->bitvec, at - a->base, n, 1);
}

void
extalloc_free(struct ExtAlloc *a, uintptr_t at, size_t n)
{
    assert(at % a->chunksize == 0);
    assert(n % a->chunksize == 0);
    at /= a->chunksize;
    n /= a->chunksize;
    bitvec_set(a->bitvec, at - a->base, n, 0);
}

void
extalloc_delete(struct ExtAlloc *a)
{
    free(a->bitvec);
    free(a);
}
