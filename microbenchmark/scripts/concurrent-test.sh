#!/bin/sh
# 多沙箱并发测试：LPS vs native fork+exec
# 用法：在 microbenchmark/ 目录下运行
#   sh scripts/concurrent-test.sh [ncore] [fib-count]

set -e
cd "$(dirname "$0")/.."

NCORE=${1:-2}
FIB=${2:-1000}
ELF=bin/workload

for N in 100 500 1000; do
    echo "=== N=$N fib=$FIB ==="
    echo "[native] fork+exec $N OS processes (explicit ELF load):"
    bin/native_concurrent $N $ELF $FIB
    echo "[lps]    $N LPS sandboxes ($NCORE cores):"
    lps-bench $N $NCORE $ELF $FIB -dasics
    echo ""
done
