#include "lps_linux.h"
#include "linux.h"

#include <stdlib.h>

struct LPSLinuxEngine * 
lps_linux_new(struct LPSEngine *lps_engine, struct LPSLinuxOpts opts)
{
    struct LPSLinuxEngine *engine = malloc(sizeof(struct LPSLinuxEngine));
    if (!engine) {
        return NULL;
    }

    // lps_sys_handler()

    *engine = (struct LPSLinuxEngine) {
        .engine = lps_engine,
        .opts = opts,
    };

    LOG(engine, "initialized LPS Linux engine");

    return engine;
}
