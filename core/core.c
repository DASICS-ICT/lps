#include "core.h"
#include "lps_core.h"
#include "log.h"

#include <assert.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>


EXPORT struct LPSEngine *
lps_new(struct LPSOptions opts, size_t nsandboxes) {
    struct LPSEngine *engine = malloc(sizeof(struct LPSEngine));

    if (!engine) {
        return NULL;
    }

    struct BoxMap *bm = boxmap_new((struct BoxMapOptions) {
        .chunksize = opts.boxsize, 
        .guardsize = 0,
    });

    if (!bm) {
        goto err1;
    }

    size_t reserve = nsandboxes * opts.boxsize;
    if (!boxmap_reserve(bm, reserve)) {
        goto err2;
    }

    *engine = (struct LPSEngine) {
        .bm = bm,
        .opts = opts,
    };

    LOG(engine, "initialized LPS engine: %ld GiB",
        reserve / 1024 / 1024 / 1024);

    return engine;

err2:
    boxmap_delete(bm);
err1:
    free(engine);
    return NULL;
}

EXPORT void
lps_free(struct LPSEngine *engine) {
    boxmap_delete(engine->bm);
    free(engine);
}

EXPORT void
lps_sys_handler(struct LPSEngine *engine, void (*sys_handler)(struct LPSContext *ctx)) {
    engine->sys_handler = sys_handler;
}

void
lps_syscall_handler(struct LPSContext *ctx) __asm__("lps_syscall_handler");

void
lps_syscall_handler(struct LPSContext *ctx)
{
    assert(ctx->box->engine->sys_handler &&
        "engine does not have a system call handler");
    ctx->box->engine->sys_handler(ctx);
}
