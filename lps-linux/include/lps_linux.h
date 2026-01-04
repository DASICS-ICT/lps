#pragma once
#include "lps_core.h"

#ifdef __cplusplus
extern "C" {
#endif

struct LPSLinuxOpts {
    bool verbose;

    bool passthrough;

    size_t brksize;
    size_t stacksize;
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
rrschedstart(bool utimer);

void
mcsched_init();

int
mcsched_add(struct LPSThread *t);

void
mcshed_start();

void
mcsched_join();

// Create a Pipe from a proc to another, write fromfd and read tofd
bool
lps_pipe_new(struct LPSProc *from, int fromfd, struct LPSProc *to, int tofd);

#ifdef __cplusplus
}
#endif