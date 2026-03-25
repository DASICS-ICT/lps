# LPS 下一步计划

目前已完成：
- microbenchmark：pipe、syscall（getpid）、yield
- macrobenchmark：SPEC CPU2006 INT（GEOMEAN 开销 ~0.75% vs native）

---

## 零、运行须知

**所有 LPS 应用（`lps-go`、`lps-bench`、`lps-startup`、`lps-chain`、`lps-pipe`、`lps-preempt` 等）在命令行末尾必须加 `-dasics`**，供 OS 识别该进程需要启用 DASICS 扩展，否则沙箱无法正常运行。

```bash
# 正确用法示例
lps-go bin/wc -dasics
lps-bench 100 2 bin/workload -dasics
lps-startup bin/exit_imm -dasics
lps-chain bin/fc_source bin/fc_transform bin/fc_sink -dasics
```

---

## 一、补充 Microbenchmark

### 1.1 多沙箱并发测试（~1000 实例）

**目标**：验证系统可以同时存在 1000 个 LPS 沙箱并正常运行完毕（功能正确性为主，而非吞吐量）。

**实现位置**：`apps/lps-bench/main.c`（新建）

**思路**：
- 扩展 `lps-sched` 的方案，使沙箱数量 N 可参数化（100/500/1000）。
- **时间测量在代码内部使用 `rdcycle` 硬件计数器**，分别记录"创建阶段"和"执行阶段"耗时，输出 cycles 和毫秒，精度高于 `time` 命令且无进程启动 overhead。
- 被测沙箱程序做极少量计算后退出，降低总体运行时间，重点验证地址空间隔离的可行性。
- 关键验证：N=1000 时所有沙箱均正常退出，无地址冲突或 DASICS 越界。

**沙箱程序**（`microbenchmark/workload.c`，新增）：
```c
// microbenchmark/workload.c
// 固定工作量：少量斐波那契计算后退出，重点是验证沙箱能正确运行
#include <stdint.h>

static uint64_t fib(uint64_t n) {
    uint64_t a = 0, b = 1;
    for (uint64_t i = 0; i < n; i++) { uint64_t c = a + b; a = b; b = c; }
    return a;
}

int main(void) {
    volatile uint64_t r = fib(1000);   // 少量计算，缩短总测试时间
    (void)r;
    return 0;
}
```
编译：`riscv64-linux-musl-gcc -static-pie -o bin/workload workload.c`

**Runner 代码**（`apps/lps-bench/main.c`，新建）：
```c
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include "lps_linux.h"

static inline uint64_t rdcycle(void) {
    uint64_t c; asm volatile("rdcycle %0" : "=r"(c)); return c;
}
#define FPGA_HZ 50000000ULL
static double cy2ms(uint64_t cy) { return (double)cy * 1e3 / FPGA_HZ; }
static size_t mb(size_t x) { return x * 1024 * 1024; }

static struct LPSThread *
gen_thread(struct LPSLinuxEngine *eng, int fd, const char *elf)
{
    struct LPSProc *proc = lps_proc_new(eng);
    assert(proc);
    assert(lps_proc_load_fd(proc, fd, elf));
    const char *env = NULL;
    return lps_thread_new(proc, 1, (const char *[]){elf, NULL}, &env);
}

int main(int argc, char **argv)
{
    int n = atoi(argv[1]), ncore = atoi(argv[2]);
    const char *elf = argv[3];

    struct LPSEngine *engine = lps_new(
        (struct LPSOptions){ .boxsize = mb(64), .verbose = false }, n + 4);
    struct LPSLinuxEngine *x_engine = lps_linux_new(engine,
        (struct LPSLinuxOpts){ .brksize = mb(4), .stacksize = mb(1) });

    mcsched_init(ncore);

    uint64_t t0 = rdcycle();
    FILE *f = fopen(elf, "r");
    for (int i = 0; i < n; i++)
        mcsched_add(gen_thread(x_engine, fileno(f), elf));
    fclose(f);
    uint64_t t1 = rdcycle();

    mcshed_start();
    mcsched_join();
    uint64_t t2 = rdcycle();

    fprintf(stderr, "[lps] N=%d ncore=%d\n", n, ncore);
    fprintf(stderr, "  create: %llu cycles  %.1f ms\n",
            (unsigned long long)(t1-t0), cy2ms(t1-t0));
    fprintf(stderr, "  run:    %llu cycles  %.1f ms\n",
            (unsigned long long)(t2-t1), cy2ms(t2-t1));
    fprintf(stderr, "  total:  %llu cycles  %.1f ms\n",
            (unsigned long long)(t2-t0), cy2ms(t2-t0));
    return 0;
}
```

**meson.build 追加**（`apps/meson.build`）：
```meson
subdir('lps-bench')
```
`apps/lps-bench/meson.build`：
```meson
executable('lps-bench', 'main.c',
  dependencies: [lps_linux],
  link_args: app_link_args,
  install: true)
```

**Native 对照程序**（`microbenchmark/native_concurrent.c`，新增）：

Native 等价方案：fork N 个子进程，每个做相同工作量后退出，父进程 wait 全部完成。
与 LPS 方案的关键差异：OS 为每个子进程创建独立地址空间（MMU 切换），LPS 则在同一地址空间用 DASICS 隔离。

```c
// microbenchmark/native_concurrent.c
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/wait.h>

static inline uint64_t rdcycle(void) {
    uint64_t c; asm volatile("rdcycle %0" : "=r"(c)); return c;
}
#define FPGA_HZ 50000000ULL
static double cy2ms(uint64_t cy) { return (double)cy * 1e3 / FPGA_HZ; }

int main(int argc, char **argv)
{
    int n = atoi(argv[1]);
    char *elf = argv[2];
    char *child_argv[] = { elf, NULL };

    uint64_t t0 = rdcycle();
    for (int i = 0; i < n; i++) {
        pid_t p = fork();
        if (p < 0) { perror("fork"); exit(1); }
        if (p == 0) { execv(elf, child_argv); _exit(1); }
    }
    uint64_t t1 = rdcycle();

    for (int i = 0; i < n; i++) wait(NULL);
    uint64_t t2 = rdcycle();

    fprintf(stderr, "[native] N=%d\n", n);
    fprintf(stderr, "  fork:  %llu cycles  %.1f ms\n",
            (unsigned long long)(t1-t0), cy2ms(t1-t0));
    fprintf(stderr, "  run:   %llu cycles  %.1f ms\n",
            (unsigned long long)(t2-t1), cy2ms(t2-t1));
    fprintf(stderr, "  total: %llu cycles  %.1f ms\n",
            (unsigned long long)(t2-t0), cy2ms(t2-t0));
    return 0;
}
```
编译：`riscv64-linux-musl-gcc -static -o bin/native_concurrent native_concurrent.c`

**测试脚本**（`microbenchmark/scripts/concurrent-test.sh`）：
```bash
#!/bin/sh
# 时间由程序内部用 rdcycle 测量，脚本不使用 time
ELF=bin/workload

for N in 100 500 1000; do
    echo "=== N=$N ==="
    echo "[native] fork+exec $N OS processes:"
    bin/native_concurrent $N $ELF
    echo "[lps]    $N LPS sandboxes:"
    lps-bench $N 2 $ELF
    echo ""
done
```

**对比意义**：native 的时间包含 fork+exec 的 OS 进程创建开销（内存 COW 复制、MMU 刷新等），LPS 的时间包含 ELF 重新加载 + BoxMap 分配 + DASICS CSR 初始化。两者均分 create/run 两阶段输出，方便对应比较：
- `fork` ↔ `create`（地址空间建立）
- `run`（等待子进程完成）↔ `run`（mcsched 执行完毕）

---

### 1.2 启动延迟测试

**目标**：拆解 LPS 沙箱创建全流程的各阶段耗时，量化启动 overhead。

**拆解阶段**：
| 阶段 | 测量点 |
|------|-------|
| ① 虚拟地址分配 | `lps_proc_new()` 前后 |
| ② ELF 加载 | `lps_proc_load_file()` 前后 |
| ③ 线程初始化+栈布局 | `lps_thread_new()` 前后 |
| ④ 首次进入沙箱（含 DASICS CSR 写入）| `lps_thread_run()` 前后 |

**实现**（`apps/lps-startup/main.c`，新建）：

```c
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <sys/mman.h>
#include "lps_linux.h"

// rdcycle：RISC-V 硬件 cycle 计数器，比 clock_gettime 精度更高
static inline uint64_t rdcycle(void) {
    uint64_t c;
    asm volatile("rdcycle %0" : "=r"(c));
    return c;
}

#define FPGA_HZ 50000000ULL
static double cy2us(uint64_t cy) {
    return (double)cy * 1e6 / FPGA_HZ;
}

static size_t mb(size_t x) { return x * 1024 * 1024; }

int main(int argc, char **argv)
{
    if (argc < 2) { fprintf(stderr, "Usage: %s <elf>\n", argv[0]); return 1; }
    const char *elf = argv[1];

    // 预热引擎（排除 boxmap 初始化的一次性开销）
    struct LPSEngine *engine = lps_new((struct LPSOptions){
        .boxsize = mb(64), .verbose = false,
    }, 16);
    assert(engine);
    struct LPSLinuxEngine *x_engine = lps_linux_new(engine,
        (struct LPSLinuxOpts){ .brksize = mb(8), .stacksize = mb(1) });
    assert(x_engine);

    const int ROUNDS = 20;
    uint64_t sum_proc = 0, sum_load = 0, sum_thread = 0, sum_run = 0;

    for (int i = 0; i < ROUNDS; i++) {
        uint64_t t0 = rdcycle();
        struct LPSProc *proc = lps_proc_new(x_engine);
        uint64_t t1 = rdcycle();
        assert(lps_proc_load_file(proc, elf));
        uint64_t t2 = rdcycle();
        const char *env = NULL;
        struct LPSThread *t = lps_thread_new(proc, 1,
            (const char *[]){elf, NULL}, &env);
        uint64_t t3 = rdcycle();
        lps_thread_run(t);   // 沙箱程序立即 exit(0)
        uint64_t t4 = rdcycle();

        sum_proc   += t1 - t0;
        sum_load   += t2 - t1;
        sum_thread += t3 - t2;
        sum_run    += t4 - t3;
    }

    printf("Startup latency (avg over %d runs, FPGA@%lluMHz):\n", ROUNDS, FPGA_HZ/1000000);
    printf("  proc_new :  %.1f μs  (%llu cycles)\n", cy2us(sum_proc/ROUNDS),   sum_proc/ROUNDS);
    printf("  elf_load :  %.1f μs  (%llu cycles)\n", cy2us(sum_load/ROUNDS),   sum_load/ROUNDS);
    printf("  thread_new: %.1f μs  (%llu cycles)\n", cy2us(sum_thread/ROUNDS), sum_thread/ROUNDS);
    printf("  first_run:  %.1f μs  (%llu cycles)\n", cy2us(sum_run/ROUNDS),    sum_run/ROUNDS);
    printf("  TOTAL:      %.1f μs\n",
        cy2us((sum_proc+sum_load+sum_thread+sum_run)/ROUNDS));
    return 0;
}
```

**被测沙箱程序**（`microbenchmark/exit_imm.c`，新建）：
```c
// 尽可能快退出，用于测量纯启动 overhead
int main(void) { return 0; }
```
编译：`riscv64-linux-musl-gcc -static-pie -o bin/exit_imm exit_imm.c`

**Native 对照：fork+exec 延迟**（`microbenchmark/native_startup.c`，新增）：

Native 等价：测量 `fork() + execv()` 启动一个最简程序的完整延迟，用 `rdcycle` 以同等精度采样。对应 LPS 的"进入沙箱运行首条指令"的总延迟。

```c
// microbenchmark/native_startup.c
#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/wait.h>
#include <assert.h>

static inline uint64_t rdcycle(void) {
    uint64_t c; asm volatile("rdcycle %0" : "=r"(c)); return c;
}
#define FPGA_HZ 50000000ULL
static double cy2us(uint64_t cy) { return (double)cy * 1e6 / FPGA_HZ; }

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <exit_imm_binary>\n", argv[0]);
        return 1;
    }

    const int ROUNDS = 20;
    uint64_t sum_fork = 0, sum_exec = 0, sum_total = 0;

    for (int i = 0; i < ROUNDS; i++) {
        uint64_t t0 = rdcycle();
        pid_t p = fork();
        uint64_t t1 = rdcycle();
        if (p == 0) {
            // 子进程：exec 最简程序（立即 exit(0)）
            char *args[] = { argv[1], NULL };
            execv(argv[1], args);
            _exit(1);
        }
        assert(p > 0);
        int status;
        waitpid(p, &status, 0);
        uint64_t t2 = rdcycle();

        sum_fork  += t1 - t0;
        sum_total += t2 - t0;
    }

    printf("Native fork+exec latency (avg over %d runs, FPGA@%lluMHz):\n",
           ROUNDS, FPGA_HZ / 1000000);
    printf("  fork():        %.1f μs  (%llu cycles)\n",
           cy2us(sum_fork / ROUNDS), sum_fork / ROUNDS);
    printf("  fork+exec+run: %.1f μs  (%llu cycles)\n",
           cy2us(sum_total / ROUNDS), sum_total / ROUNDS);
    return 0;
}
```
编译：`riscv64-linux-musl-gcc -static -o bin/native_startup native_startup.c`

**测试脚本**（`microbenchmark/scripts/startup-test.sh`）：
```bash
#!/bin/bash
echo "=== LPS sandbox startup latency ==="
lps-startup bin/exit_imm

echo ""
echo "=== Native fork+exec latency ==="
bin/native_startup bin/exit_imm
```

**对比意义**：

| 阶段 | LPS 对应 | Native 对应 |
|------|---------|------------|
| "地址空间创建" | `lps_proc_new`（BoxMap 分配） | `fork()`（COW 复制页表） |
| "程序加载" | `lps_proc_load_file`（ELF 重新加载） | `execv()`（内核加载 ELF） |
| "栈初始化" | `lps_thread_new` | execv 内部完成 |
| "首次执行" | `lps_thread_run`（DASICS CSR 写入 + URET） | fork 后子进程调度延迟 |

LPS 预期：加载阶段因每次都需重新解析 ELF 而略慢于 exec；但无 fork COW 和 MMU 刷新，整体可能持平或更快。

---

### 1.3 多核可行性验证（1 核 vs 2 核）

**目标**：受限于 FPGA 硬件，仅对比 1 核与 2 核的执行时间，验证多核调度功能可行性（能否正确加速），而非做完整的扩展性分析。

**当前问题**：`mcsched_init()` 中 `mcsched_ncore` 硬编码为 2，需参数化。

**需修改**：`lps-linux/sched.c` 和 `lps-linux/include/lps_linux.h`。

修改 `mcsched_init` 签名（同时修复 work-stealing bug，见 3.2 节）：
```c
// sched.c：接受 ncore 参数
void mcsched_init(int ncore) {
    mcsched_ncore = (ncore > 0 && ncore <= MAX_CORES) ? ncore : 2;
    // ... 其余不变
}
```

Runner 侧通过命令行参数控制核数（在 `lps-bench/main.c` 已有的基础上加一个参数）：
```c
// Usage: lps-bench <N> <ncore> <elf>
int n     = atoi(argv[1]);
int ncore = atoi(argv[2]);
const char *elf = argv[3];
// ...
mcsched_init(ncore);
```

**Native 对照程序**（`microbenchmark/native_multicore.c`，新增）：

Native 等价：用 pthreads 运行 N 个相同任务，分别绑定 1 个核和 2 个核，测量总时间。

```c
// microbenchmark/native_multicore.c
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <sched.h>
#include <assert.h>
#include <stdint.h>

static uint64_t fib(uint64_t n) {
    uint64_t a = 0, b = 1;
    for (uint64_t i = 0; i < n; i++) { uint64_t c = a + b; a = b; b = c; }
    return a;
}

static atomic_int g_counter;  // 全局任务计数器（work-stealing 风格）
static int g_total;

static void *worker(void *arg)
{
    int core_id = (int)(intptr_t)arg;

    // 绑核
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset);

    while (1) {
        int idx = atomic_fetch_add_explicit(&g_counter, 1, memory_order_relaxed);
        if (idx >= g_total) break;
        volatile uint64_t r = fib(1000);
        (void)r;
    }
    return NULL;
}

int main(int argc, char **argv)
{
    int n     = argc > 1 ? atoi(argv[1]) : 100;
    int ncore = argc > 2 ? atoi(argv[2]) : 2;

    atomic_init(&g_counter, 0);
    g_total = n;

    pthread_t threads[ncore];
    for (int i = 0; i < ncore; i++)
        pthread_create(&threads[i], NULL, worker, (void *)(intptr_t)i);
    for (int i = 0; i < ncore; i++)
        pthread_join(threads[i], NULL);

    fprintf(stderr, "[native] n=%d ncore=%d finished\n", n, ncore);
    return 0;
}
```
编译：`riscv64-linux-musl-gcc -static -o bin/native_multicore native_multicore.c -lpthread`

**测试脚本**（`microbenchmark/scripts/multicore-test.sh`）：
```bash
#!/bin/bash
# FPGA 平台只有 2 核，验证 1 核 vs 2 核的加速效果
ELF=../bin/workload
N=100     # FPGA 上适当减小 N，加快测试速度

echo "=== Native (pthreads) ==="
echo "[native] 1 thread:"
time bin/native_multicore $N 1
echo "[native] 2 threads (pinned):"
time bin/native_multicore $N 2

echo ""
echo "=== LPS (mcsched) ==="
echo "[lps] 1 core:"
time lps-bench $N 1 $ELF
echo "[lps] 2 cores:"
time lps-bench $N 2 $ELF
```

**对比意义**：native pthreads 的加速比接近理想值 2×（共享内存，无 context switch 开销），LPS mcsched 的加速比会因跨核沙箱切换（`kswitch` + `lps_load_dasics` 的 38 个 `csrw`）而略低。两者加速比之差体现了 LPS 多核调度的额外开销。

---

## 二、补充应用测试

### 2.1 wordcount —— 基础 I/O 验证（优先级最高）

**目标**：验证文件读取、标准 I/O 的完整路径，作为后续复杂应用的基线。

**被测程序**（`microbenchmark/wc.c`，新增）：
```c
// 极简 wc：统计 stdin 的行/字/字节数
#include <stdio.h>
int main(void) {
    long lines = 0, words = 0, bytes = 0;
    int c, in_word = 0;
    while ((c = getchar()) != EOF) {
        bytes++;
        if (c == '\n') lines++;
        if (c == ' ' || c == '\t' || c == '\n') in_word = 0;
        else if (!in_word) { in_word = 1; words++; }
    }
    printf("%ld %ld %ld\n", lines, words, bytes);
    return 0;
}
```
编译：`riscv64-linux-musl-gcc -static-pie -o bin/wc wc.c`

**运行验证**：
```bash
echo "hello world" | lps-go bin/wc   # 期望输出: 1 2 12
cat /etc/passwd | lps-go bin/wc      # 与宿主 wc 对比
```

**涉及 syscall**：`read`（stdin）、`write`（stdout）——均已实现，预期开箱即用。

若要测文件输入（`wc < file`），需验证 `openat` + `read` 路径，同样已实现。

---

### 2.2 dav1d —— 汇编兼容性验证

**目标**：验证 RISC-V 汇编优化代码（含间接跳转、函数指针调用）在 DASICS 沙箱下正确运行，直接以启用汇编的版本进行测试。

dav1d 是纯 C + RISC-V 汇编的 AV1 解码器，汇编函数通过函数指针调用，是测试 DASICS 跳转边界兼容性的理想目标。

#### 编译（启用汇编）

```bash
git clone https://code.videolan.org/videolan/dav1d.git
cd dav1d
meson setup build \
  --cross-file riscv64-linux-gcc.ini \
  -Denable_asm=true \
  -Ddefault_library=static \
  -Dlogging=false \
  --buildtype=release
ninja -C build
# 产出：build/src/dav1d（静态 PIE 可执行文件）
```

#### 运行与调试

```bash
# 基础运行
lps-go build/src/dav1d -- -i sample.ivf -o /dev/null --threads=1
```

**跳转边界兼容性分析**：
dav1d 的汇编函数（如 RISC-V V 扩展的解码器核心）与 .text 段同处一个 ELF PT_LOAD segment。当前 `jmpbound[0]` 覆盖整个 Box，因此函数指针调用**不应触发 DFR_JF**。

若出现跳转越界（`DFR_JF`），在 `lps-linux/sys.c` 中添加调试输出定位问题：
```c
// arch_syshandle() 中，DFR_JF 分支
case DFR_JF:
    fprintf(stderr, "[DASICS] jump fault: from=0x%lx, target=0x%lx, box=[0x%lx, 0x%lx]\n",
            regs->uepc, regs->utval,
            lps_box_info(t->proc->box).base,
            lps_box_info(t->proc->box).base + lps_box_info(t->proc->box).size);
    assert(0);
```

**可能遇到的缺失 syscall**（按出现可能性排序）：
| syscall | 处理方式 |
|---------|---------|
| `mmap` with fd | 已实现 |
| `fstat`/`newfstatat` | 已实现 |
| `openat` | 已实现 |
| `prctl(PR_SET_NAME)` | 已实现 |
| `sched_getaffinity` | 已 nosys，单线程 OK |
| `memfd_create` | 添加 `sys_nosys` |
| `pread64` | 已实现 |

若遇到 `unknown syscall: X`，查找 syscall 号后在 `sys.c` 追加一行：
```c
SYS(name, sys_nosys(t, "name"))   // 暂时忽略
// 或者实现完整版本
```

#### 性能对比

```bash
# native（宿主直接运行）
time dav1d -i big_buck_bunny_480p.ivf -o /dev/null --threads=1

# lps（沙箱内运行）
time lps-go dav1d -- -i big_buck_bunny_480p.ivf -o /dev/null --threads=1
```

与 SPEC06 结果（0.75% overhead）相比，dav1d 含大量汇编热点，overhead 理论上应更低（DASICS 边界检查对纯计算负载几乎无影响）。

---

### 2.3 Function Chain —— 隔离管道计算测试

**目标**：构建一个完整的多沙箱函数调用链测试，验证 LPS pipe IPC 的正确性与性能，并与等价的 native Unix pipe 对比。

**场景设计**（3 级流水线）：
```
[source] ──pipe──▶ [transform] ──pipe──▶ [sink]
  产生 N 字节数据     对每字节做变换        消费并统计
```

每个阶段运行在独立沙箱中，通过 `lps_pipe_new` 连接，RR 调度器协调执行。

#### 沙箱程序（3 个，共用 cycle.h 计时）

`microbenchmark/fc_source.c`（数据产生者，写 fd=4）：
```c
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include "cycle.h"

#define CHUNK  4096
#define TOTAL  (512 * 1024)   // 512KB，适配 FPGA 速度

int main(void) {
    char buf[CHUNK];
    for (int i = 0; i < CHUNK; i++) buf[i] = (char)(i & 0xFF);

    uint64_t t0 = get_cycle_count();
    ssize_t sent = 0;
    while (sent < TOTAL) {
        ssize_t n = write(4, buf, CHUNK);
        if (n <= 0) break;
        sent += n;
    }
    uint64_t t1 = get_cycle_count();

    double ms = cycle_to_time_ns(t1 - t0, FPGA_HZ) / 1e6;
    fprintf(stderr, "[source] sent=%zd bytes, time=%.1f ms, bw=%.1f KB/s\n",
            sent, ms, sent / ms);
    return 0;
}
```

`microbenchmark/fc_transform.c`（处理者，读 fd=3，写 fd=4）：
```c
#include <unistd.h>
#define CHUNK 4096
int main(void) {
    char buf[CHUNK];
    ssize_t n;
    while ((n = read(3, buf, CHUNK)) > 0) {
        // 对每字节做可验证的变换：XOR 0xAB
        for (ssize_t i = 0; i < n; i++) buf[i] ^= 0xAB;
        if (write(4, buf, n) != n) break;
    }
    return 0;
}
```

`microbenchmark/fc_sink.c`（消费者，读 fd=3，校验并统计）：
```c
#include <unistd.h>
#include <stdio.h>
#define CHUNK 4096
int main(void) {
    char buf[CHUNK];
    ssize_t total = 0;
    long chksum = 0;
    ssize_t n;
    while ((n = read(3, buf, CHUNK)) > 0) {
        for (ssize_t i = 0; i < n; i++) chksum += (unsigned char)buf[i];
        total += n;
    }
    // 打印总量和校验和，用于与 native 结果比对
    printf("[sink] received=%zd bytes, checksum=%ld\n", total, chksum);
    return 0;
}
```

编译：
```bash
CC=riscv64-linux-musl-gcc
$CC -static-pie -I. -o bin/fc_source    fc_source.c
$CC -static-pie       -o bin/fc_transform fc_transform.c
$CC -static-pie       -o bin/fc_sink     fc_sink.c
```

#### Runner（`apps/lps-chain/main.c`，新建）

```c
#include "lps_linux.h"
#include <assert.h>
#include <stdio.h>

static size_t mb(size_t x) { return x * 1024 * 1024; }

// 创建进程并加载 ELF
static struct LPSProc *
make_proc(struct LPSLinuxEngine *eng, const char *elf)
{
    struct LPSProc *p = lps_proc_new(eng);
    assert(p && lps_proc_load_file(p, elf));
    return p;
}

// 创建线程
static struct LPSThread *
make_thread(struct LPSProc *p, const char *elf)
{
    const char *env = NULL;
    return lps_thread_new(p, 1, (const char *[]){elf, NULL}, &env);
}

int main(int argc, char **argv)
{
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <source.elf> <transform.elf> <sink.elf>\n", argv[0]);
        return 1;
    }

    struct LPSEngine *engine = lps_new(
        (struct LPSOptions){ .boxsize = mb(64), .verbose = false }, 6);
    assert(engine);

    struct LPSLinuxEngine *x_engine = lps_linux_new(engine,
        (struct LPSLinuxOpts){ .brksize = mb(4), .stacksize = mb(1) });
    assert(x_engine);

    // 三个独立沙箱
    struct LPSProc *src_p = make_proc(x_engine, argv[1]);
    struct LPSProc *xfm_p = make_proc(x_engine, argv[2]);
    struct LPSProc *snk_p = make_proc(x_engine, argv[3]);

    // 建立管道连接：
    //   src.fd4(write) ──▶ xfm.fd3(read)
    //   xfm.fd4(write) ──▶ snk.fd3(read)
    assert(lps_pipe_new(src_p, 4, xfm_p, 3));
    assert(lps_pipe_new(xfm_p, 4, snk_p, 3));

    // 添加到 RR 调度器
    rrschedadd(make_thread(src_p, argv[1]));
    rrschedadd(make_thread(xfm_p, argv[2]));
    rrschedadd(make_thread(snk_p, argv[3]));

    rrschedstart(false);
    return 0;
}
```

`apps/lps-chain/meson.build`：
```meson
executable('lps-chain', 'main.c',
  dependencies: [lps_linux],
  link_args: app_link_args,
  install: true)
```

#### Pipe 缓冲区扩容（必要修改）

当前 `PIPESZ = 1024` 会导致 source 和 transform 频繁阻塞切换。将缓冲区从栈分配改为堆分配，扩大至 64KB：

```c
// lps-linux/sys/sys_pipe.c
#define PIPESZ (64 * 1024)
#define NPIPE  16

struct Pipe {
    char  *data;       // 改为指针（堆分配）
    bool   writeopen, readopen, allocated;
    size_t nread, nwrite;
    struct Queue readq, writeq;
};

// pipe_new() 中增加分配：
pipe->data = malloc(PIPESZ);
assert(pipe->data);
```

#### Native 对比方案

在宿主上用 Unix pipe 连接三个等价程序，测量相同数据量的处理时间作为基准：

```bash
# native_source.c / native_transform.c / native_sink.c
# 与沙箱程序相同逻辑，但直接读写 stdin/stdout（fd=0/1）

gcc -O2 -o native_source    fc_source_native.c
gcc -O2 -o native_transform fc_transform_native.c
gcc -O2 -o native_sink      fc_sink_native.c
```

`microbenchmark/scripts/chain-test.sh`：
```bash
#!/bin/bash
echo "=== Native Unix pipe chain ==="
time (./native_source | ./native_transform | ./native_sink)

echo ""
echo "=== LPS function chain ==="
time lps-chain bin/fc_source bin/fc_transform bin/fc_sink
```

**校验正确性**：两者的 `checksum` 输出应完全一致（相同数据量、相同 XOR 变换）。

**关键指标**：IPC 带宽（KB/s）和总延迟，分析 LPS pipe 相比 Unix pipe 的 overhead 来源（调度切换次数 × 每次切换开销）。

---

## 三、Microbenchmark Native 程序编译汇总

所有 native 对照程序均在 RISC-V 宿主上直接编译运行，在 `microbenchmark/Makefile` 中追加以下目标：

```makefile
# 追加到 microbenchmark/Makefile
NATIVE_SRC = native_concurrent.c native_startup.c native_multicore.c
NATIVE_BIN = $(patsubst %.c,bin/%, $(NATIVE_SRC))

native: $(NATIVE_BIN)

bin/native_concurrent: native_concurrent.c
	$(CC) -static -O2 -o $@ $<

bin/native_startup: native_startup.c
	$(CC) -static -O2 -o $@ $<

bin/native_multicore: native_multicore.c
	$(CC) -static -O2 -o $@ $< -lpthread

# LPS 沙箱程序（static-pie）
bin/workload: workload.c
	$(CC) -static-pie -O2 -o $@ $<

bin/exit_imm: exit_imm.c
	$(CC) -static-pie -O2 -o $@ $<
```

---



在进行上述测试前，建议先修复以下问题：

### 3.1 字段名拼写错误

`lps-core/include/lps_arch.h` 中 `ucasue` 应为 `ucause`（影响可读性，但功能正确，因汇编偏移一致）：
```c
// lps_arch.h
uint64_t ucause;   // 原: ucasue
```
同步修改 `arch_asm.h` 中 `OFFSET_UCAUSE` 及 `sys.c` 中 `regs->ucasue`。

### 3.2 Work-stealing 循环 Bug

`sched.c` 中多核 steal 循环缺少 `break`，导致只 steal 到最后一个 core 的任务：
```c
// sched.c mcsched_loop() 中修复：
for (int i = 0; i < mcsched_ncore; ++i) {
    if (i == my_sched->core_id) continue;
    e = ring_pop_back(mcsched[i]->runq);
    if (e != NULL) {
        // ...
        break;  // ← 加上这一行
    }
}
```

### 3.3 mcsched_init() 参数化

将 `mcsched_ncore = 2` 改为接受参数，供多核测试使用（见 1.3 节）。

---

## 四、优先级排序

| 优先级 | 任务 | 难度 | 说明 |
|--------|------|------|------|
| P0 | Bug 修复（3.1-3.3） | 低 | 前置条件，影响后续所有测试 |
| P1 | wordcount 验证（2.1） | 低 | 最快验证基础 I/O，预计直接通过 |
| P1 | 启动延迟 benchmark（1.2） | 低 | 新增 runner + exit_imm 程序即可 |
| P2 | 多沙箱并发测试（1.1） | 低 | 重点验证 1000 沙箱共存功能可行性 |
| P2 | 多核可行性验证（1.3） | 中 | 仅测 1 vs 2 核，需参数化 mcsched_init |
| P3 | dav1d 汇编兼容性（2.2） | 中 | 直接启用 ASM，补全缺失 syscall |
| P3 | Function chain 完整测试（2.3） | 中 | 需先扩大 pipe 缓冲区，提供 native 对比 |
