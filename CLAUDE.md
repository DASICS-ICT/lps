# LPS 项目上下文（每次工作前必读）

## 项目定位
LPS（Lightweight Process Sandbox）：基于 RISC-V DASICS 扩展的用户态沙箱运行时。
在**同一宿主进程地址空间**内加载并隔离运行多个静态 PIE ELF，CPU 硬件边界寄存器强制访存/跳转检查，无需内核参与。

## !! 运行必读 !!
**所有 LPS 可执行文件（`lps-go`、`lps-bench`、`lps-startup`、`lps-chain`、`lps-pipe`、`lps-preempt`）命令行末尾必须加 `-dasics`**，否则 DASICS 扩展不会被 OS 启用：
```bash
lps-go bin/wc -dasics
lps-bench 100 2 bin/workload 1000 -dasics
lps-startup bin/exit_imm -dasics
lps-chain bin/fc_source bin/fc_transform bin/fc_sink -dasics
```

## 架构（两层）
```
lps-core/   硬件相关层（DASICS CSR、上下文切换、地址空间管理）
  boxmap.c    256TB PROT_NONE 预留，按 boxsize 分块分配虚拟地址
  box.c       沙箱实例 [base, base+size)，含 MMAddrSpace 追踪
  bound.c     DASICS 16组内存边界 + 4组跳转边界 CSR 写入
  ctx.c       Context 创建/切换（lps_kswitch_to / kswitch）
  runtime.S   SAVE_ALL/RESTORE_ALL 宏，dasics_ufault_entry 异常入口

lps-linux/  Linux ABI 层（ELF 加载、syscall 模拟、调度器）
  elfload.c   静态 PIE (ET_DYN) ELF 加载，不支持动态链接
  thread.c    线程创建、栈布局（argc/argv/envp/auxv）
  sched.c     RRSched（单核 FIFO）+ MCSched（多核 work-stealing）
  sys.c       ~60 个 Linux syscall 分发
  sys_pipe.c  虚拟管道（1024B 环形缓冲，仅兼容 RRSched）
```

## 关键数据结构
| 结构体 | 用途 |
|--------|------|
| `LPSRegs` | 用户态完整寄存器 + DASICS 边界（memcfg/membound[16][2], jmpcfg/jmpbound[4][2]）|
| `KRegs` | 内核侧 callee-saved（ra/sp/s0-s11），Context 切换保存/恢复 |
| `LPSContext` | regs + kregs + box + userdata(→LPSThread) |
| `LPSBox` | MMAddrSpace + base + size |
| `LPSProc` | engine + box + FDTable + brk + cwd + elfinfo |
| `LPSThread` | proc + ctx + tid + stack + state + elem(调度队列节点) |

## 关键机制
- **USCRATCH**：存 context 指针，`csrrw s0, USCRATCH, s0` 异常入口零开销获取 context
- **ecall 路径**：ecall → DASICS EF → `dasics_ufault_entry` → SAVE_ALL → `syshandle` → RESTORE_ALL → URET
- **Context 切换**：`lps_kswitch_to` → `lps_load_dasics`（38 个 csrw）→ `kswitch` → `lps_ctx_entry` → RESTORE_ALL+URET
- **DASICS 边界**：目前整个 Box 统一 R/W 权限（精细化代码已注释，是临时方案）
- **定时器抢占**：UTIMECMP → U_TIMER 中断 → `lps_thread_exit` 让出 CPU

## 已知 Bug（修改相关代码时注意）
| 位置 | 问题 |
|------|------|
| `lps_arch.h` + `sys.c` | `ucasue` 字段名拼写错误（应为 `ucause`），功能正常但可读性差 |
| `sched.c` work-stealing 循环 | 缺少 `break`，steal 结果只保留最后一个 core 的 |
| `sched.c:161` | 函数名 `mcshed_start()` 拼写错误（头文件声明也如此，保持一致即可） |
| `sys_mem.c` | `sys_brk` 中互斥锁被注释，多线程 brk 不安全 |
| `sys.c:156` | `uepc += 4` 不支持 RVC（2 字节压缩指令） |

## 构建
```bash
meson setup build --cross-file riscv64-linux-gcc.ini
ninja -C build
# 沙箱内 ELF（static-pie）
riscv64-linux-musl-gcc -static-pie -O2 -o bin/workload microbenchmark/workload.c
```
- 架构：`-march=rv64gc_zba`，静态链接
- apps 使用 `apps/ld.ld`（base=0x1000000）

## 当前 Benchmark 状态
- **macrobenchmark**：SPEC CPU2006 INT，GEOMEAN overhead ~0.75% vs native
- **microbenchmark**（`microbenchmark/`）：
  - `workload [fib-count]`：fib 计算负载，默认 1000 次
  - `exit_imm`：最简退出，测启动延迟
  - `wc`：stdin 行/字/字节统计
  - `fc_source/transform/sink`：三级 function chain（LPS pipe IPC）
  - `native_concurrent/multicore/startup`：native fork+exec 对照
  - 脚本：`scripts/{startup,concurrent,multicore,chain}-test.sh`

## Apps
| 程序 | 用途 |
|------|------|
| `lps-go <elf>` | 单沙箱运行器 |
| `lps-bench <N> <ncore> <elf> [fib-count]` | N 个沙箱多核并发 benchmark |
| `lps-startup <elf>` | 启动延迟拆解测量 |
| `lps-chain <src> <xfm> <snk>` | 三级管道 function chain |
| `lps-pipe` | 双进程 IPC 测试 |
| `lps-preempt [--utimer] <elf>...` | 抢占调度测试 |
