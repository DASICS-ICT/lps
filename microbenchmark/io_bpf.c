// microbenchmark/io_seccomp_bpf.c
//
// Sequential cached-read benchmark for comparing native Linux I/O with
// software seccomp-bpf based syscall interposition.
//
// Modes:
//   native  - read the file directly.
//   filter  - install an allow-only seccomp-bpf I/O filter; reads still execute
//             in the kernel, but every syscall pays BPF evaluation cost.
//   seccomp - use seccomp user notification to trap read(2) and let an
//             unfiltered helper thread perform the real read on behalf of the
//             benchmark thread. This models software syscall instrumentation.

#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <linux/audit.h>
#include <linux/filter.h>
#include <linux/seccomp.h>
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

#include "cycle.h"

#ifndef EM_RISCV
#define EM_RISCV 243
#endif

#ifndef AUDIT_ARCH_64BIT
#define AUDIT_ARCH_64BIT 0x80000000U
#endif

#ifndef AUDIT_ARCH_LE
#define AUDIT_ARCH_LE 0x40000000U
#endif

#ifndef AUDIT_ARCH_RISCV64
#define AUDIT_ARCH_RISCV64 (AUDIT_ARCH_64BIT | AUDIT_ARCH_LE | EM_RISCV)
#endif

#ifndef SECCOMP_RET_KILL_PROCESS
#define SECCOMP_RET_KILL_PROCESS SECCOMP_RET_KILL
#endif

#ifndef SECCOMP_RET_USER_NOTIF
#define SECCOMP_RET_USER_NOTIF 0x7fc00000U
#endif

#ifndef SECCOMP_FILTER_FLAG_NEW_LISTENER
#define SECCOMP_FILTER_FLAG_NEW_LISTENER (1UL << 3)
#endif

#define EXPECTED_AUDIT_ARCH AUDIT_ARCH_RISCV64
#define DEFAULT_TOTAL_BYTES (1024ULL * 1024ULL * 1024ULL)
#define DEFAULT_BUFFER_BYTES 4096ULL
#define DEFAULT_ROUNDS 5

#define BPF_ALLOW_NR(nr)                                                       \
    BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, (nr), 0, 1),                           \
    BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW)

enum bench_mode {
    MODE_NATIVE,
    MODE_FILTER,
    MODE_NOTIFY,
};

struct bench_config {
    enum bench_mode mode;
    const char *path;
    uint64_t total_bytes;
    size_t buffer_bytes;
    int rounds;
};

struct round_result {
    uint64_t bytes;
    uint64_t read_calls;
    uint64_t cycles;
    uint64_t sample;
};

struct notify_state {
    pthread_mutex_t lock;
    pthread_cond_t cond;
    int listener_fd;
    bool stop;
};

static volatile uint64_t sample_sink;

static const char *
mode_name(enum bench_mode mode)
{
    switch (mode) {
    case MODE_NATIVE:
        return "native";
    case MODE_FILTER:
        return "seccomp-filter";
    case MODE_NOTIFY:
        return "seccomp-bpf-notify";
    default:
        return "unknown";
    }
}

static void
usage(const char *argv0)
{
    fprintf(stderr,
            "Usage: %s [native|filter|seccomp] [options] <file>\n"
            "\n"
            "Options:\n"
            "  -b, --buffer SIZE   read buffer size, e.g. 4K, 64K, 256K\n"
            "  -s, --size SIZE     bytes to read per round, default 1G; 0 means EOF\n"
            "  -r, --rounds N      repeat count, default 5; best round is reported\n"
            "\n"
            "Examples:\n"
            "  %s native  --buffer 4K --size 1G test.dat\n"
            "  %s seccomp --buffer 4K --size 1G test.dat\n",
            argv0, argv0, argv0);
}

static int
parse_size(const char *s, uint64_t *out)
{
    char *end = NULL;
    errno = 0;
    unsigned long long value = strtoull(s, &end, 0);
    if (errno != 0 || end == s)
        return -1;

    uint64_t multiplier = 1;
    if (*end != '\0') {
        if (end[1] != '\0')
            return -1;
        switch (*end) {
        case 'k':
        case 'K':
            multiplier = 1024ULL;
            break;
        case 'm':
        case 'M':
            multiplier = 1024ULL * 1024ULL;
            break;
        case 'g':
        case 'G':
            multiplier = 1024ULL * 1024ULL * 1024ULL;
            break;
        default:
            return -1;
        }
    }

    if (value > UINT64_MAX / multiplier)
        return -1;
    *out = (uint64_t)value * multiplier;
    return 0;
}

static int
parse_mode(const char *s, enum bench_mode *mode)
{
    if (strcmp(s, "native") == 0) {
        *mode = MODE_NATIVE;
        return 0;
    }
    if (strcmp(s, "filter") == 0 || strcmp(s, "seccomp-filter") == 0) {
        *mode = MODE_FILTER;
        return 0;
    }
    if (strcmp(s, "seccomp") == 0 || strcmp(s, "notify") == 0 ||
        strcmp(s, "seccomp-bpf") == 0) {
        *mode = MODE_NOTIFY;
        return 0;
    }
    return -1;
}

static int
parse_args(int argc, char **argv, struct bench_config *cfg)
{
    cfg->mode = MODE_NATIVE;
    cfg->path = NULL;
    cfg->total_bytes = DEFAULT_TOTAL_BYTES;
    cfg->buffer_bytes = DEFAULT_BUFFER_BYTES;
    cfg->rounds = DEFAULT_ROUNDS;

    int i = 1;
    if (i < argc && argv[i][0] != '-') {
        enum bench_mode mode;
        if (parse_mode(argv[i], &mode) == 0) {
            cfg->mode = mode;
            i++;
        }
    }

    while (i < argc) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            usage(argv[0]);
            exit(0);
        } else if (strcmp(argv[i], "-b") == 0 ||
                   strcmp(argv[i], "--buffer") == 0) {
            if (++i >= argc)
                return -1;
            uint64_t parsed;
            if (parse_size(argv[i], &parsed) < 0 || parsed == 0 ||
                parsed > (uint64_t)SIZE_MAX)
                return -1;
            cfg->buffer_bytes = (size_t)parsed;
        } else if (strcmp(argv[i], "-s") == 0 ||
                   strcmp(argv[i], "--size") == 0) {
            if (++i >= argc)
                return -1;
            if (parse_size(argv[i], &cfg->total_bytes) < 0)
                return -1;
        } else if (strcmp(argv[i], "-r") == 0 ||
                   strcmp(argv[i], "--rounds") == 0) {
            if (++i >= argc)
                return -1;
            char *end = NULL;
            long rounds = strtol(argv[i], &end, 10);
            if (*end != '\0' || rounds <= 0 || rounds > 100000)
                return -1;
            cfg->rounds = (int)rounds;
        } else if (argv[i][0] == '-') {
            return -1;
        } else {
            if (cfg->path != NULL)
                return -1;
            cfg->path = argv[i];
        }
        i++;
    }

    return cfg->path == NULL ? -1 : 0;
}

static int
set_no_new_privs(void)
{
    if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) < 0) {
        perror("prctl(PR_SET_NO_NEW_PRIVS)");
        return -1;
    }
    return 0;
}

static int
install_filter_only(void)
{
    struct sock_filter filter[] = {
        BPF_STMT(BPF_LD | BPF_W | BPF_ABS,
                 (uint32_t)offsetof(struct seccomp_data, arch)),
        BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, EXPECTED_AUDIT_ARCH, 1, 0),
        BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_KILL_PROCESS),
        BPF_STMT(BPF_LD | BPF_W | BPF_ABS,
                 (uint32_t)offsetof(struct seccomp_data, nr)),
#ifdef SYS_openat
        BPF_ALLOW_NR(SYS_openat),
#endif
#ifdef SYS_close
        BPF_ALLOW_NR(SYS_close),
#endif
#ifdef SYS_lseek
        BPF_ALLOW_NR(SYS_lseek),
#endif
#ifdef SYS_fcntl
        BPF_ALLOW_NR(SYS_fcntl),
#endif
#ifdef SYS_newfstatat
        BPF_ALLOW_NR(SYS_newfstatat),
#endif
#ifdef SYS_readv
        BPF_ALLOW_NR(SYS_readv),
#endif
#ifdef SYS_pread64
        BPF_ALLOW_NR(SYS_pread64),
#endif
#ifdef SYS_write
        BPF_ALLOW_NR(SYS_write),
#endif
#ifdef SYS_writev
        BPF_ALLOW_NR(SYS_writev),
#endif
        BPF_ALLOW_NR(SYS_read),
        BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),
    };
    struct sock_fprog prog = {
        .len = (unsigned short)(sizeof(filter) / sizeof(filter[0])),
        .filter = filter,
    };

    if (set_no_new_privs() < 0)
        return -1;
    if (prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &prog) < 0) {
        perror("prctl(PR_SET_SECCOMP, FILTER)");
        return -1;
    }
    return 0;
}

static int
install_notify_filter(void)
{
    struct sock_filter filter[] = {
        BPF_STMT(BPF_LD | BPF_W | BPF_ABS,
                 (uint32_t)offsetof(struct seccomp_data, arch)),
        BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, EXPECTED_AUDIT_ARCH, 1, 0),
        BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_KILL_PROCESS),
        BPF_STMT(BPF_LD | BPF_W | BPF_ABS,
                 (uint32_t)offsetof(struct seccomp_data, nr)),
        BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, SYS_read, 0, 1),
        BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_USER_NOTIF),
        BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),
    };
    struct sock_fprog prog = {
        .len = (unsigned short)(sizeof(filter) / sizeof(filter[0])),
        .filter = filter,
    };

    if (set_no_new_privs() < 0)
        return -1;

    int fd = (int)syscall(SYS_seccomp, SECCOMP_SET_MODE_FILTER,
                          SECCOMP_FILTER_FLAG_NEW_LISTENER, &prog);
    if (fd < 0) {
        perror("seccomp(SECCOMP_FILTER_FLAG_NEW_LISTENER)");
        return -1;
    }
    return fd;
}

static void
notify_state_init(struct notify_state *state)
{
    pthread_mutex_init(&state->lock, NULL);
    pthread_cond_init(&state->cond, NULL);
    state->listener_fd = -1;
    state->stop = false;
}

static void
notify_state_publish_fd(struct notify_state *state, int listener_fd)
{
    pthread_mutex_lock(&state->lock);
    state->listener_fd = listener_fd;
    pthread_cond_signal(&state->cond);
    pthread_mutex_unlock(&state->lock);
}

static void
notify_state_stop(struct notify_state *state)
{
    pthread_mutex_lock(&state->lock);
    state->stop = true;
    pthread_cond_signal(&state->cond);
    pthread_mutex_unlock(&state->lock);
}

static int
notification_recv(int listener_fd, struct seccomp_notif *req)
{
    for (;;) {
        memset(req, 0, sizeof(*req));
        if (ioctl(listener_fd, SECCOMP_IOCTL_NOTIF_RECV, req) == 0)
            return 0;
        if (errno == EINTR)
            continue;
        return -1;
    }
}

static int
notification_send(int listener_fd, struct seccomp_notif_resp *resp)
{
    for (;;) {
        if (ioctl(listener_fd, SECCOMP_IOCTL_NOTIF_SEND, resp) == 0)
            return 0;
        if (errno == EINTR)
            continue;
        if (errno == ENOENT)
            return 0;
        return -1;
    }
}

static void *
notification_thread(void *arg)
{
    struct notify_state *state = arg;

    pthread_mutex_lock(&state->lock);
    while (state->listener_fd < 0 && !state->stop)
        pthread_cond_wait(&state->cond, &state->lock);
    int listener_fd = state->listener_fd;
    bool stop = state->stop;
    pthread_mutex_unlock(&state->lock);

    if (stop)
        return NULL;

    for (;;) {
        struct seccomp_notif req;
        if (notification_recv(listener_fd, &req) < 0) {
            perror("SECCOMP_IOCTL_NOTIF_RECV");
            break;
        }

        struct seccomp_notif_resp resp;
        memset(&resp, 0, sizeof(resp));
        resp.id = req.id;

        if (req.data.nr == SYS_read) {
            int fd = (int)req.data.args[0];
            void *buf = (void *)(uintptr_t)req.data.args[1];
            size_t count = (size_t)req.data.args[2];
            long ret = syscall(SYS_read, fd, buf, count);
            if (ret < 0) {
                resp.val = 0;
                resp.error = -errno;
            } else {
                resp.val = ret;
                resp.error = 0;
            }
        } else {
            resp.val = 0;
            resp.error = -ENOSYS;
        }

        if (notification_send(listener_fd, &resp) < 0) {
            perror("SECCOMP_IOCTL_NOTIF_SEND");
            break;
        }
    }

    return NULL;
}

static int
read_loop(int fd, unsigned char *buf, size_t buffer_bytes,
          uint64_t total_limit, struct round_result *result)
{
    uint64_t total = 0;
    uint64_t calls = 0;
    uint64_t sample = 0;

    while (total_limit == 0 || total < total_limit) {
        size_t want = buffer_bytes;
        if (total_limit != 0 && total_limit - total < want)
            want = (size_t)(total_limit - total);

        ssize_t n;
        do {
            n = read(fd, buf, want);
        } while (n < 0 && errno == EINTR);
        calls++;

        if (n < 0) {
            perror("read");
            return -1;
        }
        if (n == 0)
            break;

        sample += buf[0];
        sample += buf[n - 1];
        total += (uint64_t)n;
    }

    result->bytes = total;
    result->read_calls = calls;
    result->sample = sample;
    return 0;
}

static int
run_round(int fd, unsigned char *buf, const struct bench_config *cfg,
          struct round_result *result)
{
    if (lseek(fd, 0, SEEK_SET) < 0) {
        perror("lseek");
        return -1;
    }

    uint64_t start = get_cycle_count();
    if (read_loop(fd, buf, cfg->buffer_bytes, cfg->total_bytes, result) < 0)
        return -1;
    uint64_t end = get_cycle_count();

    result->cycles = end - start;
    sample_sink += result->sample;
    return 0;
}

static void
print_result(const struct bench_config *cfg, const struct round_result *best)
{
    double seconds = (double)best->cycles / (double)FPGA_HZ;
    double ms = seconds * 1000.0;
    double mib = (double)best->bytes / (1024.0 * 1024.0);
    double mbps = seconds > 0.0 ? mib / seconds : 0.0;
    double cycles_per_read = best->read_calls > 0
                                 ? (double)best->cycles /
                                       (double)best->read_calls
                                 : 0.0;

    printf("Best result:\n");
    printf("  Mode:            %s\n", mode_name(cfg->mode));
    printf("  Buffer bytes:    %zu\n", cfg->buffer_bytes);
    printf("  Bytes read:      %" PRIu64 "\n", best->bytes);
    printf("  Read calls:      %" PRIu64 "\n", best->read_calls);
    printf("  Total cycles:    %" PRIu64 "\n", best->cycles);
    printf("  Time:            %.3f ms\n", ms);
    printf("  Throughput:      %.2f MB/s\n", mbps);
    printf("  Cycles/read:     %.2f\n", cycles_per_read);
    printf("CSV,%s,%zu,%" PRIu64 ",%" PRIu64 ",%" PRIu64
           ",%.3f,%.2f,%.2f\n",
           mode_name(cfg->mode), cfg->buffer_bytes, best->bytes,
           best->read_calls, best->cycles, ms, mbps, cycles_per_read);
}

int
main(int argc, char **argv)
{
    struct bench_config cfg;
    if (parse_args(argc, argv, &cfg) < 0) {
        usage(argv[0]);
        return 2;
    }

    void *raw_buf = NULL;
    int r = posix_memalign(&raw_buf, 4096, cfg.buffer_bytes);
    if (r != 0) {
        fprintf(stderr, "posix_memalign: %s\n", strerror(r));
        return 1;
    }
    unsigned char *buf = raw_buf;

    int fd = open(cfg.path, O_RDONLY);
    if (fd < 0) {
        perror("open");
        free(raw_buf);
        return 1;
    }

    struct stat st;
    if (fstat(fd, &st) == 0 && S_ISREG(st.st_mode) && cfg.total_bytes != 0 &&
        (uint64_t)st.st_size < cfg.total_bytes) {
        fprintf(stderr,
                "warning: file is smaller than requested size; reading to EOF "
                "(file=%" PRIu64 ", requested=%" PRIu64 ")\n",
                (uint64_t)st.st_size, cfg.total_bytes);
    }

    printf("[io-seccomp-bpf] mode=%s file=%s buffer=%zu total=%" PRIu64
           " rounds=%d\n",
           mode_name(cfg.mode), cfg.path, cfg.buffer_bytes, cfg.total_bytes,
           cfg.rounds);

    struct round_result warmup;
    if (run_round(fd, buf, &cfg, &warmup) < 0) {
        close(fd);
        free(raw_buf);
        return 1;
    }

    struct notify_state notify_state;
    pthread_t notify_thread;
    bool notify_thread_started = false;
    int listener_fd = -1;

    if (cfg.mode == MODE_FILTER) {
        if (install_filter_only() < 0) {
            close(fd);
            free(raw_buf);
            return 1;
        }
    } else if (cfg.mode == MODE_NOTIFY) {
        notify_state_init(&notify_state);
        r = pthread_create(&notify_thread, NULL, notification_thread,
                           &notify_state);
        if (r != 0) {
            fprintf(stderr, "pthread_create: %s\n", strerror(r));
            close(fd);
            free(raw_buf);
            return 1;
        }
        notify_thread_started = true;

        listener_fd = install_notify_filter();
        if (listener_fd < 0) {
            notify_state_stop(&notify_state);
            pthread_join(notify_thread, NULL);
            close(fd);
            free(raw_buf);
            return 1;
        }
        notify_state_publish_fd(&notify_state, listener_fd);
    }

    struct round_result best = {0};
    bool have_best = false;
    for (int i = 0; i < cfg.rounds; i++) {
        struct round_result current;
        if (run_round(fd, buf, &cfg, &current) < 0) {
            close(fd);
            free(raw_buf);
            return 1;
        }

        double ms = ((double)current.cycles / (double)FPGA_HZ) * 1000.0;
        printf("Round %d: bytes=%" PRIu64 " calls=%" PRIu64
               " cycles=%" PRIu64 " time=%.3f ms\n",
               i + 1, current.bytes, current.read_calls, current.cycles, ms);

        if (!have_best || current.cycles < best.cycles) {
            best = current;
            have_best = true;
        }
    }

    print_result(&cfg, &best);

    if (listener_fd >= 0)
        close(listener_fd);
    if (notify_thread_started)
        pthread_detach(notify_thread);
    close(fd);
    free(raw_buf);
    return 0;
}
