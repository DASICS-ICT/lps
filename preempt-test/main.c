#include <stdio.h>
#include <argp.h>

#include "lfi_tux.h"
#include "lfi.h"

typedef struct {
    uint8_t* data;
    size_t size;
} buf_t;

buf_t bufreadfile(struct Tux* tux, const char* filename);

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

    struct LFIPlatform* plat = lfi_new_plat((struct LFIPlatOptions) {
        .pagesize = kb(4),
        .vmsize = gb(4),
        .poc = false,
    });

    struct Tux* tux = lfi_tux_new(plat, (struct TuxOptions) {
        .pagesize = kb(4),
        .verbose = true,
        .stacksize = mb(2),
        .utimer = args.utimer,
    });

    for (int i = 0; i < args.ninputs; ++i) {
        buf_t elf = bufreadfile(tux, args.inputs[i]);

        if (!elf.data) {
            fprintf(stderr, "error openning: %s\n", args.inputs[i]);
            return 1;
        }

        struct TuxThread* p = lfi_tux_proc_new(tux, elf.data, elf.size, 0, NULL);

        scheduler_add_task(p);
    }

    scheduler_begin();
    
    return 0;
}