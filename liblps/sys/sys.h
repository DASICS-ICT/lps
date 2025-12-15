#pragma once

#include "fd.h"
#include "host.h"
#include "lps_arch.h"
#include "lps_core.h"
#include "linux.h"
#include "proc.h"
#include "host.h"

#include <assert.h>
#include <fcntl.h>
#include <stdalign.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>

#define SIGLFI SIGUSR1

// Returns true if the pointer is valid for the sandbox.
static inline bool
ptrcheck(struct LPSThread *t, uintptr_t p)
{
    return lps_box_ptrvalid(t->proc->box, p);
}

// Returns true if the buffer is valid and properly aligned.
static inline bool
bufcheck(struct LPSThread *t, uintptr_t p, size_t size, size_t align)
{
    if (!ptrcheck(t, p))
        return false;
    if (!ptrcheck(t, p + size - 1))
        return false;
    if (p % align != 0)
        return false;
    return true;
}

// Converts a sandbox buffer to a host buffer. Returns NULL if the buffer check
// fails. Be careful when using this function: directly accessing a sandbox
// buffer can open us to TOCTOU attacks. In certain cases, TOCTOU is not
// possible or is not a concern, so we can use this function to more
// efficiently directly access the sandbox. For a safer version, use copyout
// and copyin.
// Use of this function assumes the sandbox is in the same address space as the
// host.
static inline void *
bufhost(struct LPSThread *t, uintptr_t p, size_t size, size_t align)
{
    if (!bufcheck(t, p, size, align))
        return NULL;
    return (void *) p;
}

static inline void *
ptrhost(struct LPSThread *t, uintptr_t p)
{
    if (!ptrcheck(t, p))
        return NULL;
    return (void *) p;
}

// Allocates a buffer and copies the sandbox data to it. Assumes the sandbox
// buffer has already been checked with bufcheck.
static inline void *
copyout(struct LPSThread *t, uintptr_t p, size_t size)
{
    assert(bufcheck(t, p, size, 1));
    void *host = malloc(size);
    if (!host)
        return NULL;    
    memcpy(host, (void *) p, size);
    return host;
}

static inline void
copyin(struct LPSThread *t, uintptr_t p, void *hostp, size_t size)
{
    assert(bufcheck(t, p, size, 1));
    memcpy((void *) p, hostp, size);
}

// Checks and copies the given sandbox path into host memory. The user must
// free the returned path buffer when finished.
static inline char *
pathcopy(struct LPSThread *t, uintptr_t pathp)
{
    if (!ptrcheck(t, pathp))
        return NULL;
    char *host = (char *) malloc(FILENAME_MAX);
    if (!host)
        return NULL;
    strncpy(host, (char *) pathp, FILENAME_MAX - 1);
    host[FILENAME_MAX - 1] = 0;
    return host;
}

// Checks pathp, copies it into host memory, and resolves it to a host path.
// Places the resolved host path in host_path, and returns the copied sandbox
// path. The user must free the sandbox path.
static inline char *
pathcopyresolve(struct LPSThread *t, uintptr_t pathp, char *host_path,
    size_t host_size)
{
    char *path = pathcopy(t, pathp);
    if (!path)
        return NULL;
    // LOCK_WITH_DEFER(&t->proc->cwd.lk, lk_cwd);
    // if (!path_resolve(t->proc, path, host_path, host_size)) {
    //     free(path);
    //     return NULL;
    // }
    // TODO: path join (now tmp hostpath == path)
    strncpy(host_path, path, FILENAME_MAX);
    return path;
}

uintptr_t
sys_ignore(struct LPSThread *t, const char *name);

uintptr_t
sys_nosys(struct LPSThread *t, const char *name);

int
sys_arch_prctl(struct LPSThread *t, int code, uintptr_t addr);

int
sys_set_tid_address(struct LPSThread *t, uintptr_t ctid);

int
sys_ioctl(struct LPSThread *t, int fd, unsigned long request,
    uintptr_t va0, uintptr_t va1, uintptr_t va2, uintptr_t va3);

ssize_t
sys_write(struct LPSThread *t, int fd, uintptr_t bufp, size_t size);

ssize_t
sys_writev(struct LPSThread *t, int fd, uintptr_t iovp, size_t iovcnt);

uintptr_t
sys_exit_group(struct LPSThread *t, int code);

uintptr_t
sys_brk(struct LPSThread *t, uintptr_t addr);

uintptr_t
sys_mmap(struct LPSThread *t, uintptr_t addrp, size_t length, int prot,
    int flags, int fd, off_t off);

int
sys_mprotect(struct LPSThread *t, uintptr_t addrp, size_t length, int prot);

int
sys_munmap(struct LPSThread *t, uintptr_t addrp, size_t length);

int
sys_gettid(struct LPSThread *t);

int
sys_sched_yield(struct LPSThread *t);

int
sys_sched_getaffinity(struct LPSThread *t, int32_t pid,
    uint64_t cpusetsize, uintptr_t maskaddr);

long
sys_futex(struct LPSThread *t, uintptr_t uaddrp, int op, uint32_t val,
    uint64_t timeoutp, uintptr_t uaddr2p, uint32_t val3);

uintptr_t
sys_exit(struct LPSThread *t, int code);

int
sys_clone(struct LPSThread *t, uint64_t flags, uint64_t stack,
    uint64_t ptid, uint64_t ctid, uint64_t tls, uint64_t func);

ssize_t
sys_read(struct LPSThread *t, int fd, uintptr_t bufp, size_t size);

ssize_t
sys_readv(struct LPSThread *t, int fd, uintptr_t iovp, size_t iovcnt);

off_t
sys_lseek(struct LPSThread *t, int fd, off_t offset, int whence);

ssize_t
sys_pread64(struct LPSThread *t, int fd, uintptr_t bufp, size_t size,
    ssize_t offset);

int
sys_openat(struct LPSThread *t, int dirfd, uintptr_t pathp, int flags,
    int mode);

int
sys_close(struct LPSThread *t, int fd);

ssize_t
sys_getdents64(struct LPSThread *t, int fd, uintptr_t dirp, size_t count);

int
sys_newfstatat(struct LPSThread *t, int dirfd, uintptr_t pathp,
    uintptr_t statbufp, int flags);

int
sys_fchmod(struct LPSThread *t, int fd, linux_mode_t mode);

int
sys_truncate(struct LPSThread *p, uintptr_t pathp, off_t length);

int
sys_ftruncate(struct LPSThread *t, int fd, off_t length);

int
sys_fchown(struct LPSThread *t, int fd, linux_uid_t owner,
    linux_gid_t group);

int
sys_fsync(struct LPSThread *p, int fd);

int
sys_mkdirat(struct LPSThread *t, int dirfd, uintptr_t pathp,
    linux_mode_t mode);

int
sys_unlinkat(struct LPSThread *t, int dirfd, uintptr_t pathp, int flags);

int
sys_renameat(struct LPSThread *t, int olddir, uintptr_t oldpathp, int newdir,
    uintptr_t newpathp);

int
sys_faccessat(struct LPSThread *t, int dirfd, uintptr_t pathp, int mode);

ssize_t
sys_readlinkat(struct LPSThread *t, int dirfd, uintptr_t pathp, uintptr_t bufp,
    size_t size);

int
sys_chdir(struct LPSThread *p, uintptr_t pathp);

int
sys_fchdir(struct LPSThread *t, int fd);

ssize_t
sys_getcwd(struct LPSThread *t, uintptr_t bufp, size_t size);

int
sys_nanosleep(struct LPSThread *t, uintptr_t reqp, uintptr_t remp);

int
sys_clock_gettime(struct LPSThread *t, linux_clockid_t clockid, uintptr_t tp);

ssize_t
sys_getrandom(struct LPSThread *t, uintptr_t bufp, size_t buflen,
    unsigned int flags);

int
sys_fcntl(struct LPSThread *t, int fd, int cmd, uintptr_t va0,
    uintptr_t va1, uintptr_t va2, uintptr_t va3);

linux_time_t
sys_time(struct LPSThread *t, uintptr_t tlocp);

int
sys_chmod(struct LPSThread *t, uintptr_t pathp, linux_mode_t mode);

uintptr_t
sys_passthrough(struct LPSThread *t, uintptr_t sysno, uintptr_t a0,
    uintptr_t a1, uintptr_t a2, uintptr_t a3, uintptr_t a4, uintptr_t a5);


int
sys_prctl(struct LPSThread *t, int op, uint64_t arg2, uint64_t arg3,
    uint64_t arg4, uint64_t arg5);

int
sys_sysinfo(struct LPSThread *t, uintptr_t infop);

int
sys_uname(struct LPSThread *t, uintptr_t bufp);

int
sys_renameat2(struct LPSThread *t, int olddir, uintptr_t oldpathp, int newdir,
    uintptr_t newpathp, unsigned int flags);

int
sys_getrlimit(struct LPSThread *t, int resource, uintptr_t rlimp);

int
sys_dup(struct LPSThread *t, int oldfd);

int
sys_dup3(struct LPSThread *t, int oldfd, int newfd, int flags);
