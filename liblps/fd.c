#include "lock.h"
#include "fd.h"
#include "host.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int
fdfassign(struct FDTable *t, struct FDFile *f)
{
    LOCK_WITH_DEFER(&t->lk, t_lk);
    int i;
    for (i = 0; i < LINUX_NOFILE; i++) {
        if (t->files[i] == NULL) {
            t->files[i] = f;
            LOCK_WITH_DEFER(&f->lk_refs, lk_refs);
            f->refs++;
            return i;
        }
    }
    return -1;
}

struct FDFile *
fdfget(struct FDTable *t, int fd)
{
    if (fd < 0 || fd >= LINUX_NOFILE)
        return false;
    LOCK_WITH_DEFER(&t->lk, lk);
    return t->files[fd];
}

int
fdfdup2(struct FDTable *t, int oldfd, int newfd)
{
    if (oldfd < 0 || oldfd >= LINUX_NOFILE || newfd < -1 || newfd >= LINUX_NOFILE)
        return -LINUX_EBADF;
    LOCK_WITH_DEFER(&t->lk, lk);
    struct FDFile *f = t->files[oldfd];
    if (!f)
        return -LINUX_EBADF;
    if (newfd == -1) {
        // Alloac an empty file descripter slot for newfd
        for (newfd = 0; newfd < LINUX_NOFILE; newfd++) {
            if (t->files[newfd] == NULL)
                break;
        }
        if (newfd == LINUX_NOFILE)
            return -LINUX_EMFILE;
    } else {
        if (t->files[newfd] != NULL)
            return -LINUX_EBADF;
    }
    // Now we have a valid newfd
    t->files[newfd] = f;
    LOCK_WITH_DEFER(&f->lk_refs, lk_refs);
    f->refs++;

    return newfd;
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
        t->files[i] = NULL;
    }

    t->files[0] = filenew(dup(STDIN_FILENO), NULL);
    t->files[1] = filenew(dup(STDOUT_FILENO), NULL);
    t->files[2] = filenew(dup(STDERR_FILENO), NULL);
}

void
fdfree(struct FDTable *t)
{
    for (size_t i = 0; i < LINUX_NOFILE; i++) {
        struct FDFile *f = t->files[i];
        if (!f)
            continue;
        f->close(f->dev);
        if (f->path) {
            free(f->path);
        }
        free(f);
        t->files[i] = NULL;
    }
}
