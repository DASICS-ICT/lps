#!/bin/sh
# 多核可行性验证：1 核 vs 2 核加速比
# 用法：在 microbenchmark/ 目录下运行
#   sh scripts/multicore-test.sh [fib-count]
# FPGA 平台只有 2 核，验证 1 核 vs 2 核的加速效果

set -e
cd "$(dirname "$0")/.."

FIB=${1:-1000}
ELF=bin/workload
N=100     # FPGA 上适当减小 N，加快测试速度

echo "=== Native (fork+exec, explicit ELF load) fib=$FIB ==="
echo "[native] 1 core:"
bin/native_multicore $N 1 $ELF $FIB
echo "[native] 2 cores (pinned):"
bin/native_multicore $N 2 $ELF $FIB

echo ""
echo "=== LPS (mcsched) fib=$FIB ==="
echo "[lps] 1 core:"
lps-bench $N 1 $ELF $FIB -dasics
echo "[lps] 2 cores:"
lps-bench $N 2 $ELF $FIB -dasics
