#pragma once
#include "lps_core.h"

#ifdef __cplusplus
extern "C" {
#endif

struct LPSLinuxOpts {
    bool verbose;
};

struct LPSLinuxEngine;

struct LPSProc;

struct LPSThread;

struct LPSLinuxEngine * 
lps_linux_new(struct LPSEngine *engine, struct LPSLinuxOpts opts);

struct LPSProc * 
lps_proc_new(struct LPSLinuxEngine *engine);

bool 
lps_proc_load(struct LPSProc *proc, const uint8_t *prog, size_t prog_size,
    const char *prog_path);

struct LPSThread * 
lps_thread_new(struct LPSProc *proc, int argc, const char **argv,
    const char **envp);

void
lps_thread_run(struct LPSThread *t);

#ifdef __cplusplus
}
#endif