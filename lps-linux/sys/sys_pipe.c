#include "sys/sys.h"
#include "psched.h"

#include <stdlib.h>
#include <string.h>

#define NPIPE  64
#define PIPESZ (64 * 1024)

struct Pipe {
    char  *data;       // 堆分配缓冲区

    bool writeopen;
    bool readopen;
    bool allocated;

    size_t nread;
    size_t nwrite;

    struct Queue readq;
    struct Queue writeq;
};

// dev is PipeEnd
struct PipeEnd {
    struct Pipe *pipe;
    bool is_read;
};

static struct Pipe pipes[NPIPE];

static ssize_t
pipe_read(void *dev, uint8_t* buf, size_t n)
{
    struct PipeEnd *end = (struct PipeEnd *)dev;
    struct Pipe *pipe = end->pipe;
    DBG("[pipe_read]: pipe:%p, nread:%u, nwrite:%u", pipe, pipe->nread, pipe->nwrite);

    while (pipe->nread == pipe->nwrite && pipe->writeopen) {
        DBG("[pipe_read]: pipe:%p, blocked", pipe);
        rrschedblock(&pipe->readq);
        DBG("[pipe_read]: pipe:%p, waked up", pipe);
    }

    size_t avail = pipe->nwrite - pipe->nread;  // 可读字节数
    size_t to_read = (n < avail) ? n : avail;   // 实际读取字节数
    size_t copied = 0;

    while (copied < to_read) {
        size_t read_pos = pipe->nread % PIPESZ;
        size_t chunk = to_read - copied;

        // 处理环形缓冲区边界：不能跨越 PIPESZ
        if (read_pos + chunk > PIPESZ) {
            chunk = PIPESZ - read_pos;
        }

        memcpy(buf + copied, pipe->data + read_pos, chunk);
        pipe->nread += chunk;
        copied += chunk;
    }

    DBG("[pipe_read]: pipe:%p, nread:%u, nwrite:%u", pipe, pipe->nread, pipe->nwrite);
    rrschedwake(&pipe->writeq);
    return copied;
}

static ssize_t
pipe_write(void *dev, uint8_t* buf, size_t n)
{
    struct PipeEnd *end = (struct PipeEnd *)dev;
    struct Pipe *pipe = end->pipe;
    DBG("[pipe_write]: pipe:%p, nread:%u, nwrite:%u", pipe, pipe->nread, pipe->nwrite);

    size_t written = 0;

    while (written < n) {
        if (!pipe->readopen) {
            return -1;
        }

        size_t avail_space = PIPESZ - (pipe->nwrite - pipe->nread);  // 可写空间
        if (avail_space == 0) {
            // 缓冲区满，阻塞
            rrschedwake(&pipe->readq);
            DBG("[pipe_write]: pipe:%p, blocked", pipe);
            rrschedblock(&pipe->writeq);
            DBG("[pipe_write]: pipe:%p, waked up", pipe);
            continue;
        }

        size_t to_write = n - written;
        if (to_write > avail_space) {
            to_write = avail_space;
        }

        size_t write_pos = pipe->nwrite % PIPESZ;
        size_t chunk = to_write;

        // 处理环形缓冲区边界：不能跨越 PIPESZ
        if (write_pos + chunk > PIPESZ) {
            chunk = PIPESZ - write_pos;
        }

        memcpy(pipe->data + write_pos, buf + written, chunk);
        pipe->nwrite += chunk;
        written += chunk;
    }

    rrschedwake(&pipe->readq);
    DBG("[pipe_write]: pipe:%p, nread:%u, nwrite:%u", pipe, pipe->nread, pipe->nwrite);
    return written;
}

static int
pipe_close(void *dev)
{
    struct PipeEnd *end = (struct PipeEnd *)dev;
    struct Pipe *pipe = end->pipe;

    if (end->is_read) {
        pipe->readopen = false;
        DBG("[pipe_close]: pipe:%p, close read", pipe);
        rrschedwake(&pipe->writeq);
    } else {
        pipe->writeopen = false;
        DBG("[pipe_close]: pipe:%p, close write", pipe);
        rrschedwake(&pipe->readq);
    }

    if (!pipe->readopen && !pipe->writeopen) {
        free(pipe->data);
        pipe->data = NULL;
        pipe->allocated = false;
    }

    free(end);
    return 0;
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

    pipe->data = malloc(PIPESZ);
    if (!pipe->data) {
        pipe->allocated = false;
        return false;
    }

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

    struct PipeEnd *rend = malloc(sizeof(struct PipeEnd));
    struct PipeEnd *wend = malloc(sizeof(struct PipeEnd));
    if (!rend || !wend) {
        free(rend); free(wend);
        free(pipe->data);
        pipe->allocated = false;
        return false;
    }

    rend->pipe = pipe;
    rend->is_read = true;

    wend->pipe = pipe;
    wend->is_read = false;

    p0->dev   = rend;
    p0->read  = pipe_read;
    p0->write = NULL;
    p0->close = pipe_close;

    p1->dev   = wend;
    p1->read  = NULL;
    p1->write = pipe_write;
    p1->close = pipe_close;
    
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