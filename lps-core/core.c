#include "core.h"
#include "lps_core.h"
#include "log.h"
#include "ucsr.h"

#include <assert.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

extern void lps_init_dasics() __asm__ ("lps_init_dasics");

EXPORT struct LPSEngine *
lps_new(struct LPSOptions opts, size_t nsandboxes) {
    struct LPSEngine *engine = malloc(sizeof(struct LPSEngine));

    if (!engine) {
        return NULL;
    }

    struct BoxMap *bm = boxmap_new((struct BoxMapOptions) {
        .chunksize = opts.boxsize,
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

    // TODO: check thread-safety
    static int initialized = 0;
    if (!initialized) {
        lps_init_dasics();
        initialized = 1;
    }

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

EXPORT void
lps_utimer_init()
{
    csr_write(CSR_USTATUS,  0x10);
    csr_write(CSR_UIE, 0x111);
}