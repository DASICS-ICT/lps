#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>

#include "uintr.h"

const char *test_info = "[MAIN] N-extension u-timer interrupt test\n";

void exit_function(void) {
	printf("[MAIN] u-timer interrupt test finished\n");
}

int main(void) {
	atexit(exit_function);

	printf(test_info);

    prepare_u_intr();

    printf("[U_INTR_HANDLER] init utimecmp to 0\n");
	csr_write(0x45, 0);

    while(1) {
        
    }

	clear_u_intr();

	return 0;
}