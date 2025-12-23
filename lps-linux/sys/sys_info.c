#include "sys/sys.h"

#include <sys/sysinfo.h>

int
sys_sysinfo(struct LPSThread *t, uintptr_t infop)
{
    struct SysInfo *info = bufhost(t, infop, sizeof(struct SysInfo),
        alignof(struct SysInfo));
    if (!info)
        return -LINUX_EINVAL;

    struct sysinfo kinfo;
    int r = sysinfo(&kinfo);
    if (r < 0)
        return host_err(errno);

    *info = (struct SysInfo) {
        .uptime = kinfo.uptime,
        .totalram = kinfo.totalram,
        .freeram = kinfo.freeram,
        .sharedram = kinfo.sharedram,
        .bufferram = kinfo.bufferram,
        .totalswap = kinfo.totalswap,
        .freeswap = kinfo.freeswap,
        .procs = kinfo.procs,
        .totalhigh = kinfo.totalhigh,
        .freehigh = kinfo.freehigh,
        .mem_unit = kinfo.mem_unit,
    };
    info->loads[0] = kinfo.loads[0];
    info->loads[1] = kinfo.loads[1];
    info->loads[2] = kinfo.loads[2];

    return 0;
}


#define UTSNAME_LENGTH 65
#define LINUX_VERSION  "4.5.0"

struct UTSName {
    char sysname[UTSNAME_LENGTH];
    char nodename[UTSNAME_LENGTH];
    char release[UTSNAME_LENGTH];
    char version[UTSNAME_LENGTH];
    char machine[UTSNAME_LENGTH];
};

int
sys_uname(struct LPSThread *t, uintptr_t bufp)
{
    struct UTSName *uts = bufhost(t, bufp, sizeof(struct UTSName),
        alignof(struct UTSName));
    if (!uts)
        return -LINUX_EINVAL;
    strcpy(uts->sysname, "Linux LPS");
    strcpy(uts->nodename, "lps-sandbox");
    strcpy(uts->release, LINUX_VERSION "-lps");
    strcpy(uts->version, "0.0.0-unknown");
    strcpy(uts->machine, "riscv64");
    return 0;
}


#define LINUX_RLIMIT_CPU     0
#define LINUX_RLIMIT_FSIZE   1
#define LINUX_RLIMIT_DATA    2
#define LINUX_RLIMIT_STACK   3
#define LINUX_RLIMIT_CORE    4
#define LINUX_RLIMIT_RSS     5
#define LINUX_RLIMIT_NPROC   6
#define LINUX_RLIMIT_NOFILE  7
#define LINUX_RLIMIT_MEMLOCK 8
#define LINUX_RLIMIT_AS      9

struct RLimit {
    uint64_t cur;
    uint64_t max;
};

int
sys_getrlimit(struct LPSThread *t, int resource, uintptr_t rlimp)
{
    struct RLimit *rlim = bufhost(t, rlimp, sizeof(struct RLimit),
        alignof(struct RLimit));
    if (!rlim)
        return -LINUX_EINVAL;

    switch (resource) {
    case LINUX_RLIMIT_STACK:
        rlim->max = t->stack_size;
        rlim->cur = t->stack_size;
        break;
    case LINUX_RLIMIT_AS:
        rlim->max = t->proc->boxinfo.size;
        rlim->cur = t->proc->boxinfo.size;
        break;
    case LINUX_RLIMIT_CORE:
        rlim->max = 0;
        rlim->cur = 0;
        break;
    case LINUX_RLIMIT_RSS:
        rlim->max = t->proc->boxinfo.size;
        rlim->cur = t->proc->boxinfo.size;
        break;
    case LINUX_RLIMIT_DATA:
        rlim->max = t->proc->boxinfo.size;
        rlim->cur = t->proc->boxinfo.size;
        break;
    case LINUX_RLIMIT_NOFILE:
        rlim->max = LINUX_NOFILE;
        rlim->cur = LINUX_NOFILE;
        break;
    default:
        return -LINUX_EINVAL;
    }

    return 0;
}