#!/bin/sh
# run_tests.sh - Run LPS DASICS security micro-tests
#
# Usage: ./run_tests.sh [lps-security-path]
# Default: ../../../build/lps-security
# All executables must end with -dasics flag.

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
RUNNER="${1:-$SCRIPT_DIR/../../../build/lps-security}"
BIN_DIR="$SCRIPT_DIR/bin"

if [ ! -x "$RUNNER" ]; then
    echo "Error: lps-security not found at: $RUNNER"
    echo "Build it first: ninja -C build"
    exit 1
fi

TESTS="test_load test_store test_jump test_ret test_ecall \
       test_cross test_csr test_sysptr test_boundary test_mmap test_consistency"
PASS=0
FAIL=0
TOTAL=0

echo "============================================"
echo " LPS DASICS Security Micro-Tests"
echo "============================================"
echo ""

for test in $TESTS; do
    TOTAL=$((TOTAL + 1))
    elf="$BIN_DIR/$test"

    if [ ! -f "$elf" ]; then
        echo "[$test] SKIP - binary not found: $elf"
        FAIL=$((FAIL + 1))
        continue
    fi

    echo "--- $test ---"
    if $RUNNER "$elf" -dasics 2>&1; then
        PASS=$((PASS + 1))
    else
        echo "  FAIL (runner returned non-zero)"
        FAIL=$((FAIL + 1))
    fi
    echo ""
done

echo "============================================"
echo " Results: $PASS/$TOTAL passed, $FAIL failed"
echo "============================================"

if [ $FAIL -eq 0 ]; then
    exit 0
else
    exit 1
fi
