#include "sys/sys.h"
#include "align.h"

int
sys_chdir(struct LPSThread *t, uintptr_t pathp)
{
    char *path = pathcopy(t, pathp);
    if (!path)
        return -LINUX_EINVAL;
    int r = proc_chdir(t->proc, path);
    free(path);
    return r;
}

int
sys_fchdir(struct LPSThread *t, int fd)
{
    // lock
    if (fdget(&t->proc->fdtable, fd) == -1)
        return -LINUX_EBADF;
    char *dir = fddir(&t->proc->fdtable, fd);
    if (!dir)
        return -LINUX_ENOTDIR;
    return proc_chdir(t->proc, dir);
}

ssize_t
sys_getcwd(struct LPSThread *t, uintptr_t bufp, size_t size)
{
    if (size == 0)
        return 0;
    uint8_t *buf = bufhost(t, bufp, size, 1);
    if (!buf)
        return -LINUX_EINVAL;
    // LOCK_WITH_DEFER(&t->proc->cwd.lk, lk_cwd);
    size_t len = MIN(size,
        strnlen(t->proc->cwd.path, sizeof(t->proc->cwd.path) - 1) + 1);
    memcpy(buf, t->proc->cwd.path, len);
    buf[len - 1] = 0;
    return len;
}