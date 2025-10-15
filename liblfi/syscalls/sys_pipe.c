#include <stdlib.h>

#include "syscalls/syscalls.h"
#include "fd.h"

#include "myqueue.h"
#include "schedule.h"

enum {
    NPIPE = 64,
    PIPESZ = 1024,
};

struct Pipe {
    char data[PIPESZ];

    bool writeopen;
    bool readopen;
    bool allocated;

    size_t nread;
    size_t nwrite;

    struct TuxThread *thread;

    struct queue readq;
    struct queue writeq;
};

struct Pipe pipes[NPIPE];

ssize_t pipe_read(void *dev, uint8_t* buf, size_t n) {
    struct Pipe* pipe = (struct Pipe*) dev;

    while (pipe->nread == pipe->nwrite && pipe->writeopen) {
        block(&pipe->readq, pipe->thread, THREAD_BOLCKED);
    }

    ssize_t i = 0;
    for (i = 0; i < n; i++) {
        if (pipe->nread == pipe->nwrite)
            break;
        buf[i] = pipe->data[pipe->nread++ % PIPESZ];
    }

    wake_all(&pipe->writeq);
    return i;
}

ssize_t pipe_write(void *dev, uint8_t* buf, size_t n) {
    struct Pipe* pipe = (struct Pipe*) dev;
    
    ssize_t i = 0;
    while (i < n) {
        if (!pipe->readopen) {
            return -1;
        }
        if (pipe->nwrite == pipe->nread + PIPESZ) {
            // pipe buf is full
            wake_all(&pipe->readq);
            block(&pipe->writeq, pipe->thread, THREAD_BOLCKED); // block here
        } else {
            pipe->data[pipe->nwrite++ % PIPESZ] = buf[i];
            i++;
        }
    }
    wake_all(&pipe->readq);
    return i;
}

bool pipe_new(struct TuxThread* p, struct FDFile **f0, struct FDFile **f1) {
    struct Pipe* pipe;
    for (size_t i = 0; i < NPIPE; ++i) {
        if (!pipes[i].allocated) {
            pipe = &pipe[i];
        }
    }
    if (!pipe) {
        return false;
    }
    pipe->readopen = true;
    pipe->writeopen = true;
    pipe->nread = 0;
    pipe->nwrite = 0;
    pipe->thread = p;
    queue_init(&pipe->readq);
    queue_init(&pipe->writeq);

    struct FDFile *p0, *p1;

    p0 = malloc(sizeof(struct FDFile));
    p1 = malloc(sizeof(struct FDFile));

    if(!p0) return false;
    if(!p1) {
        free(p0);
        return false;
    }

    p0->dev = pipe;
    p0->read = pipe_read;
    p0->write = NULL;

    p1->dev = pipe;
    p1->read = NULL;
    p1->write = pipe_write;
    
    *f0 = p0;
    *f1 = p1;

    return true;
}

int sys_pipe2(struct TuxThread *p, uintptr_t pipefd, int flags) {
    // check pipefd is a vaild address
    int* pipes = (int *)pipefd;

    struct FDFile *f0, *f1;

    if (!pipe_new(p, &f0, &f1)) {
        goto err1;
    }

    struct TuxProc *proc = p->proc;

    int fd0 = fdalloc(&proc->fdtable);
    if(fd0 < 0) goto err2;
    fdassign(&proc->fdtable, fd0, f0);

    int fd1 = fdalloc(&proc->fdtable);
    if(fd1 < 0) goto err3;
    fdassign(&proc->fdtable, fd1, f1);

    pipes[0] = fd0;
    pipes[1] = fd1;

    return 0;

err3:
    fdremove(&proc->fdtable, fd0);
err2:
    free(f0);
    free(f1);
err1:
    return -1;
}