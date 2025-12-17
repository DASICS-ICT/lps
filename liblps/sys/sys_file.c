#include "sys/sys.h"

#include <unistd.h>
#include <fcntl.h>

struct IOVec {
    uintptr_t base;
    size_t len;
};

ssize_t
sys_write(struct LPSThread *t, int fd, uintptr_t bufp, size_t size)
{
    if (size == 0)
        return 0;
    struct FDFile *f = fdfget(&t->proc->fdtable, fd);
    if (f == NULL)
        return -LINUX_EBADF;
    if (!f->write)
        return -LINUX_EPERM;
    uint8_t *buf = bufhost(t, bufp, size, 1);
    if (!buf)
        return -LINUX_EINVAL;
    return f->write(f->dev, buf, size);
}

ssize_t
sys_writev(struct LPSThread *t, int fd, uintptr_t iovp, size_t iovcnt)
{
    if (!bufcheck(t, iovp, iovcnt * sizeof(struct IOVec),
            alignof(struct IOVec)))
        return -LINUX_EINVAL;
    struct IOVec *iov = copyout(t, iovp, iovcnt * sizeof(struct IOVec));
    if (!iov)
        return -LINUX_ENOMEM;
    ssize_t total = 0;

    for (size_t i = 0; i < iovcnt; i++) {
        ssize_t n = sys_write(t, fd, iov[i].base, iov[i].len);
        if (n < 0) {
            total = n;
            goto end;
        }
        total += n;
    }

end:
    free(iov);
    return total;
}

ssize_t
sys_read(struct LPSThread *t, int fd, uintptr_t bufp, size_t size)
{
    if (size == 0)
        return 0;
    struct FDFile *f = fdfget(&t->proc->fdtable, fd);
    if (f == NULL)
        return -LINUX_EBADF;
    if (!f->read)
        return -LINUX_EPERM;
    uint8_t *buf = bufhost(t, bufp, size, 1);
    if (!buf)
        return -LINUX_EINVAL;
    return f->read(f->dev, buf, size);
}

ssize_t
sys_readv(struct LPSThread *t, int fd, uintptr_t iovp, size_t iovcnt)
{
    if (!bufcheck(t, iovp, iovcnt * sizeof(struct IOVec),
            alignof(struct IOVec)))
        return -LINUX_EINVAL;
    struct IOVec *iov = copyout(t, iovp, iovcnt * sizeof(struct IOVec));
    if (!iov)
        return -LINUX_ENOMEM;
    ssize_t total = 0;

    for (size_t i = 0; i < iovcnt; i++) {
        ssize_t n = sys_read(t, fd, iov[i].base, iov[i].len);
        if (n < 0) {
            total = n;
            goto end;
        }
        total += n;
    }

end:
    free(iov);
    return total;
}

static int
openflags(int flags)
{
    return ((flags & LINUX_O_RDONLY) ? O_RDONLY : 0) |
        ((flags & LINUX_O_WRONLY) ? O_WRONLY : 0) |
        ((flags & LINUX_O_RDWR) ? O_RDWR : 0) |
        ((flags & LINUX_O_CREAT) ? O_CREAT : 0) |
        ((flags & LINUX_O_APPEND) ? O_APPEND : 0) |
        ((flags & LINUX_O_NONBLOCK) ? O_NONBLOCK : 0) |
        ((flags & LINUX_O_DIRECTORY) ? __O_DIRECTORY : 0);
}

int
sys_openat(struct LPSThread *t, int dirfd, uintptr_t pathp, int flags,
    int mode)
{
    if (dirfd != LINUX_AT_FDCWD)
        return -LINUX_EBADF;
    char host_path[FILENAME_MAX];
    char *path = pathcopyresolve(t, pathp, host_path, sizeof(host_path));
    if (!path)
        return -LINUX_ENOENT;
    int kfd = open(host_path, openflags(flags), mode);
    if (kfd < 0) {
        LOG(t->proc->engine, "sys_open(\"%s\") = %d", path, host_err(errno));
        free(path);
        return host_err(errno);
    }
    struct FDFile *f = filenew(kfd, path);
    if (!f)
        return -LINUX_ENOENT;
    int fd = fdfassign(&t->proc->fdtable, f);
    if (fd < 0) {
        LOG(t->proc->engine, "sys_open(\"%s\") = %d", path, -LINUX_EMFILE);
        free(path);
        return -LINUX_EMFILE;
    }
    LOG(t->proc->engine, "sys_open(\"%s\") = %d(kfd:%d)", path, fd, kfd);
    return fd;
}

int
sys_close(struct LPSThread *t, int fd)
{
    if (!fdfclose(&t->proc->fdtable, fd))
        return -LINUX_EBADF;
    return 0;
}


off_t
sys_lseek(struct LPSThread *t, int fd, off_t offset, int whence)
{
    struct FDFile *f = fdfget(&t->proc->fdtable, fd);
    if (f == NULL)
        return -LINUX_EBADF;
    if (!f->lseek)
        return -LINUX_EPERM;
    return f->lseek(f->dev, offset, whence);
}

ssize_t
sys_pread64(struct LPSThread *t, int fd, uintptr_t bufp, size_t size,
    ssize_t offset)
{
    struct FDFile *f = fdfget(&t->proc->fdtable, fd);
    if (!f)
        return -LINUX_EBADF;
    if (!f->read || !f->read)
        return -LINUX_EPERM;
    uint8_t *buf = bufhost(t, bufp, size, 1);
    if (!buf)
        return -LINUX_EFAULT;
    ssize_t orig = f->lseek(f->dev, 0, SEEK_CUR);
    if (orig < 0)
        return host_err(orig);
    f->lseek(f->dev, offset, SEEK_SET);
    ssize_t n = f->read(f->dev, buf, size);
    f->lseek(f->dev, orig, SEEK_SET);
    return n;
}

ssize_t
sys_getdents64(struct LPSThread *t, int fd, uintptr_t dirp, size_t count)
{
    struct FDFile *f = fdfget(&t->proc->fdtable, fd);
    if (!f)
        return -LINUX_EBADF;
    if (!f->getdents)
        return -LINUX_EPERM;
    uint8_t *buf = bufhost(t, dirp, count, alignof(struct Dirent));
    if (!buf)
        return -LINUX_EINVAL;
    return f->getdents(f->dev, buf, count);
}

int
sys_newfstatat(struct LPSThread *t, int dirfd, uintptr_t pathp,
    uintptr_t statbufp, int flags)
{
    struct Stat *stat_ = bufhost(t, statbufp, sizeof(struct Stat),
        alignof(struct Stat));
    if (!stat_)
        return -LINUX_EINVAL;
    if ((flags & LINUX_AT_EMPTY_PATH) == 0) {
        char host_path[FILENAME_MAX];
        char *path = pathcopyresolve(t, pathp, host_path, sizeof(host_path));
        if (!path)
            return -LINUX_EINVAL;
        free(path);
        if (dirfd != LINUX_AT_FDCWD)
            return -LINUX_EBADF;
        return host_fstatat(LINUX_AT_FDCWD, host_path, stat_, flags);
    }
    struct FDFile *f = fdfget(&t->proc->fdtable, dirfd);
    if (!f)
        return -LINUX_EBADF;
    if (!f->stat_)
        return -LINUX_EPERM;
    return f->stat_(f->dev, stat_);
}

int
sys_fchmod(struct LPSThread *t, int fd, linux_mode_t mode)
{
    struct FDFile *f = fdfget(&t->proc->fdtable, fd);
    if (!f)
        return -LINUX_EBADF;
    if (!f->chmod)
        return -LINUX_EPERM;
    return f->chmod(f->dev, mode);
}

int
sys_truncate(struct LPSThread *t, uintptr_t pathp, off_t length)
{
    char host_path[FILENAME_MAX];
    char *path = pathcopyresolve(t, pathp, host_path, sizeof(host_path));
    if (!path)
        return -LINUX_EINVAL;
    free(path);
    return HOST_ERR(int, truncate(host_path, length));
}

int
sys_ftruncate(struct LPSThread *t, int fd, off_t length)
{
    struct FDFile *f = fdfget(&t->proc->fdtable, fd);
    if (!f)
        return -LINUX_EBADF;
    if (!f->truncate)
        return -LINUX_EPERM;
    return f->truncate(f->dev, length);
}

int
sys_fchown(struct LPSThread *t, int fd, linux_uid_t owner,
    linux_gid_t group)
{
    struct FDFile *f = fdfget(&t->proc->fdtable, fd);
    if (!f)
        return -LINUX_EBADF;
    if (!f->chown)
        return -LINUX_EPERM;
    return f->chown(f->dev, owner, group);
}
int
sys_fsync(struct LPSThread *t, int fd)
{
    struct FDFile *f = fdfget(&t->proc->fdtable, fd);
    if (!f)
        return -LINUX_EBADF;
    if (!f->sync)
        return -LINUX_EPERM;
    return f->sync(f->dev);
}

int
sys_mkdirat(struct LPSThread *t, int dirfd, uintptr_t pathp,
    linux_mode_t mode)
{
    if (dirfd != LINUX_AT_FDCWD)
        return -LINUX_EBADF;
    char host_path[FILENAME_MAX];
    char *path = pathcopyresolve(t, pathp, host_path, sizeof(host_path));
    if (!path)
        return -LINUX_EINVAL;
    free(path);
    return HOST_ERR(int, mkdir(host_path, mode));
}

int
sys_unlinkat(struct LPSThread *t, int dirfd, uintptr_t pathp, int flags)
{
    if (dirfd != LINUX_AT_FDCWD)
        return -LINUX_EBADF;
    if (flags != 0)
        return -LINUX_EINVAL;
    char host_path[FILENAME_MAX];
    char *path = pathcopyresolve(t, pathp, host_path, sizeof(host_path));
    if (!path)
        return -LINUX_EINVAL;
    free(path);
    return HOST_ERR(int, unlink(host_path));
}

int
sys_renameat(struct LPSThread *t, int olddir, uintptr_t oldpathp, int newdir,
    uintptr_t newpathp)
{
    if (olddir != LINUX_AT_FDCWD || newdir != LINUX_AT_FDCWD)
        return -LINUX_EBADF;
    char old_host_path[FILENAME_MAX];
    char *oldpath = pathcopyresolve(t, oldpathp, old_host_path,
        sizeof(old_host_path));
    if (!oldpath)
        return -LINUX_EINVAL;
    free(oldpath);
    char new_host_path[FILENAME_MAX];
    char *newpath = pathcopyresolve(t, newpathp, new_host_path,
        sizeof(new_host_path));
    if (!newpath)
        return -LINUX_EINVAL;
    free(newpath);
    return HOST_ERR(int, rename(old_host_path, new_host_path));
}

int
sys_renameat2(struct LPSThread *t, int olddir, uintptr_t oldpathp, int newdir,
    uintptr_t newpathp, unsigned int flags)
{
    if (flags != 0) {
        return -LINUX_EINVAL;
    }

    return sys_renameat(t, olddir, oldpathp, newdir, newpathp);
}


static int
accessmode(int mode)
{
    if (mode == LINUX_F_OK)
        return F_OK;
    return ((mode & LINUX_R_OK) ? R_OK : 0) | ((mode & LINUX_W_OK) ? W_OK : 0) |
        ((mode & LINUX_X_OK) ? X_OK : 0);
}

int
sys_faccessat(struct LPSThread *t, int dirfd, uintptr_t pathp, int mode)
{
    if (dirfd != LINUX_AT_FDCWD)
        return -LINUX_EBADF;
    char host_path[FILENAME_MAX];
    char *path = pathcopyresolve(t, pathp, host_path, sizeof(host_path));
    if (!path)
        return -LINUX_EINVAL;
    free(path);
    return HOST_ERR(int, access(host_path, accessmode(mode)));
}

ssize_t
sys_readlinkat(struct LPSThread *t, int dirfd, uintptr_t pathp, uintptr_t bufp,
    size_t size)
{
    if (dirfd != LINUX_AT_FDCWD)
        return -LINUX_EBADF;
    char host_path[FILENAME_MAX];
    char *path = pathcopyresolve(t, pathp, host_path, sizeof(host_path));
    if (!path)
        return -LINUX_EINVAL;
    free(path);

    char *buf = bufhost(t, bufp, size, 1);
    if (!buf)
        return -LINUX_EINVAL;

    return HOST_ERR(ssize_t, readlink(host_path, buf, size));
}

int
sys_dup(struct LPSThread *t, int oldfd)
{
    return fdfdup2(&t->proc->fdtable, oldfd, -1);
}

int
sys_dup3(struct LPSThread *t, int oldfd, int newfd, int flags)
{
    if (flags != 0)
        return -LINUX_EINVAL;
    return fdfdup2(&t->proc->fdtable, oldfd, newfd);
}
