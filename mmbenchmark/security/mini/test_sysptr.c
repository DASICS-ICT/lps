// test_sysptr.c - Syscall pointer escape test
//
// Verifies that the LPS syscall handler validates buffer pointers.
// Calls write(1, bait_addr, 8) where bait_addr is outside the box.
// The handler's bufcheck should reject it and return -EINVAL.
// If the write succeeds, host memory is leaked → security failure.

#include <stdlib.h>
#include <stdint.h>
#include "test_common.h"

int main(int argc, char **argv) {
    if (argc < 2) return 1;
    uintptr_t bait = (uintptr_t)strtoul(argv[1], NULL, 16);

    // Try to write(1, bait_addr, 8) — bait is outside box.
    // sys_write → bufhost → ptrcheck → should fail.
    long ret = raw_write(1, (const char *)bait, 8);

    if (ret < 0) {
        // Pointer was validated and rejected.
        print("PASS [sysptr]: write() rejected out-of-box buffer pointer\n");
        raw_exit(0);
    }

    // write succeeded — host memory leaked to stdout!
    print("FAIL [sysptr]: write() accepted out-of-box buffer pointer!\n");
    raw_exit(1);
}
