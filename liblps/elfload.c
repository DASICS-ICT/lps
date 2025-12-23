#include "elfload.h"
#include "buf.h"
#include "align.h"

#include <elf.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <assert.h>

// We do not load ELF files that have more than 64 program headers.
#define PHNUM_MAX 64
// We do not allow interpreter paths longer than 1024 bytes.
#define INTERP_MAX 1024
// We do not allow executable segments above 1GiB.
#define CODE_MAX (1UL * 1024 * 1024 * 1024)

// Convert ELF protection flags to LFI mmap protection flags.
static int
pflags(int prot)
{
    return ((prot & PF_R) ? PROT_READ : 0) |
        ((prot & PF_W) ? PROT_WRITE : 0) |
        ((prot & PF_X) ? PROT_EXEC : 0);
}

// Sanity-check the ELF header.
static bool
elf_check(Elf64_Ehdr *ehdr)
{
    return memcmp(ehdr->e_ident, ELFMAG, SELFMAG) == 0 &&
        ehdr->e_ident[EI_CLASS] == ELFCLASS64 && ehdr->e_version == EV_CURRENT;
}

static bool
buf_read_elfseg(struct LPSProc *proc, uintptr_t start, uintptr_t offset,
    uintptr_t end, size_t p_offset, size_t filesz, size_t memsz, int prot,
    struct Buf buf, size_t pagesize, size_t p_align)
{
    struct LPSBox *box = proc->box;
    uintptr_t p;

    if (buf.fd != -1) {
        p = lps_box_mapat(box, start, end - start, prot, 
            MAP_PRIVATE, buf.fd, truncp(p_offset, p_align));
        if (p == (uintptr_t) -1) {
            return false;
        }
        if (memsz > filesz) {
            if ((prot & PROT_WRITE) == 0) {
                LOG(proc->engine, "ELF error: non-writable BSS segment");
                return false;
            }
            uintptr_t bss_start = start + offset + filesz;
            uintptr_t bss_rest = ceilp(bss_start, pagesize);
            memset((void *) bss_start, 0, bss_rest - bss_start);
            if (bss_rest < end) {
                p = lps_box_mapat(box, bss_rest, end - bss_rest, prot,
                    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
            }
        }


    } else {
        p = lps_box_mapat(box, start, end - start, PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        
        if (p == (uintptr_t) -1) {
            return false;
        }

        ssize_t n = buf_read(buf, (void *) (start + offset), filesz, p_offset);
        if (n != (ssize_t) filesz) {
            return false;
        }

        if (lps_box_mprotect(box, start, end - start, prot) < 0) {
            return false;
        }
        
    }

    assert(proc->seginfo.len < ELFSEGMAXN);
    proc->seginfo.elfsegs[proc->seginfo.len++] = (struct ElfSeg) {
        .start = start,
        .end = end,
        .prot = prot,
    };
    
    return true;
}

// Load a single in-memory ELF image into the address space.
static bool
elf_load_one(struct LPSProc *proc, struct Buf elf, uint64_t base,
    uintptr_t *p_first, uintptr_t *p_last, uintptr_t *p_entry, Elf64_Ehdr *ehdr)
{
    size_t pagesize = 4 * 1024;

    size_t n = buf_read(elf, ehdr, sizeof(*ehdr), 0);
    if (n != sizeof(*ehdr)) {
        ERROR("elf_load error: buf_read ehdr failed");
        return false;
    }

    if (!elf_check(ehdr)) {
        ERROR("elf_load error: elf_check failed");
        return false;
    }

    // In theory we could support fully static executables if not using
    // stores_only, since the LFI instrumentation is like a poor man's
    // position-independence. However, there is basically no reason to do this
    // instead of static-pie and supporting it adds some complexity. As a
    // result, we require binaries to be position-independent (ET_DYN).
    if (ehdr->e_type != ET_DYN) {
        ERROR(
            "elf_load error: elf binary is not PIE (did you compile with -static-pie?)");
        return false;
    }

    if (ehdr->e_phnum > PHNUM_MAX) {
        ERROR("elf_load error: e_phnum (%d) is too large", ehdr->e_phnum);
        return false;
    }

    Elf64_Phdr *phdrs = malloc(sizeof(Elf64_Phdr) * ehdr->e_phnum);
    if (!phdrs) {
        ERROR("elf_load error: out of memory");
        return false;
    }

    if (ehdr->e_phoff >= elf.size) {
        ERROR("elf_load error: e_phoff (%ld) is too large",
            (long) ehdr->e_phoff);
        goto err1;
    }

    n = buf_read(elf, phdrs, sizeof(*phdrs) * ehdr->e_phnum, ehdr->e_phoff);
    if (n != sizeof(*phdrs) * ehdr->e_phnum) {
        ERROR("elf_load error: buf_read phdrs failed");
        goto err1;
    }

    if (ehdr->e_entry >= CODE_MAX) {
        ERROR("elf_load error: e_entry (0x%lx) is larger than CODE_MAX (0x%lx)",
            (unsigned long) ehdr->e_entry, CODE_MAX);
        goto err1;
    }

    uintptr_t first = 0;
    uintptr_t last = 0;
    uintptr_t elfbase = base;
    uintptr_t laststart = -1;

    for (int i = 0; i < ehdr->e_phnum; i++) {
        Elf64_Phdr *p = &phdrs[i];
        if (p->p_type != PT_LOAD)
            continue;
        if (p->p_memsz == 0)
            continue;

        if (p->p_align % pagesize != 0) {
            ERROR(
                "elf_load error: invalid p_align (%ld) not a multiple of pagesize (%ld)",
                (long) p->p_align, pagesize);
            goto err1;
        }

        uintptr_t start = truncp(p->p_vaddr, p->p_align);
        uintptr_t end = ceilp(p->p_vaddr + p->p_memsz, p->p_align);
        uintptr_t offset = p->p_vaddr - start;

        if (p->p_memsz < p->p_filesz) {
            ERROR("elf_load error: p_memsz (0x%lx) < p_filesz (0x%lx)",
                (unsigned long) p->p_memsz, (unsigned long) p->p_filesz);
            goto err1;
        }
        if (end <= start || start >= CODE_MAX || end >= CODE_MAX) {
            ERROR(
                "elf_load error: segment start (0x%lx) or end (0x%lx) is invalid",
                start, end);
            goto err1;
        }

        if (first == 0 || elfbase + start < first)
            first = elfbase + start;
        if (start == laststart)
            LOG(proc->engine,
                "warning: current segment will overwrite previous segment");

        laststart = start;

        LOG(proc->engine, "elf_load [0x%lx, 0x%lx] (P: %d)", base + start,
            base + end, pflags(p->p_flags));
    
        if (!buf_read_elfseg(proc, base + start, offset, base + end,
                    p->p_offset, p->p_filesz, p->p_memsz, pflags(p->p_flags),
                    elf, pagesize, p->p_align)) {
            ERROR("elf_load error: reading elf segment failed");
            goto err1;
        }
        

        if (base == 0)
            base = base + start;
        if (base + end > last)
            last = base + end;
    }

    *p_last = last;
    *p_entry = base + ehdr->e_entry;
    *p_first = first;

    free(phdrs);

    return true;

err1:
    free(phdrs);
    return false;
}

bool
elf_load(struct LPSProc *proc, const char *prog_path, int prog_fd,
    const uint8_t *prog_data, size_t prog_size, const char *interp_path,
    int interp_fd, const uint8_t *interp_data, size_t interp_size, struct ELFLoadInfo *info)
{
    struct Buf prog = (struct Buf) {
        .fd = prog_fd,
        .data = prog_data,
        .size = prog_size,
    };

    struct Buf interp = (struct Buf) {
        .fd = interp_fd,
        .data = interp_data,
        .size = interp_size,
    };

    uintptr_t base = proc->boxinfo.base;
    uintptr_t p_first, p_last, p_entry, i_first, i_last, i_entry;
    bool has_interp = interp.data != NULL;
    Elf64_Ehdr p_ehdr, i_ehdr;

    if (!elf_load_one(proc, prog, base, &p_first, &p_last, &p_entry, &p_ehdr))
        goto err;
    if (has_interp) {
        if (!elf_load_one(proc, interp, p_last, &i_first, &i_last, &i_entry, &i_ehdr))
            goto err;
    }

    *info = (struct ELFLoadInfo) {
        .lastva = has_interp ? i_last : p_last,
        .elfentry = p_entry,
        .ldentry = has_interp ? i_entry : 0,
        .elfbase = p_first,
        .ldbase = has_interp ? i_first : p_first,
        .elfphoff = p_ehdr.e_phoff,
        .elfphnum = p_ehdr.e_phnum,
        .elfphentsize = p_ehdr.e_phentsize,
    };

    return true;

err:
    return false;
}