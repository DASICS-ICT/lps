#pragma once

#define LOG_TAG "lps-core"

#ifdef LOG_DISABLE

#define LOG(engine, ...)

#else

#define LOG(engine, ...)            \
    do {                            \
        if ((*engine).opts.verbose) \
            LOG_(__VA_ARGS__);      \
    } while (0)

#endif

#define LOG_(fmt, ...) \
    fprintf(stderr, "[" LOG_TAG "] " fmt "\n", ##__VA_ARGS__);

#define ERROR(fmt, ...) \
    fprintf(stderr, "[" LOG_TAG "] " fmt "\n", ##__VA_ARGS__)

