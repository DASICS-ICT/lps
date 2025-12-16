#include "lock.h"
#include "fd.h"
#include "host.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

bool
fdassign(struct FDTable *t, int fd, int host_fd, char *dir)
{
    if (t->passthrough)
        return true;
    if (fd < 0 || fd >= LINUX_NOFILE)
        return false;
    LOCK_WITH_DEFER(&t->lk, t_lk);
    assert(t->fds[fd] == -1);
    t->fds[fd] = host_fd;
    t->dirs[fd] = dir;
    return true;
}

int
fdfassign(struct FDTable *t, struct FDFile *f)
{
    LOCK_WITH_DEFER(&t->lk, t_lk);
    int i;
    for (i = 0; i < LINUX_NOFILE; i++) {
        if (t->files[i] == NULL) {
            t->files[i] = f;
            return i;
        }
    }
    return -1;
}

int
fdget(struct FDTable *t, int fd)
{
    if (t->passthrough)
        return fd;
    if (fd < 0 || fd >= LINUX_NOFILE)
        return false;
    LOCK_WITH_DEFER(&t->lk, lk);
    return t->fds[fd];
}

char *
fddir(struct FDTable *t, int fd)
{
    if (fd < 0 || fd >= LINUX_NOFILE)
        return false;
    LOCK_WITH_DEFER(&t->lk, lk);
    return t->dirs[fd];
}

struct FDFile *
fdgetfile(struct FDTable *t, int fd)
{
    if (fd < 0 || fd >= LINUX_NOFILE)
        return false;
    LOCK_WITH_DEFER(&t->lk, lk);
    return t->files[fd];
}

int
fddup2(struct FDTable *t, int oldfd, int newfd)
{
    if (oldfd < 0 || oldfd >= LINUX_NOFILE || newfd < -1 || newfd >= LINUX_NOFILE)
        return -LINUX_EBADF;
    LOCK_WITH_DEFER(&t->lk, lk);
    int koldfd = t->fds[oldfd];
    if (koldfd == -1)
        return -LINUX_EBADF;
    char *dir = NULL;
    if (t->dirs[oldfd]) {
        dir = malloc(FILENAME_MAX);
        if (!dir)
            return false;
    }
    int knewfd;
    if (newfd == -1) {
        knewfd = dup(koldfd);
        if (knewfd == -1)
            goto err;
        newfd = knewfd;
        t->fds[newfd] = knewfd;
    } else {
        knewfd = t->fds[newfd];
        if (knewfd == -1)
            knewfd = newfd;
        if (dup2(koldfd, knewfd) < 0)
            goto err;
        t->fds[newfd] = knewfd;
        if (t->dirs[newfd]) {
            free(t->dirs[newfd]);
            t->dirs[newfd] = NULL;
        }
    }

    if (t->dirs[oldfd]) {
        assert(t->dirs[newfd] == NULL && dir != NULL);
        t->dirs[newfd] = dir;
        strncpy(t->dirs[newfd], t->dirs[oldfd], FILENAME_MAX - 1);
    }
    return newfd;

err:
    if (dir)
        free(dir);
    return -LINUX_EINVAL;
}

bool
fdclose(struct FDTable *t, int fd)
{
    if (t->passthrough)
        return true;
    if (fd < 0 || fd >= LINUX_NOFILE)
        return false;
    LOCK_WITH_DEFER(&t->lk, lk);
    if (t->fds[fd] == -1)
        return false;
    close(t->fds[fd]);
    t->fds[fd] = -1;
    if (t->dirs[fd]) {
        free(t->dirs[fd]);
        t->dirs[fd] = NULL;
    }
    return true;
}

bool
fdfclose(struct FDTable *t, int fd)
{
    if (fd < 0 || fd >= LINUX_NOFILE)
        return false;
    LOCK_WITH_DEFER(&t->lk, lk);
    struct FDFile *f = t->files[fd];
    if (f == NULL)
        return false;
    filefree(f);
    t->files[fd] = NULL;
    return true;
}

void
fdinit(struct LPSLinuxEngine *engine, struct FDTable *t)
{
    pthread_mutex_init(&t->lk, NULL);

    for (size_t i = 0; i < LINUX_NOFILE; i++) {
        t->fds[i] = -1;
        t->files[i] = NULL;
    }

    // t->fds[0] = dup(STDIN_FILENO);
    // t->fds[1] = dup(STDOUT_FILENO);
    // t->fds[2] = dup(STDERR_FILENO);

    t->files[0] = filenew(dup(STDIN_FILENO), NULL);
    t->files[1] = filenew(dup(STDOUT_FILENO), NULL);
    t->files[2] = filenew(dup(STDERR_FILENO), NULL);

    t->passthrough = engine->opts.passthrough;
}

void
fdfree(struct FDTable *t)
{
    for (size_t i = 0; i < LINUX_NOFILE; i++) {
        if (t->fds[i] == -1)
            continue;
        close(t->fds[i]);
        t->fds[i] = -1;
        if (t->dirs[i]) {
            free(t->dirs[i]);
            t->dirs[i] = NULL;
        }
    }
}
