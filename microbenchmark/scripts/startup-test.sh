#!/bin/sh
# 启动延迟对比：LPS 沙箱 vs native fork+exec
# 用法：在 microbenchmark/ 目录下运行
#   bash scripts/startup-test.sh

set -e
cd "$(dirname "$0")/.."

echo "=== LPS sandbox startup latency ==="
lps-startup bin/exit_imm -dasics

echo ""
echo "=== Native fork+exec latency ==="
bin/native_startup bin/exit_imm
