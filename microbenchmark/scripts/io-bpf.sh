#!/bin/sh
# Sequential I/O benchmark: native Linux vs software seccomp-bpf interposition.
# 用法：在 microbenchmark/ 目录下运行
#   sh scripts/io-seccomp-bpf-test.sh /path/to/1G-file
#
# 可选环境变量：
#   IO_SIZE=1G
#   IO_ROUNDS=5
#   IO_BUFS="4K 16K 64K 256K"

set -e
cd "$(dirname "$0")/.."

TEST_FILE=${1:-io-test-1g.dat}
IO_SIZE=${IO_SIZE:-1G}
IO_ROUNDS=${IO_ROUNDS:-5}
IO_BUFS=${IO_BUFS:-"4K 16K 64K 256K"}

if [ ! -f "$TEST_FILE" ]; then
    echo "test file not found: $TEST_FILE" >&2
    echo "pass a pre-generated 1GB file path as the first argument" >&2
    exit 1
fi

echo "=== Sequential read I/O benchmark ==="
echo "file=$TEST_FILE size=$IO_SIZE rounds=$IO_ROUNDS buffers=$IO_BUFS"
echo ""

for buf in $IO_BUFS; do
    echo "=== buffer=$buf native ==="
    bin/io_bpf native --buffer "$buf" --size "$IO_SIZE" \
        --rounds "$IO_ROUNDS" "$TEST_FILE"
    echo ""

    echo "=== buffer=$buf seccomp-bpf notify ==="
    bin/io_bpf seccomp --buffer "$buf" --size "$IO_SIZE" \
        --rounds "$IO_ROUNDS" "$TEST_FILE"
    echo ""
done
