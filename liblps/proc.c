#include "proc.h"
#include "fd.h"
#include "elfload.h"
#include "cwalk.h"

#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>

struct LPSProc * 
lps_proc_new(struct LPSLinuxEngine *engine)
{
    struct LPSBox *box = lps_box_new(engine->engine);

    struct LPSProc *proc = calloc(sizeof(struct LPSProc), 1);
    if (!proc) {
        return NULL;
    }
    proc->engine = engine;
    proc->box = box;
    proc->boxinfo = lps_box_info(box);

    lps_box_setdata(box, proc);

    fdinit(engine, &proc->fdtable);

    return proc;
}

static bool
proc_load(struct LPSProc *proc, int prog_fd, const uint8_t *prog,
    size_t prog_size, const char *prog_path)
{
    // TODO: support other besides static-pie

    struct ELFLoadInfo info;

    if (!elf_load(proc, prog_path, prog_fd, prog, prog_size, 
        NULL, -1, NULL, 0, &info)) {
        return false;
    }

    proc->entry = info.elfentry;
    proc->elfinfo = info;

    proc->brkbase = info.lastva;
    proc->brksize = 0;

    size_t brkmaxsize = BRKMAXSIZE;

    uintptr_t brkregion = lps_box_mapat(proc->box, proc->brkbase, brkmaxsize,
        PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (brkregion == (uintptr_t) -1) {
        return false;
    }

    return true;
}

EXPORT bool 
lps_proc_load(struct LPSProc *proc, const uint8_t *prog, size_t prog_size,
    const char *prog_path)
{
    return proc_load(proc, -1, prog, prog_size, prog_path);
}

EXPORT bool
lps_proc_load_fd(struct LPSProc *proc, int fd, const char *prog_path)
{
    off_t cur = lseek(fd, 0, SEEK_CUR);
    off_t prog_size = lseek(fd, 0, SEEK_END);
    if (prog_size < 0)
        return false;
    lseek(fd, cur, SEEK_SET);

    lseek(fd, 0, SEEK_SET);
    void *prog_data = mmap(NULL, prog_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (prog_data == (void *) -1)
        return false;

    bool ok = proc_load(proc, fd, prog_data, prog_size, prog_path);
    munmap(prog_data, prog_size);
    return ok;
}

EXPORT bool
lps_proc_load_file(struct LPSProc *proc, const char *prog_path)
{
    FILE *f = fopen(prog_path, "r");
    if (!f) {
        LOG(proc->engine, "%s: file does not exist", prog_path);
        return false;
    }
    bool ok = lps_proc_load_fd(proc, fileno(f), prog_path);
    fclose(f);
    return ok;
}

int
proc_mapany(struct LPSProc *p, size_t size, int prot, int flags, int fd,
    off_t offset, uintptr_t *o_mapstart)
{
    int kfd = -1;
    if (fd >= 0) {
        kfd = fdfget(&p->fdtable, fd)->kfd;
        if (kfd == -1)
            return -LINUX_EBADF;
    }
    // TODO: lock
    uintptr_t addr = lps_box_mapany(p->box, size, prot, flags, kfd, offset);
    if (addr == (uintptr_t) -1)
        return -LINUX_EINVAL;
    *o_mapstart = (uintptr_t) addr;
    return 0;
}

int
proc_mapat(struct LPSProc *p, uintptr_t start, size_t size, int prot,
    int flags, int fd, off_t offset)
{
    int kfd = -1;
    if (fd >= 0) {
        kfd = fdfget(&p->fdtable, fd)->kfd;
        if (kfd == -1)
            return LINUX_EBADF;
    }
    // TODO: lock
    uintptr_t addr = lps_box_mapat(p->box, start, size, prot, flags, kfd, offset);
    if (addr == (uintptr_t) -1)
        return -LINUX_EINVAL;
    return 0;
}

int
proc_unmap(struct LPSProc *p, uintptr_t start, size_t size)
{
    // TODO: lock
    return lps_box_munmap(p->box, start, size);
}

int
proc_chdir(struct LPSProc *p, const char *path)
{
    // TODO: path check
    if (cwk_path_is_absolute(path)) {
        // lock
        // check
        size_t len = strnlen(path, sizeof(p->cwd.path) - 1);
        memcpy(&p->cwd.path[0], path, len);
        p->cwd.path[len] = 0;
    } else {
        if (p->cwd.path[0] == 0)
            return -LINUX_ENOENT;
        
        char joined[FILENAME_MAX];
        // lock
        cwk_path_join(p->cwd.path, path, joined, sizeof(joined));
        // check
        strncpy(p->cwd.path, joined, sizeof(joined));
        p->cwd.path[sizeof(p->cwd.path) - 1] = 0;
    }
    return 0;
}