#include "sys/sys.h"
#include "psched.h"

#define NPIPE 64
#define PIPESZ 1024

struct Pipe {
    char data[PIPESZ];

    bool writeopen;
    bool readopen;
    bool allocated;

    size_t nread;
    size_t nwrite;

    struct Queue readq;
    struct Queue writeq;  
};

static struct Pipe pipes[NPIPE];

static ssize_t 
pipe_read(void *dev, uint8_t* buf, size_t n) 
{
    struct Pipe* pipe = (struct Pipe*) dev;
    DBG("[pipe_read]: pipe:%p, nread:%u, nwrite:%u", pipe, pipe->nread, pipe->nwrite);

    while (pipe->nread == pipe->nwrite && pipe->writeopen) {
        DBG("[pipe_read]: pipe:%p, blocked", pipe);
        rrschedblock(&pipe->readq);
        DBG("[pipe_read]: pipe:%p, waked up", pipe);
    }

    ssize_t i = 0;
    for (i = 0; i < n; i++) {
        if (pipe->nread == pipe->nwrite)
            break;
        buf[i] = pipe->data[pipe->nread++ % PIPESZ];
    }

    DBG("[pipe_read]: pipe:%p, nread:%u, nwrite:%u", pipe, pipe->nread, pipe->nwrite);
    rrschedwake(&pipe->writeq);
    return i;
}

static ssize_t 
pipe_write(void *dev, uint8_t* buf, size_t n) 
{
    struct Pipe* pipe = (struct Pipe*) dev;
    DBG("[pipe_write]: pipe:%p, nread:%u, nwrite:%u", pipe, pipe->nread, pipe->nwrite);
    
    ssize_t i = 0;
    while (i < n) {
        if (!pipe->readopen) {
            return -1;
        }
        if (pipe->nwrite == pipe->nread + PIPESZ) {
            // pipe buf is full
            rrschedwake(&pipe->readq);
            DBG("[pipe_write]: pipe:%p, blocked", pipe);
            rrschedblock(&pipe->writeq);
            DBG("[pipe_write]: pipe:%p, waked up", pipe);
        } else {
            pipe->data[pipe->nwrite++ % PIPESZ] = buf[i];
            i++;
        }
    }
    rrschedwake(&pipe->readq);
    DBG("[pipe_write]: pipe:%p, nread:%u, nwrite:%u", pipe, pipe->nread, pipe->nwrite);
    return i;
}

bool pipe_new(struct FDFile **f0, struct FDFile **f1) 
{
    struct Pipe* pipe = NULL;
    for (size_t i = 0; i < NPIPE; ++i) {
        if (!pipes[i].allocated) {
            pipe = &pipes[i];
            break;
        }
    }
    if (!pipe) {
        return false;
    }
    pipe->allocated = true;
    pipe->readopen = true;
    pipe->writeopen = true;
    pipe->nread = 0;
    pipe->nwrite = 0;
    queue_init(&pipe->readq);
    queue_init(&pipe->writeq);

    struct FDFile *p0 = malloc(sizeof(struct FDFile));
    struct FDFile *p1 = malloc(sizeof(struct FDFile));

    if (!p0) {
        pipe->allocated = false;
        return false;
    }
    if(!p1) {
        pipe->allocated = false;
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

int sys_pipe2(struct LPSThread *t, uintptr_t pipefd, int flags) {
    // check pipefd is a vaild address
    int* pipes = (int *)pipefd;

    struct FDFile *f0, *f1;

    if (!pipe_new(&f0, &f1)) {
        goto err1;
    }

    int fd0 = fdfassign(&t->proc->fdtable, f0);
    if(fd0 < 0) goto err2;

    int fd1 = fdfassign(&t->proc->fdtable, f1);
    if(fd1 < 0) goto err3; 

    pipes[0] = fd0;
    pipes[1] = fd1;

    return 0;

err3:
    // fdremove(&proc->fdtable, fd0);
err2:
    free(f0);
    free(f1);
err1:
    return -1;
}

EXPORT bool
lps_pipe_new(struct LPSProc *from, int fromfd, struct LPSProc *to, int tofd)
{
    struct FDFile *readf, *writef;

    // pipe f1 -> f0
    if (!pipe_new(&readf, &writef)) {
        goto err1;
    }

    if (fdassign(&to->fdtable, tofd, readf) < 0) {
        goto err2;
    }
    if (fdassign(&from->fdtable, fromfd, writef) < 0) {
        goto err3;
    }

    return true;

err3:
    // fdremove
err2:
    free(readf);
    free(writef);
err1:    
    return false;
}