#include "lps_linux.h"
#include "linux.h"
#include "sys.h"

#include <stdlib.h>

struct LPSLinuxEngine * 
lps_linux_new(struct LPSEngine *lps_engine, struct LPSLinuxOpts opts)
{
    struct LPSLinuxEngine *engine = malloc(sizeof(struct LPSLinuxEngine));
    if (!engine) {
        return NULL;
    }

    lps_sys_handler(lps_engine, arch_syshandle);

    if (opts.brksize == 0)
        opts.brksize = BRKMAXSIZE;
    if (opts.stacksize == 0)
        opts.stacksize = STACKSIZE;

    *engine = (struct LPSLinuxEngine) {
        .engine = lps_engine,
        .opts = opts,
    };

    LOG(engine, "initialized LPS Linux engine");

    return engine;
}
