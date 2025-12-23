#pragma once


#define SGR_ERR      "\033[1;91m"  // Bold red.
#define SGR_WARN     "\033[0;33m"  // Yellow.
#define SGR_WARN_DBG "\033[1;33m"  // Bold yellow.
#define SGR_INFO     "\033[0;96m"  // Cyan.
#define SGR_DEBUG    "\033[1;32m"  // Bold green.
#define SGR_RESET    "\033[0m"

#define LOG_TAG "lps-linux"

#ifdef LOG_DISABLE

#define LOG(engine, ...)

#else

#define LOG(engine, ...)            \
    do {                            \
        if ((*engine).opts.verbose) \
            LOG_(__VA_ARGS__);      \
    } while (0)

#endif

#define LOG_(fmt, ...) fprintf(stderr, "[" LOG_TAG "] " fmt "\n", ##__VA_ARGS__)

#define ERROR(fmt, ...) \
    fprintf(stderr, "[" LOG_TAG "] " fmt "\n", ##__VA_ARGS__)


#define NO_DEBUG

#ifdef NO_DEBUG
#define DBG(fmt, ...) do { } while (0)
#else
#define DBG(fmt, ...)                                                                   \
    do {                                                                                \
        fprintf(stderr, SGR_DEBUG "[" LOG_TAG "] " fmt SGR_RESET "\n", ##__VA_ARGS__);  \
    } while (0)
#endif
