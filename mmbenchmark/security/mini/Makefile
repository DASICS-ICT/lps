# Makefile for LPS DASICS security micro-tests
#
# Build:  make
# Run:    ./run_tests.sh [path-to-lps-security]

CC = riscv64-linux-gcc
CFLAGS = -static-pie -O2 -I.

# --- basic DASICS fault tests ---
#   test_load   - out-of-bound load  (DFR_LF)
#   test_store  - out-of-bound store (DFR_SF)
#   test_jump   - out-of-bound jump via function pointer (DFR_JF)
#   test_ret    - out-of-bound jump via return address   (DFR_JF)
#   test_ecall  - ecall interception (DFR_EF)
#
# --- advanced security tests ---
#   test_cross       - cross-sandbox isolation
#   test_csr         - CSR tampering defense
#   test_sysptr      - syscall pointer validation
#   test_boundary    - boundary precision
#   test_mmap        - mmap escape prevention
#   test_consistency - state consistency after fault

TESTS = test_load test_store test_jump test_ret test_ecall \
        test_cross test_csr test_sysptr test_boundary test_mmap test_consistency
BINS = $(addprefix bin/, $(TESTS))

COMMON = test_common.h

all: $(BINS)

# Basic tests (no common header needed)
bin/test_load: test_load.c
	$(CC) $(CFLAGS) -o $@ $<

bin/test_store: test_store.c
	$(CC) $(CFLAGS) -o $@ $<

bin/test_jump: test_jump.c
	$(CC) $(CFLAGS) -o $@ $<

bin/test_ret: test_ret.c
	$(CC) $(CFLAGS) -o $@ $<

# Tests using test_common.h
bin/test_ecall: test_ecall.c $(COMMON)
	$(CC) $(CFLAGS) -o $@ $<

bin/test_cross: test_cross.c
	$(CC) $(CFLAGS) -o $@ $<

bin/test_csr: test_csr.c $(COMMON)
	$(CC) $(CFLAGS) -o $@ $<

bin/test_sysptr: test_sysptr.c $(COMMON)
	$(CC) $(CFLAGS) -o $@ $<

bin/test_boundary: test_boundary.c $(COMMON)
	$(CC) $(CFLAGS) -o $@ $<

bin/test_mmap: test_mmap.c $(COMMON)
	$(CC) $(CFLAGS) -o $@ $<

bin/test_consistency: test_consistency.c $(COMMON)
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f bin/*

.PHONY: all clean
