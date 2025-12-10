#include "proc.h"
#include "fd.h"
#include "elfload.h"

#include <stdlib.h>

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

    


}

bool 
lps_proc_load(struct LPSProc *proc, const uint8_t *prog, size_t prog_size,
    const char *prog_path)
{
    
}