#include "lps_linux.h"

#include <stdio.h>
#include <argp.h>
#include <sys/mman.h>
#include <assert.h>

static size_t
gb(size_t x)
{
    return x * 1024 * 1024 * 1024;
}

static size_t
mb(size_t x)
{
    return x * 1024 * 1024;
}

static size_t
kb(size_t x)
{
    return x * 1024;
}

struct Buf {
    void *data;
    size_t size;
};

static struct Buf
readfile(const char *path)
{
    FILE *f = fopen(path, "r");
    if (!f) {
        return (struct Buf) { 0 };
    }
    fseek(f, 0, SEEK_END);
    size_t sz = ftell(f);
    void *p = mmap(NULL, sz, PROT_READ, MAP_PRIVATE, fileno(f), 0);
    fclose(f);
    if (!p) {
        return (struct Buf) { 0 };
    }
    return (struct Buf) {
        .data = p,
        .size = sz,
    };
}

#define INPUTMAX 256

static char doc[] = "preempt test";

static char args_doc[] = "ELF0 ELF1 ...";

struct Args {
    char* inputs[INPUTMAX];
    size_t ninputs;
    bool utimer;
};

static struct argp_option options[] = {
    { "help",           'h',               0,      0, "show this message", -1 },
    { "utimer",         'u',               0,      0, "enable utimer", -1 },
    { 0 },
};

static error_t
parse_opt(int key, char* arg, struct argp_state* state) {
    struct Args* args = state->input;

    switch(key) {
        case 'h':
            argp_state_help(state, state->out_stream, ARGP_HELP_STD_HELP);
            break;
        case 'u':
            args->utimer = true;
            break;
        case ARGP_KEY_ARG:
            if (args->ninputs < INPUTMAX) {
                args->inputs[args->ninputs++] = arg;
            }
            break;
        default:
            return ARGP_ERR_UNKNOWN;
    }

    return 0;
}

static struct argp argp = { .options = options, .parser = parse_opt, .args_doc = args_doc, .doc = doc };

struct Args args;

int
main(int argc, char** argv)
{
    
    argp_parse(&argp, argc, argv, ARGP_NO_HELP | ARGP_IN_ORDER, 0, &args);

    if (args.ninputs <= 0) {
        fprintf(stderr, "no input file provided\n");
        return 1;
    }

    struct LPSEngine *engine = lps_new((struct LPSOptions) {
        .boxsize = gb(4),
        .verbose = true,
    }, 10);
    assert(engine);

    struct LPSLinuxEngine *x_engine = lps_linux_new(engine, (struct LPSLinuxOpts) {
        .verbose = true,
        .passthrough = false,
    });
    assert(x_engine);

    const char *envp = NULL;

    for (int i = 0; i < args.ninputs; ++i) {
        const char *path = args.inputs[i];

        struct Buf elf = readfile(path);

        if (!elf.data) {
            fprintf(stderr, "error openning: %s\n", path);
            return 1;
        }

        struct LPSProc *proc = lps_proc_new(x_engine);
        assert(proc);

        assert(lps_proc_load(proc, elf.data, elf.size, path));

        struct LPSThread *t = lps_thread_new(proc, 0, NULL, &envp);
        assert(t);

        rrschedadd(t);
    }

    rrschedstart(args.utimer);
    
    return 0;
}