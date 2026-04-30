#!/bin/bash

# 用法: ./copy_benchmarks.sh <src_base> <dst_base>
if [ "$#" -ne 2 ]; then
  echo "Usage: $0 <src_base> <dst_base>"
  echo "Example: $0 /path/to/A /path/to/B"
  exit 1
fi

SRC_BASE="$1"
DST_BASE="$2"

# 定义所有基准测试名称
BENCHMARKS=(
  "401.bzip2"
  "429.mcf"
  "445.gobmk"
  "456.hmmer"
  "458.sjeng"
  "462.libquantum"
  "464.h264ref"
  "473.astar"
)

for bench in "${BENCHMARKS[@]}"; do
  src="${SRC_BASE}/${bench}/build/${bench}"
  dst_dir="${DST_BASE}/${bench}"
  dst="${dst_dir}/${bench}"

  mkdir -p "${dst_dir}"

  if [ -e "${src}" ]; then
    cp -r "${src}" "${dst}"
    echo "[OK] Copied: ${src} -> ${dst}"
  else
    echo "[WARN] Source not found: ${src}"
  fi
done

echo "Done."
