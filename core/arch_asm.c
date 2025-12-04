#include "lps_arch.h"
#include "arch_asm.h"

#include <stddef.h>

_Static_assert(offsetof(struct LPSRegs, host_sp) == OFFSET_KSP,
    "incorrect REGS offset");
_Static_assert(offsetof(struct LPSRegs, host_gp) == OFFSET_KGP,
    "incorrect REGS offset");
_Static_assert(offsetof(struct LPSRegs, host_tp) == OFFSET_KTP,
    "incorrect REGS offset");
_Static_assert(offsetof(struct LPSRegs, zero) == OFFSET_ZERO,
    "incorrect REGS offset");
_Static_assert(offsetof(struct LPSRegs, f) == OFFSET_F,
    "incorrect REGS offset");
_Static_assert(offsetof(struct LPSRegs, uepc) == OFFSET_UEPC,
    "incorrect REGS offset");
_Static_assert(offsetof(struct LPSRegs, utval) == OFFSET_UTVAL,
    "incorrect REGS offset");
_Static_assert(offsetof(struct LPSRegs, dfreason) == OFFSET_DFREN,
    "incorrect REGS offset");
_Static_assert(offsetof(struct LPSRegs, memcfg) == OFFSET_MEMCFG,
    "incorrect REGS offset");
_Static_assert(offsetof(struct LPSRegs, membound) == OFFSET_MEMBOUND0LO,
    "incorrect REGS offset");
_Static_assert(offsetof(struct LPSRegs, jmpcfg) == OFFSET_JMPCFG,
    "incorrect REGS offset");
_Static_assert(offsetof(struct LPSRegs, jmpbound) == OFFSET_JMPBOUND0LO,
    "incorrect REGS offset");

_Static_assert(offsetof(struct KRegs, ra) == CONTEXT_RA,
    "incorrect REGS offset");

_Static_assert(offsetof(struct KRegs, sp) == CONTEXT_SP,
    "incorrect REGS offset");

_Static_assert(offsetof(struct KRegs, s0) == CONTEXT_S0,
    "incorrect REGS offset");

_Static_assert(offsetof(struct KRegs, s11) == CONTEXT_S11,
    "incorrect REGS offset");