#!/bin/sh
# Function chain 对比测试：LPS 三级管道 vs native Unix pipe
# 用法：在 microbenchmark/ 目录下运行
#   sh scripts/chain-test.sh
#
# 校验正确性：两者的 checksum 输出应完全一致。
# 关键指标：IPC 带宽（KB/s）和总延迟（rdcycle 测量）。

set -e
cd "$(dirname "$0")/.."

echo "=== Native Unix pipe chain ==="
bin/native_chain bin/fc_source_native bin/fc_transform_native bin/fc_sink_native

echo ""
echo "=== LPS function chain ==="
lps-chain bin/fc_source bin/fc_transform bin/fc_sink -dasics
