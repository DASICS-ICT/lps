#include "pal/arch_asm.h"
#include "lfi_arch.h"

#include <stddef.h>

_Static_assert(offsetof(struct TuxRegs, host_sp) == OFFSET_KSP,
    "incorrect REGS offset");
_Static_assert(offsetof(struct TuxRegs, host_gp) == OFFSET_KGP,
    "incorrect REGS offset");
_Static_assert(offsetof(struct TuxRegs, host_tp) == OFFSET_KTP,
    "incorrect REGS offset");
_Static_assert(offsetof(struct TuxRegs, zero) == OFFSET_ZERO,
    "incorrect REGS offset");
_Static_assert(offsetof(struct TuxRegs, f) == OFFSET_F,
    "incorrect REGS offset");
_Static_assert(offsetof(struct TuxRegs, uepc) == OFFSET_UPEC,
    "incorrect REGS offset");
_Static_assert(offsetof(struct TuxRegs, utval) == OFFSET_UTVAL,
    "incorrect REGS offset");
_Static_assert(offsetof(struct TuxRegs, dfreason) == OFFSET_DFREN,
    "incorrect REGS offset");