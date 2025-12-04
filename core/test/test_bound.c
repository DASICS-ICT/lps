#include "lps_arch.h"
#include "lps_core.h"

#include <assert.h>
#include <stdio.h>

int main() {
    struct LPSEngine *engine = lps_new((struct LPSOptions) {
        .boxsize = 4UL * 1024 * 1024 * 1024,
        .verbose = true,
    }, 10);

    struct LPSBox *box = lps_box_new(engine);

    struct LPSContext *ctx = lps_ctx_new(box, NULL);

    lps_membound_set(ctx, 0, LIBCFG_R | LIBCFG_W, 0x1000UL, 0x1fffUL);
    lps_membound_set(ctx, 1, LIBCFG_R, 0x10000UL, 0x1ffffUL);
    lps_jmpbound_set(ctx, 0, 0x100000UL, 0x1fffffUL);

    struct LPSRegs *regs = lps_ctx_regs(ctx);

    assert(regs->membound[0][0] == 0x1000UL);
    assert(regs->membound[0][1] == 0x1fffUL);
    assert(regs->membound[1][0] == 0x10000UL);
    assert(regs->membound[1][1] == 0x1ffffUL);
    assert(regs->memcfg == 0xab);

    assert(regs->jmpbound[0][0] == 0x100000UL);
    assert(regs->jmpbound[0][1] == 0x1fffffUL);
    assert(regs->jmpcfg == 0x1);

    lps_membound_clear(ctx, 0);

    assert(regs->memcfg == 0xa0);

    lps_membound_clear(ctx, 1);

    assert(regs->memcfg == 0x0);

    lps_jmpbound_clear(ctx, 0);

    assert(regs->jmpcfg == 0x0);

    return 0;
}