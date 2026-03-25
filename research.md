# LPS (Lightweight Process Sandbox) — 深度研究报告

> 作者：Claude Code 自动生成
> 日期：2026-02-26
> 仓库路径：`/home/zcy/coding/x-dasics/lfi`
> 当前分支：`x-dasics`

---

## 目录

1. [项目概述](#1-项目概述)
2. [背景：RISC-V DASICS 扩展](#2-背景risc-v-dasics-扩展)
3. [仓库结构总览](#3-仓库结构总览)
4. [lps-core：沙箱核心层](#4-lps-core沙箱核心层)
5. [lps-linux：Linux 兼容层](#5-lps-linuxlinux-兼容层)
6. [应用程序层（apps）](#6-应用程序层apps)
7. [完整执行流程](#7-完整执行流程)
8. [关键数据结构与关系](#8-关键数据结构与关系)
9. [安全模型与 DASICS 集成细节](#9-安全模型与-dasics-集成细节)
10. [调度器设计](#10-调度器设计)
11. [IPC 管道机制](#11-ipc-管道机制)
12. [构建系统](#12-构建系统)
13. [当前局限与 TODO](#13-当前局限与-todo)
14. [总结与关键发现](#14-总结与关键发现)

---

## 1. 项目概述

LPS（Lightweight Process Sandbox，轻量级进程沙箱）是一个基于 **RISC-V DASICS 硬件扩展**的用户态运行时系统，旨在**在同一宿主进程的地址空间内加载并隔离运行多个 ELF 可执行文件**。

核心设计理念：
- **同地址空间多实例**：所有沙箱共享同一个宿主进程的虚拟地址空间，通过虚拟地址分区（每个沙箱 box 独占一段大地址区间）实现隔离。
- **硬件强制隔离**：依赖 DASICS 扩展的 CSR 边界寄存器，由 CPU 硬件在每次访存和跳转时强制检查范围，无需软件插桩。
- **用户态异常处理**：DASICS 违规（包括系统调用触发）通过 RISC-V N 扩展（用户态中断）上报，跳转到注册的用户态异常处理向量 `dasics_ufault_entry`，运行时在用户态完成系统调用模拟，无需进入内核。
- **Linux 兼容接口**：通过 lps-linux 层模拟约 60+ 个 Linux 系统调用，使标准静态链接的 RISC-V ELF 程序无需修改即可运行。

---

## 2. 背景：RISC-V DASICS 扩展

DASICS（Dynamic Access Switch & Call Sandbox）是一个 RISC-V 轻量级隔离扩展，通过增加若干 CSR（控制状态寄存器）实现访存和跳转限制。

### 新增 CSR

**内存访问边界（16 组）：**
- `CSR_DLCFG`（0x880）：16 个边界的使能/权限配置，每个边界占 4 bit（V/R/W）。
- `CSR_DLBOUND{0-15}LO / HI`：各边界的低地址/高地址。

**跳转边界（4 组）：**
- `CSR_DJCFG`（0x8c8）：4 个跳转边界的使能配置，每个占 1 bit（V）。
- `CSR_DJBOUND{0-3}LO / HI`：各跳转边界的低/高地址。

**用户态异常相关：**
- `CSR_UTVEC`（0x005）：用户态异常向量地址。
- `CSR_UEPC`（0x041）：发生异常时的 PC。
- `CSR_UCAUSE`（0x042）：异常原因。
- `CSR_UTVAL`（0x043）：异常辅助值（如触发访存的地址）。
- `CSR_DFREASON`：DASICS 违规类型（EF=ecall fault / LF=load fault / SF=store fault / JF=jump fault）。
- `CSR_USCRATCH`（0x040）：暂存寄存器，LPS 用于存储当前 context 指针。

**用户态定时器：**
- `CSR_UTIMECMP`（0x045）：用户态定时器比较值，可触发 U_TIMER 中断。
- `CSR_USTATUS`（0x000）、`CSR_UIE`（0x004）：用户态中断使能。

### 工作原理

当 CPU 在用户态执行内存访问或跳转时，硬件自动对照已写入 CSR 的边界表进行检查：
- 若访问地址不在任何已使能的 `DLBOUND` 范围内（且有对应权限） → 触发 DASICS Load/Store Fault。
- 若跳转目标不在任何 `DJBOUND` 范围内 → 触发 DASICS Jump Fault。
- `ecall` 指令 → 触发 DASICS EF（ecall fault）。

所有违规均跳转至 `CSR_UTVEC` 所指向的异常处理函数（用户态），LPS 运行时在此处理系统调用请求。

---

## 3. 仓库结构总览

```
lfi/
├── lps-core/          # 沙箱核心库（平台无关层）
│   ├── arch_asm.h/c   # 寄存器偏移量定义与静态断言校验
│   ├── bound.c        # DASICS 内存/跳转边界管理 API
│   ├── box.c          # 沙箱 Box 生命周期管理
│   ├── boxmap.c/h     # 大块虚拟地址空间预留与分配
│   ├── core.c         # 引擎初始化、syscall 分发
│   ├── core.h         # 内部数据结构（LPSEngine/Box/Context）
│   ├── ctx.c          # Context 创建、切换（含 DASICS 加载）
│   ├── mmap.c/h       # Box 内存映射追踪器（linked list）
│   ├── runtime.S      # RISC-V 汇编：上下文切换、异常入口、DASICS 初始化
│   ├── ucsr.h         # RISC-V CSR 地址宏与内联读写函数
│   ├── log.h          # 日志宏
│   ├── include/
│   │   ├── lps_arch.h # 公开：LPSRegs/KRegs 结构体、配置位定义
│   │   └── lps_core.h # 公开：完整 Engine/Box/Context API
│   └── test/
│       └── test_bound.c  # DASICS 边界配置单元测试
│
├── lps-linux/         # Linux 兼容层（依赖 lps-core）
│   ├── linux.h        # LPSLinuxEngine 结构体、Linux 常量定义
│   ├── linux.c        # 引擎初始化
│   ├── proc.h/c       # 进程（LPSProc）管理
│   ├── thread.c       # 线程（LPSThread）创建、启动、退出、栈初始化
│   ├── elfload.h/c    # ELF 加载（静态 PIE）
│   ├── sched.c        # 调度器：RRSched（单核）+ MCSched（多核）
│   ├── fd.h/c         # 文件描述符表（FDTable，最大 1024）
│   ├── kfile.h/c      # 宿主文件 vtable（read/write/lseek...）
│   ├── host.h/c       # 宿主 syscall 封装（fstatat/getdents/getrandom）
│   ├── buf.h/c        # 文件 mmap buffer 工具
│   ├── queue.h/c      # 链表队列 + 无锁环形队列（work-stealing）
│   ├── list.h/c       # 双向链表
│   ├── lock.h         # pthread mutex RAII 封装
│   ├── cwalk.h/c      # 路径操作库（MIT）
│   ├── sys.h/c        # syscall 分发主入口（syshandle）
│   ├── sys/
│   │   ├── sys_mem.c     # brk/mmap/mprotect/munmap
│   │   ├── sys_file.c    # read/write/open/close/lseek/stat/getdents...
│   │   ├── sys_pipe.c    # pipe 实现（虚拟管道 + 调度阻塞）
│   │   ├── sys_proc.c    # exit/exit_group
│   │   ├── sys_thread.c  # gettid/set_tid_address
│   │   ├── sys_cwd.c     # chdir/fchdir/getcwd
│   │   ├── sys_time.c    # nanosleep/clock_gettime
│   │   ├── sys_rand.c    # getrandom
│   │   ├── sys_info.c    # uname/sysinfo/getrlimit
│   │   ├── sys_ioctl.c   # ioctl（TIOCGWINSZ）
│   │   ├── sys_ptrctl.c  # prctl（PR_SET_NAME）
│   │   ├── sys_passthrough.c # 直接透传 syscall 至宿主
│   │   ├── sys_rtcall.c  # runtime_call（YIELD 等内部调用）
│   │   └── sys_none.c    # sys_ignore / sys_nosys
│   ├── bound.h        # DASICS 边界槽位常量
│   ├── align.h        # 地址对齐宏
│   ├── arch_sys.h     # Linux syscall 号、RISC-V CSR 宏
│   ├── log.h          # 日志宏（彩色）
│   ├── psched.h       # 调度器前向声明
│   └── include/
│       └── lps_linux.h  # 公开 API
│
└── apps/              # 测试应用程序
    ├── ld.ld          # 自定义 linker script（base=0x1000000）
    ├── lps-go/        # 单进程运行器
    ├── lps-pipe/      # 双进程 IPC 测试
    ├── lps-preempt/   # 多进程抢占调度测试（可选 utimer）
    ├── lps-sched/     # 多核调度压力测试（100 并发）
    └── utimer-test/   # 用户态定时器中断测试
```

---

## 4. lps-core：沙箱核心层

### 4.1 BoxMap：虚拟地址空间预留与分配

**文件**：`boxmap.c / boxmap.h`

BoxMap 解决了"如何为多个沙箱分配隔离的虚拟地址区间"的问题。

**策略**：在进程初始化时，通过 `mmap(PROT_NONE)` 一次性预留大块连续虚拟地址（默认尝试 256TB，指数退半直至成功，最小 32GB），最多维护 16 个这样的 Region（`AddrRegion`）。每个 Region 内部用 `ExtAlloc` 位图按 `chunksize`（= 沙箱 box 大小，如 4GB）为单位管理分配。

```
BoxMap
├── regions[0]: AddrRegion [0x400000000000, +N*4GB] ← PROT_NONE 预留
│   └── ExtAlloc (bitvec): 已分配 Box 槽位
├── regions[1]: AddrRegion (如果第一个用完再扩展)
└── ...
```

分配时，`boxmap_addspace(map, size)` 在某个 Region 找到空闲 chunk，标记已用，返回起始虚拟地址。此地址即是新 Box 的基地址。

**优势**：避免了 `mmap` 随机分配导致不同沙箱地址重叠，也避免了每次从 OS 申请大量内存的开销——只是虚拟地址预留，物理页面按需 fault in。

### 4.2 Box：沙箱实例

**文件**：`box.c`，结构体在 `core.h` 中的 `LPSBox`

每个 `LPSBox` 代表一个独立沙箱，具有固定的虚拟地址区间 `[base, base+size)`。

```c
struct LPSBox {
    struct MMAddrSpace mm; // 内存映射追踪器
    uintptr_t base;        // Box 基地址
    size_t    size;        // Box 大小（如 4GB）
    void     *userdata;    // 关联到 LPSProc
    struct LPSEngine *engine;
};
```

关键 API：
- `lps_box_mapat(box, addr, size, prot, flags, fd, off)`：在 Box 内固定地址映射内存，使用 `MAP_FIXED` 的实际 `mmap`，同时更新 MM 追踪器。
- `lps_box_mapany(box, size, prot, flags, fd, off)`：在 Box 内任意位置找空闲映射。
- `lps_box_munmap / mprotect`：解映射和权限变更。
- `lps_box_ptrvalid(box, addr)`：检查指针是否在 Box 范围内（用于 syscall 参数校验）。

### 4.3 MMAddrSpace：内存映射追踪器

**文件**：`mmap.c / mmap.h`

由于 Box 内部的映射需要追踪（以支持 `mapany` 的空闲区域查找，以及 `mprotect` 等操作），LPS 维护了一个按地址排序的双向链表 `MMNode`，记录每个已映射区域的 `[base, base+len)`、权限、flags 等。

**核心难点**：`mm_unmap` 需要处理 5 种 overlap 情形（node 完全在范围内、node 包含范围、左重叠、右重叠、范围外），并进行节点分裂或合并。

`cursor` 字段是一个优化提示，记录上次分配的节点，使 `mapany` 的下一次分配从 cursor 附近开始扫描，避免总是从头遍历。

### 4.4 DASICS 边界管理

**文件**：`bound.c`，`lps_core/include/lps_arch.h`

`LPSRegs` 结构体中存储了完整的 DASICS 边界状态（作为 Context 的一部分保存/恢复）：

```c
struct LPSRegs {
    // ... 通用寄存器、浮点寄存器 ...
    uint64_t uepc, ucasue, utval, dfreason;  // 异常寄存器
    uint64_t memcfg;          // 16 个内存边界的配置位图（每边界 4bit：V/R/W）
    uint64_t membound[16][2]; // 16 对 [lo, hi] 内存访问边界
    uint64_t jmpcfg;          // 4 个跳转边界的配置位图（每边界 1bit：V）
    uint64_t jmpbound[4][2];  // 4 对 [lo, hi] 跳转边界
};
```

`lps_membound_set(ctx, idx, prot, low, high)`：
1. 存入 `ctx->regs.membound[idx]`。
2. 按 4bit 编码更新 `ctx->regs.memcfg`：`(prot | LIBCFG_V) << (idx * 4)`。

`lps_jmpbound_set(ctx, idx, low, high)`：类似，jmpcfg 每边界只有 1 bit（JMPCFG_V）。

**重要设计**：边界状态保存在 Context 的 `LPSRegs` 内存中，Context 切换时由 `lps_load_dasics`（汇编）批量写入全部 CSR，确保每个沙箱拥有独立的 DASICS 配置。

### 4.5 Context 与上下文切换

**文件**：`ctx.c`，`runtime.S`

```c
struct LPSContext {
    struct LPSRegs regs;   // 用户态寄存器（含 DASICS）
    struct KRegs kregs;    // 内核态 callee-saved 寄存器
    void *userdata;        // → LPSThread
    struct LPSBox *box;
};

struct KRegs {
    uint64_t ra, sp, s0-s11, sp_base;  // 运行时内核侧寄存器
};
```

**切换到沙箱（`lps_kswitch_to`）**：
```
lps_kswitch_to(ctx)
  → lps_myctx = ctx              // 设置线程局部当前 Context
  → lps_load_dasics(ctx)         // 批量写入 DASICS CSR（16*2 + 4*2 + 2 = 38 个 csrw）
  → kswitch(ctx, &lps_kctx, &ctx->kregs)  // 保存 kctx，加载 ctx->kregs，ret 跳至 lps_ctx_entry
  → [lps_ctx_entry]:
      sd sp/gp/tp → ctx->regs.host_sp/gp/tp  // 保存运行时 sp/gp/tp 至 regs
      RESTORE_ALL                              // 从 ctx->regs 恢复用户寄存器，URET 进入用户态
```

**异常处理（DASICS 违规 / ecall）**：
```
[用户态触发异常] → CPU 跳转至 CSR_UTVEC → dasics_ufault_entry
  → SAVE_ALL:
      csrrw s0, USCRATCH, s0   // s0 = &ctx->regs（USCRATCH 中预存了指针）
      保存所有通用寄存器到 ctx->regs
      csrr 保存 UEPC/UCAUSE/UTVAL/DFREASON
      ld sp/gp/tp 从 ctx->regs.host_sp/gp/tp（切回运行时栈）
  → jal lps_syscall_handler(s0)  // 调用 C 处理函数
  → RESTORE_ALL → URET           // 恢复用户态继续执行
```

**切换回运行时（`lps_kswitch_from`）**：
```
lps_kswitch_from(ctx)
  → kswitch(NULL, &ctx->kregs, &lps_kctx)  // 保存 ctx 内核侧状态，恢复运行时 kctx
  → 返回到 lps_thread_run() 调用点之后
```

`USCRATCH` 寄存器的作用：异常发生时 CPU 不自动保存通用寄存器，通过 `csrrw s0, USCRATCH, s0` 可以在 s0 中暂存 USCRATCH（context 指针），并用原 s0 值覆写 USCRATCH，稍后再通过 `csrr t0, USCRATCH` 取回原 s0，实现无额外内存的上下文获取。

---

## 5. lps-linux：Linux 兼容层

### 5.1 引擎与进程结构

**LPSLinuxEngine**（`linux.h / linux.c`）：
- 包含 `LPSEngine *engine`（lps-core 引擎）。
- `LPSLinuxOpts` 配置：`verbose`（日志）、`passthrough`（透传 syscall）、`brksize`（堆大小，默认 64MB）、`stacksize`（栈大小，默认 8MB）。

**LPSProc**（`proc.h / proc.c`）：对应一个沙箱进程。
```c
struct LPSProc {
    struct LPSLinuxEngine *engine;
    struct LPSBox   *box;           // 虚拟地址隔离单元
    struct LPSBoxInfo boxinfo;      // base + size
    struct FDTable   fdtable;       // 文件描述符表（1024）
    uintptr_t entry;                // ELF 入口地址
    uintptr_t brkbase, brksize;     // heap 基址与当前大小
    struct Dir cwd;                 // 当前工作目录
    struct ELFLoadInfo elfinfo;     // ELF 加载信息（auxv 用）
    struct ELFSegInfo seginfo;      // 已加载 ELF Segment 列表
    atomic_int total_thread_count;  // TID 计数器
};
```

**LPSThread**（`proc.h / thread.c`）：对应一个执行线程。
```c
struct LPSThread {
    struct LPSProc  *proc;
    struct LPSContext *ctx;   // lps-core 执行上下文
    int tid;
    uintptr_t stack;           // 栈起始地址
    size_t    stack_size;
    int state;                 // RUNNABLE / BLOCKED / EXITED
    struct List elem;          // 调度队列链表节点
};
```

### 5.2 ELF 加载

**文件**：`elfload.c`

LPS 要求 ELF 为 **静态 PIE**（`ET_DYN`，`-static-pie` 编译）。不支持标准动态链接器（`ld.so`），但 `elf_load()` 接口预留了 interpreter 参数（当前不使用）。

加载流程（`proc_load` → `elf_load` → `elf_load_one`）：

1. **校验 ELF header**：魔数、ELF64、`ET_DYN`、phnum ≤ 64。
2. **读取 Program Headers**，只处理 `PT_LOAD` 段：
   - 地址对齐到 `p_align`（通常 4KB）。
   - 调用 `lps_box_mapat` 在 Box 内固定位置映射（若有 fd 则文件映射，否则匿名映射）。
   - 处理 BSS：`memsz > filesz` 的部分清零并追加匿名映射。
3. **记录加载信息**：`p_first`（起始地址）、`p_last`（结束地址，作为 brk 起点）、`p_entry`（入口）。
4. **映射 brk 区域**：以 `PROT_NONE` 预留 `brksize`（默认 64MB）的堆空间，`sys_brk` 按需 remap 为 R/W。

### 5.3 线程栈初始化

**文件**：`thread.c` → `stack_init()`

栈内存在 Box 末尾映射（`end - stacksize` 处）。栈布局（从低到高）：

```
sp → | argc (8B) | argv[0..n] NULL | envp[0..m] NULL | auxv | 16B random | argv/envp strings |
                                                                             ← strs_start
```

Auxiliary Vector（auxv）包含约 18 项，涵盖：
- `AT_BASE`（ld 加载基址）、`AT_PHDR/PHNUM/PHENT`、`AT_ENTRY`
- `AT_EXECFN`（程序路径）、`AT_PAGESZ`（4096）
- `AT_HWCAP/HWCAP2`（从宿主透传）
- `AT_RANDOM`（16 字节随机数地址）
- `AT_UID/EUID/GID/EGID`（固定为 1000）
- `AT_SYSINFO/EHDR`（置 0，不支持 vDSO）

### 5.4 系统调用分发

**文件**：`sys.c` → `arch_syshandle()` → `syshandle()`

调用链：
```
DASICS ecall 异常 → dasics_ufault_entry(runtime.S)
  → lps_syscall_handler(ctx)  (core.c)
    → arch_syshandle(ctx)     (sys.c)
```

`arch_syshandle` 首先区分中断原因：
- `CAUSE_IRQ_U_TIME`：定时器中断，设置下一个 `UTIMECMP`，然后调用 `lps_thread_exit` 让出 CPU（抢占）。
- `CAUSE_DASICS_U_CHECK_FAULT + DFR_LF/SF`：内存边界违规，assert 终止。
- `CAUSE_DASICS_U_CHECK_FAULT + DFR_EF`（ecall）：正常系统调用路径，`uepc += 4`（跳过 ecall 指令），读取 `a7` 作为 syscall 号，`a0-a5` 作为参数，调用 `syshandle`。

`syshandle` 采用两段式分发：
1. 第一段（快速路径）：`brk`、`mmap`、`mprotect`、`munmap`、`exit`、`exit_group`、`runtime_call`。
2. 如果未处理且启用了 passthrough，则直接透传给宿主内核。
3. 第二段：文件 I/O、线程、信号（stub）、时间、随机数、ioctl 等。
4. 未知 syscall → 打印错误并 `exit(1)`。

已实现的 syscall（~60 个）见 `sys.c` switch 语句。

### 5.5 文件描述符子系统

**FDTable**（`fd.h/c`）：最多 1024 个文件描述符，`pthread_mutex` 保护。

**FDFile**（`kfile.h`）：基于 vtable 的文件对象：
```c
struct FDFile {
    void *dev;    // 指向具体设备（宿主 fd 或 Pipe 结构体）
    ssize_t (*read)(void *dev, uint8_t *buf, size_t n);
    ssize_t (*write)(void *dev, uint8_t *buf, size_t n);
    off_t   (*lseek)(...);
    int     (*stat_)(...);
    int     (*getdents)(...);
    // 其他：close/chown/chmod/truncate/sync/ioctl
    int kfd;          // 宿主实际 fd（对 pipe 等虚拟设备为 -1）
    int refcount;     // 引用计数（dup/close）
    pthread_mutex_t lk;
};
```

`fdinit` 初始化标准 I/O（0/1/2），通过 `filenew`（`kfile.c`）创建指向宿主 fd 的 FDFile。

---

## 6. 应用程序层（apps）

### 6.1 自定义 Linker Script（`ld.ld`）

沙箱内运行的 ELF 使用自定义 linker script，将程序基地址设置为 `0x1000000`（16MB），这是一个 Box 内部的偏移地址（每个 Box 的绝对地址由 BoxMap 分配的 base 决定，ELF 作为 PIE 会被加载到 `box.base + 0x1000000` 处）。

### 6.2 lps-go：单进程运行器

```c
// 使用方式：lps-go <ELF二进制>
LPSEngine → LPSLinuxEngine → LPSProc → load ELF → LPSThread → rrschedstart()
```

最简单的运行器，验证单个程序是否能在沙箱内正确运行。

### 6.3 lps-pipe：双进程 IPC 测试

加载"ping"和"pong"两个进程，建立双向管道（`lps_pipe_new`），分配固定 fd（读=3，写=4），并行运行两个进程测试跨沙箱 IPC。

### 6.4 lps-preempt：抢占调度测试

- 支持 `--utimer` 标志：启用用户态定时器中断实现抢占（`lps_utimer_init`，设置 `USTATUS.UIE + UTIMECMP`）。
- 支持 argp 命令行解析，可传入最多 256 个 ELF 并发运行。
- 使用 RR 调度器。

### 6.5 lps-sched：多核调度压力测试

实例化 100 个线程（同一 ELF 的 100 份实例），使用多核调度器（mcsched，默认 2 核），压力测试多核隔离和调度性能。
Box 大小 64MB，堆 16MB，栈 2MB（防止 100 个进程耗尽地址空间）。

### 6.6 utimer-test：用户态定时器独立测试

在沙箱外（直接在宿主上）测试 RISC-V N 扩展的用户态定时器功能：
- `prepare_u_intr()`：写 `UTVEC`→`u_intr_entry`，使能 `USTATUS` 和 `UIE`。
- 汇编 `u_intr_entry`：保存全部寄存器，调用 `u_intr_handler()`，恢复并 URET。
- 定时器间隔 100,000,000 cycles，每次中断重新设置 `UTIMECMP`。

---

## 7. 完整执行流程

以 `lps-go /path/to/app` 为例，端到端流程如下：

```
main() [lps-go]
  │
  ├─ lps_new(opts={boxsize=4GB}, nsandboxes=1)
  │    ├─ boxmap_new() → boxmap_reserve(256TB 尝试 → 按需缩小)
  │    └─ lps_init_dasics() → csrw UTVEC = &dasics_ufault_entry
  │
  ├─ lps_linux_new(engine)
  │    └─ 设置 brksize/stacksize 默认值
  │
  ├─ lps_proc_new(x_engine)
  │    ├─ lps_box_new(engine) → boxmap_addspace() → 得到 [base, base+4GB)
  │    └─ fdinit(engine, &fdtable) → 建立 stdin/stdout/stderr
  │
  ├─ lps_proc_load_file(proc, "/path/to/app")
  │    ├─ mmap 文件 → elf_load()
  │    │    ├─ 校验 ELF，读 Program Headers
  │    │    ├─ 对每个 PT_LOAD: lps_box_mapat(box, base+seg.vaddr, ...)
  │    │    └─ 返回 entry, lastva (brk 起点)
  │    └─ lps_box_mapat(box, brkbase, brksize, PROT_NONE) → 预留堆空间
  │
  ├─ lps_thread_new(proc, argc, argv, envp)
  │    ├─ lps_ctx_new(box, t)
  │    │    ├─ malloc(sizeof(LPSContext))
  │    │    ├─ malloc(2MB) 内核栈
  │    │    ├─ kregs.ra = lps_ctx_entry（上下文切换返回点）
  │    │    ├─ lps_membound_set(ctx, 0, R|W, base, base+size) → 整个 Box 可访存
  │    │    └─ lps_jmpbound_set(ctx, 0, base, base+size)       → 整个 Box 可跳转
  │    ├─ lps_box_mapat(box, end-stacksize, stacksize, R|W) → 映射栈
  │    ├─ stack_init(t, argc, argv, envp) → 写 argc/argv/envp/auxv 到栈
  │    ├─ regs.sp = stack_start；regs.uepc = proc->entry
  │    └─ lps_membound_set(ctx, 0, R|W, base, end)  （最终整 Box 权限）
  │
  ├─ rrschedadd(t) → 加入 RR 队列
  │
  └─ rrschedstart(false)
       └─ loop:
            t = get_next_runable()
            lps_thread_run(t)              ← 进入沙箱
              │  lps_kswitch_to(ctx)
              │    lps_load_dasics(ctx)    ← 写 16*2+4*2+2=38 个 CSR
              │    kswitch(_, kctx, ctx->kregs)  ← 保存运行时寄存器，加载 ctx 内核寄存器
              │    [lps_ctx_entry]:
              │      sd sp/gp/tp → regs.host_*
              │      RESTORE_ALL + URET  ← 进入用户态，从 regs.uepc 开始执行
              │
              │  ← [用户态程序运行...]
              │
              │  [ecall 指令触发 DASICS EF]
              │    CPU 跳转 UTVEC = dasics_ufault_entry
              │      SAVE_ALL         ← 保存用户寄存器，切换到内核栈
              │      lps_syscall_handler(ctx)
              │        arch_syshandle(ctx)
              │          syshandle(t, a7, a0-a5)
              │            → 对应 sys_*() 函数
              │      RESTORE_ALL + URET ← 返回用户态，uepc 已 +4 跳过 ecall
              │
              │  [exit/exit_group syscall]
              │    sys_exit_group() → lps_thread_exit(t)
              │      lps_kswitch_from(ctx) ← 保存 ctx 内核寄存器，恢复运行时 kctx
              │      ← 回到 lps_thread_run() 返回点
              │
            if t->state == THREAD_RUNNABLE → 重新入队
            if t->state == THREAD_EXITED   → 结束
```

---

## 8. 关键数据结构与关系

```
LPSEngine
  └─── BoxMap (虚拟地址总管)
          └─── AddrRegion[] (每个 PROT_NONE 预留块)
                    └─── ExtAlloc (位图，按 chunksize 分配)

LPSBox (每个沙箱)
  ├─── MMAddrSpace (映射追踪链表 MMNode)
  ├─── base, size (虚拟地址区间)
  └─── engine

LPSContext (执行上下文)
  ├─── LPSRegs (用户态寄存器状态，含 DASICS 边界)
  │       ├─── host_sp/gp/tp (运行时侧的 sp/gp/tp，存于此以便异常时还原)
  │       ├─── 32 GPR + 32 FPR
  │       ├─── uepc/ucause/utval/dfreason
  │       ├─── memcfg + membound[16][2]
  │       └─── jmpcfg + jmpbound[4][2]
  ├─── KRegs (内核侧 callee-saved：ra/sp/s0-s11)
  ├─── box  → LPSBox
  └─── userdata → LPSThread

LPSLinuxEngine
  └─── engine → LPSEngine

LPSProc
  ├─── engine → LPSLinuxEngine
  ├─── box    → LPSBox
  ├─── FDTable (1024 FDFile 指针，mutex 保护)
  ├─── brkbase, brksize, entry, elfinfo, seginfo
  └─── cwd

LPSThread
  ├─── proc → LPSProc
  ├─── ctx  → LPSContext
  ├─── tid, stack, stack_size, state
  └─── elem (List 节点，用于调度队列)
```

---

## 9. 安全模型与 DASICS 集成细节

### 9.1 内存隔离

当前实现中，`lps_thread_new` 中的 DASICS 边界设置为**整个 Box 可读写**：

```c
lps_membound_set(t->ctx, 0, LIBCFG_R | LIBCFG_W, start, end);
lps_jmpbound_set(t->ctx, 0, start, end);
```

代码中有被注释掉的精细化代码（以 ELF segment 粒度分别设置读/写/执行权限），说明设计目标是支持细粒度权限，但当前简化为整 Box 权限。

**结论**：Box 间的隔离是有效的（不同沙箱分配到不同虚拟地址区间，DASICS 边界仅覆盖本 Box），但 Box 内部的细粒度保护（如防止沙箱写 .text 段）尚未完全实现。

### 9.2 syscall 参数安全检查

`sys/sys.h` 中定义了参数校验宏：
- `ptrcheck(t, addr)`：等价于 `lps_box_ptrvalid(t->proc->box, addr)`，检查指针在 Box 范围内。
- `bufcheck(t, addr, len)`：检查 `[addr, addr+len)` 均在 Box 内。
- `pathcopy/bufhost/copyout`：将沙箱内字符串/数据安全复制到宿主栈，或反向复制结果。

所有涉及沙箱地址的 syscall 参数都应经过 `ptrcheck`/`bufcheck` 验证，防止沙箱程序通过 syscall 访问 Box 外的宿主内存。

### 9.3 系统调用隔离策略

- **拒绝 clone**：`SYS(clone, sys_nosys(t, "clone"))`，沙箱内无法创建线程（多线程通过 LPS 自身的线程管理）。
- **拒绝 socket/mremap/statx 等**：返回 ENOSYS。
- **忽略信号相关 syscall**：`rt_sigaction`、`rt_sigprocmask` 等静默忽略（返回 0）。
- **mmap 标志白名单**：仅允许 `MAP_ANONYMOUS|MAP_PRIVATE|MAP_NORESERVE|MAP_DENYWRITE|MAP_FIXED`，拒绝 `MAP_SHARED` 等。
- **passthrough 模式**：可选启用，未处理的 syscall 直接透传给宿主（用于测试/基准）。

### 9.4 定时器抢占（实验性）

启用 `--utimer` 后：
- `lps_utimer_init()`：设置 `USTATUS=0x10`（UPIE bit），`UIE=0x111`。
- 定时器中断触发时，`arch_syshandle` 识别 `CAUSE_IRQ_U_TIME`，设置下一个 `UTIMECMP`，然后调用 `lps_thread_exit` 强制切出当前线程，实现时间片轮转抢占。

---

## 10. 调度器设计

### 10.1 Round-Robin 调度器（单核）

**文件**：`sched.c`

```
全局链表队列 runq：(FIFO)

rrschedstart():
  while (有可运行线程):
    t = dequeue(runq)
    lps_thread_run(t)        ← 阻塞直到线程 exit/yield
    if t->RUNNABLE: enqueue(runq, t)
    if t->EXITED: 打印结束信息
```

**阻塞/唤醒**：
- `rrschedblock(q)`：将当前线程放入 waitq，切出（`lps_thread_exit`）。
- `rrschedwake(q)`：将 waitq 中所有线程移回 runq，设置为 RUNNABLE。
- pipe 的读写等待直接使用此机制。

### 10.2 多核调度器（MCSched）

**文件**：`sched.c`

```
全局队列 mcsched_grunq (RingQueue, 1024 容量)
每个 core 有本地队列 mcsched[i]->runq (RingQueue, 1024 容量)

mcsched_loop() (每个 pthread 一份):
  while (true):
    1. pop_one(local_runq)
    2. 如果空: pop(global_runq, count/ncore+1) → push 到 local_runq
    3. 如果还空: 从其他 core 的 local_runq 的尾部 steal 一个（work stealing）
    4. 执行: lps_thread_run(t)
    5. 线程 RUNNABLE → push 回 local_runq
    6. 无任务 → usleep(10ms)
```

**RingQueue**（`queue.c`）：环形缓冲，head/tail 各有独立 mutex，支持 `ring_pop_back` 从尾部窃取（work stealing）。

`pthread_setaffinity_np` 将每个 worker 线程绑定到对应 CPU core，减少跨核调度开销。

---

## 11. IPC 管道机制

**文件**：`sys/sys_pipe.c`

管道是轻量虚拟设备，最多 64 个，每个 1024 字节环形缓冲区：

```c
struct Pipe {
    char data[1024];   // 环形缓冲区
    size_t nread, nwrite;
    bool readopen, writeopen;
    struct Queue readq, writeq;  // 阻塞等待队列
};
```

- **写满阻塞**：`nwrite == nread + PIPESZ` → 唤醒 readq → 自己阻塞到 writeq。
- **读空阻塞**：`nread == nwrite` 且写端未关闭 → 自己阻塞到 readq。
- 阻塞使用 `rrschedblock(q)` 让出 CPU，唤醒用 `rrschedwake(q)`。

`lps_pipe_new(from, fromfd, to, tofd)`：跨进程创建管道对，`from` 持有写端，`to` 持有读端，通过 `fdassign` 分配指定 fd 号。

**局限**：管道基于 RR 调度器的 block/wake 机制，与多核调度器不兼容（`mcsched_loop` 不处理阻塞唤醒）。

---

## 12. 构建系统

使用 Meson 构建，支持交叉编译（RISC-V64）：

```bash
meson setup build --cross-file riscv64-linux-gcc.ini
ninja -C build
```

关键编译选项：
- `-march=rv64gc_zba`：指定 RISC-V 扩展集（含 DASICS 需要的用户态中断相关扩展）。
- 静态链接（`link_args: ['-static']`）。
- `-Werror=implicit-function-declaration`、`-Werror=incompatible-pointer-types`：严格类型检查。
- `libargp` 作为 subproject 提供参数解析。

产出：
- `install/lib/liblps-core.a`
- `install/lib/liblps-linux.a`
- `install/bin/{lps-go,lps-pipe,lps-preempt,lps-sched,utimer-test}`
- `install/include/{lps_arch.h,lps_core.h,lps_linux.h}`

---

## 13. 当前局限与 TODO

从代码注释和实现中发现的主要 TODO/限制：

| 位置 | 问题描述 |
|------|---------|
| `thread.c:240-253` | 精细化 DASICS 边界（堆/栈/ELF 段分别设置）代码已注释，当前整 Box 统一权限 |
| `sys_mem.c:11` | `sys_brk` 中 `lk_brk` 互斥锁被注释掉，多线程 brk 不安全 |
| `proc.c:37` | TODO: 仅支持 static-pie，不支持动态链接 |
| `proc.c:54` | 注释说明 brk 预留机制有待完善（"Abandon"标注） |
| `sched.c:128` | work stealing 循环有 bug：只对最后一个非自身 core 的 steal 结果赋值（缺少 break） |
| `sys.c:129` | `regs->ucasue` 字段名拼写错误（应为 `ucause`，`lps_arch.h` 中也是 `ucasue`） |
| `sys.c:156` | `uepc += 4` 仅处理 4 字节指令，TODO 需支持 2 字节压缩指令（RVC） |
| `proc.c:113-114` | `proc_mapany` 中 locking TODO |
| `sched.c:197` | 多核调度器 utimer 初始化 TODO |
| `sys_pipe.c` | pipe 仅支持 RR 调度器的 block/wake，不支持多核调度器 |
| `ctx.c:51` | "tmp dasics whole range" 注释表明当前 DASICS 边界是临时方案 |
| `sched.c:161` | 函数名 `mcshed_start()` 有拼写错误（应为 `mcsched_start`），头文件声明也如此 |
| 整体 | clone/futex/信号 等未实现，多线程程序无法完整运行 |

---

## 14. 总结与关键发现

### 架构特点

LPS 是一个**两层架构**的沙箱运行时：

1. **lps-core**（平台相关）：提供基于 DASICS 硬件的地址空间隔离、上下文切换和异常路由，是最底层的 trusted runtime，代码量约 1000 行（含汇编）。

2. **lps-linux**（Linux ABI 模拟）：在 lps-core 基础上提供 Linux syscall 接口模拟，使标准程序无需修改即可运行，代码量约 4000 行。

### 设计精妙之处

- **USCRATCH 双用途**：异常入口无临时寄存器问题的优雅解法——`csrrw s0, USCRATCH, s0` 一条指令同时完成"获取 context 指针"和"保存 s0"两件事。
- **BoxMap 超大预留**：通过一次 PROT_NONE mmap 预留 256TB，避免地址碎片，且不消耗物理内存。
- **LPSRegs 中内嵌 DASICS 状态**：将 DASICS 边界寄存器纳入 Context 的寄存器状态，使多沙箱切换只需一次批量 CSR 写入（`lps_load_dasics`），架构干净。
- **vtable 文件系统**：FDFile 的函数指针设计使管道和宿主文件对 syscall 层透明，可扩展性好。
- **Work-stealing 调度**：多核场景下支持任务窃取，适合 CPU 密集型负载均衡。

### 项目定位

LPS 是对 DASICS 轻量隔离扩展在**同地址空间多进程**场景下的系统级验证原型。它证明了：
- 无需 MMU 切换即可实现进程级隔离（通过 DASICS CSR 边界）。
- 用户态异常处理（RISC-V N 扩展）可以高效实现 syscall 拦截。
- 同地址空间多沙箱模型可以支持完整的 Linux ABI 兼容层。

目前处于**研究原型**阶段，精细化 DASICS 边界、完整多线程支持、动态链接等功能尚在开发中。
