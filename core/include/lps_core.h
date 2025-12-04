#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct LPSEngine;

struct LPSBox;

struct LPSContext;

struct LPSOptions {
    size_t boxsize;

    bool verbose;
};

// Creates a new LPS engine and reserve enough virtual address space for 'n'
// sandboxes.
struct LPSEngine *
lps_new(struct LPSOptions opts, size_t nsandboxes);

// Frees the LPSEngine.
void
lps_free(struct LPSEngine *lps);

// Sets the system call handler for the given engine.
void
lps_sys_handler(struct LPSEngine *engine,
    void (*sys_handler)(struct LPSContext *ctx));

// Creates a new LPSBox by allocating a portion of the virtual address space.
struct LPSBox *
lps_box_new(struct LPSEngine *lps);

// Return the engine that this box belongs to.
struct LPSEngine *
lps_box_engine(struct LPSBox *box);

// Set this box's associated user data.
void
lps_box_setdata(struct LPSBox *box, void *userdata);

// Return this box's associated user data.
void *
lps_box_data(struct LPSBox *box);

// Returns information about the sandbox (base and size).
struct LPSBoxInfo
lps_box_info(struct LPSBox *box);

// Creates a new memory mapping in the sandbox at 'addr'. Returns -1 on
// failure. May run the verifier for executable pages. Disallows shared
// mappings to prevent double mapping pages as W and X.
uintptr_t
lps_box_mapat(struct LPSBox *box, uintptr_t addr, size_t size, int prot, int flags,
    int fd, off_t off);

// Creates a new memory mapping in the sandbox at an arbitrary location,
// similar to mapat.
uintptr_t
lps_box_mapany(struct LPSBox *box, size_t size, int prot, int flags, int fd,
    off_t off);

// Unmaps a memory mapping in the sandbox. Returns -1 on failure.
int
lps_box_munmap(struct LPSBox *box, uintptr_t addr, size_t size);

// Changes the protection of a memory mapping inside the sandbox. May run the
// verifier for executable pages.
int
lps_box_mprotect(struct LPSBox *box, uintptr_t addr, size_t size, int prot);

// Returns whether a pointer is valid within the given sandbox.
bool
lps_box_ptrvalid(struct LPSBox *box, uintptr_t addr);

// Returns whether a buffer is valid within the given sandbox.
bool
lps_box_bufvalid(struct LPSBox *box, uintptr_t addr, size_t size);

// Copies data out from the given sandbox location.
void *
lps_box_copyfm(struct LPSBox *box, void *dst, uintptr_t src, size_t size);

// Copies data in to the given sandbox location.
uintptr_t
lps_box_copyto(struct LPSBox *box, uintptr_t dst, const void *src, size_t size);

// Frees all resources associated with box and deallocates its reservation in
// the LPSEngine in which it was allocated.
void
lps_box_free(struct LPSBox *box);

// Creates a new LPSContext for a given sandbox, along with a user-provided
// data pointer.
struct LPSContext *
lps_ctx_new(struct LPSBox *box, void *userdata);

// Returns the ctxp user data pointer for ctx.
void *
lps_ctx_data(struct LPSContext *ctx);

// Begins executing the sandbox context.
int
lps_ctx_run(struct LPSContext *ctx, uintptr_t entry);

// Frees the sandbox context.
void
lps_ctx_free(struct LPSContext *ctx);

// Returns a pointer to the sandbox context's registers, which can be read and
// written.
struct LPSRegs *
lps_ctx_regs(struct LPSContext *ctx);

// Causes the sandbox context to exit with a given exit code.
void
lps_ctx_exit(struct LPSContext *ctx, int code);

// Returns the sandbox associated with the given context.
struct LPSBox *
lps_ctx_box(struct LPSContext *ctx);

// Return the currently active sandbox context.
struct LPSContext *
lps_cur_ctx(void);

void
lps_set_ctx(struct LPSContext *ctx);


// DASICS bound management
void 
lps_membound_set(struct LPSContext *ctx, int idx, int prot, uintptr_t low, uintptr_t high);

void 
lps_membound_clear(struct LPSContext *ctx, int idx);

void 
lps_jmpbound_set(struct LPSContext *ctx, int idx, uintptr_t low, uintptr_t high);

void 
lps_jmpbound_clear(struct LPSContext *ctx, int idx);


#ifdef __cplusplus
}
#endif