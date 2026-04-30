#!/bin/sh
# 多核可行性验证：1/2/4/8/16/32 核加速比测试
# 用法：在 microbenchmark/ 目录下运行
#   sh scripts/multicore-test.sh [fib-count]

set -e
cd "$(dirname "$0")/.."

FIB=${1:-1000}
ELF=bin/workload
N=50     # FPGA 上适当减小 N，加快测试速度

CORE_LIST="1 2 4 8"

# echo "=== Native (fork+exec, explicit ELF load) fib=$FIB ==="
# for CORES in $CORE_LIST; do
#     echo "[native] ${CORES} core(s):"
#     bin/native_multicore $N $CORES $ELF $FIB
# done

echo ""
echo "=== LPS (mcsched) fib=$FIB ==="
for CORES in $CORE_LIST; do
    echo "[lps] ${CORES} core(s):"
    time lps-bench $N $CORES $ELF $FIB -dasics > /dev/null
done