#include "kfile.h"
#include "host.h"
#include "lock.h"

#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

static inline int
getkfd(struct FDFile *f)
{
    return f->kfd;
}

static ssize_t
kread(void *dev, uint8_t *buf, size_t size)
{
    int kfd = getkfd((struct FDFile *)dev);
    return HOST_ERR(ssize_t, read(kfd, buf, size));
}

static ssize_t
kwrite(void *dev, uint8_t *buf, size_t size)
{
    int kfd = getkfd((struct FDFile *)dev);
    return HOST_ERR(ssize_t, write(kfd, buf, size));
}

static ssize_t
klseek(void *dev, linux_off_t off, int whence)
{
    int kfd = getkfd((struct FDFile *)dev);
    return HOST_ERR(ssize_t, lseek(kfd, off, whence));
}

static int
kftruncate(void *dev, linux_off_t length)
{
    int kfd = getkfd((struct FDFile *)dev);
    return HOST_ERR(int, ftruncate(kfd, length));
}

static int
kfchown(void *dev, linux_uid_t owner, linux_gid_t group)
{
    int kfd = getkfd((struct FDFile *)dev);
    return HOST_ERR(int, fchown(kfd, owner, group));
}

static int
kchmod(void *dev, linux_mode_t mode)
{
    int kfd = getkfd((struct FDFile *)dev);
    return HOST_ERR(int, fchmod(kfd, mode));
}

static int
kclose(void *dev)
{
    int kfd = getkfd((struct FDFile *)dev);
    return HOST_ERR(int, close(kfd));
}

static int
kstat(void *dev, struct Stat *stat)
{
    int kfd = getkfd((struct FDFile *)dev);
    return host_fstatat(kfd, "", stat, LINUX_AT_EMPTY_PATH);
}

static int
ksync(void *dev)
{
    int kfd = getkfd((struct FDFile *)dev);
    return HOST_ERR(int, fsync(kfd));
}

static ssize_t
kgetdents(void* dev, void* dirp, size_t count)
{
    int kfd = getkfd((struct FDFile *)dev);
    return host_getdents64(kfd, dirp, count);
}

static int
kioctl
(void *dev, unsigned long request, void *arg)
{
    int kfd = getkfd((struct FDFile *)dev);
    return HOST_ERR(int, ioctl(kfd, request, arg));
}

// file 
struct FDFile *
filenew(int kfd, char *path)
{
    struct FDFile *f = malloc(sizeof(struct FDFile));
    if (!f) {
        return NULL;
    }
    *f = (struct FDFile) {
        .dev = f,
        .refs = 0,
        .read = kread,
        .write = kwrite,
        .lseek = klseek,
        .close = kclose,
        .stat_ = kstat,
        .getdents = kgetdents,
        .chown = kfchown,
        .chmod = kchmod,
        .truncate = kftruncate,
        .sync = ksync,
        .ioctl = kioctl,
        .kfd = kfd,
        .path = path,
    };
    pthread_mutex_init(&f->lk_refs, NULL);
    return f;
}

void
filefree(struct FDFile *f)
{
    if (!f)
        return;
    LOCK_WITH_DEFER(&f->lk_refs, lk_refs);
    f->refs--;
    if (!f->refs)
        return;
    // Actually free f
    if (f->path)
        free(f->path);
    if (!f->close) {
        f->close((void *) f);
    }
    free(f);
}