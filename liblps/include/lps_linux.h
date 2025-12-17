#pragma once
#include "lps_core.h"

#ifdef __cplusplus
extern "C" {
#endif

struct LPSLinuxOpts {
    bool verbose;

    bool passthrough;
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

bool
lps_proc_load_fd(struct LPSProc *proc, int fd, const char *prog_path);

bool
lps_proc_load_file(struct LPSProc *proc, const char *prog_path);

struct LPSThread * 
lps_thread_new(struct LPSProc *proc, int argc, const char **argv,
    const char **envp);

void
lps_thread_run(struct LPSThread *t);

void
lps_thread_exit(struct LPSThread *t);

// Scheduler functions
void
rrschedadd(struct LPSThread *t);

void
rrschedstart();

// fd assign functions
struct FDFile;

int
lps_proc_fd_assign(struct LPSProc *proc, int fd, struct FDFile *f);

#ifdef __cplusplus
}
#endif