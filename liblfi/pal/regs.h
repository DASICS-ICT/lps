#include "lfi_arch.h"

void lfi_regs_init(struct TuxRegs* regs, struct LFIAddrSpace* as, struct LFIContext* ctx);

void lfi_ctx_init_sys(struct LFIContext* ctx);

uintptr_t* lfi_regs_entry(struct TuxRegs* regs);

uintptr_t* lfi_regs_arg0(struct TuxRegs* regs);

void kcontext_init(struct KContext* k_ctx, uintptr_t entry, uintptr_t sp, uintptr_t sp_base);
