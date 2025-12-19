#include <stdio.h>
#include <stdlib.h>

#include "uintr.h"

void prepare_u_intr(void){
	csr_write(0x05, (uint64_t)u_intr_entry); // set utvec
    uint64_t ustatus = csr_read(0x00);
    printf("[prepare_u_intr]: before write ustatus: %lu\n", ustatus);
    csr_write(0x00, 0x11); //set ustatus uie/upie
    ustatus = csr_read(0x00);
    printf("[prepare_u_intr]: after write ustatus: %lu\n", ustatus);
    csr_write(0x04, 0x111); // set uie: enable all u intr
}


void clear_u_intr(void){
    csr_write(0x00, 0x0); // clear ustatus uie/upie
    csr_write(0x04, 0x0); // clear uie: disable all u intr
	csr_write(0x05, 0x0); // clear utvec
}

uint64_t get_time(void) {
    uint64_t time;
    asm volatile ("rdtime %0" : "=r"(time));
    return time;
}

#define INTERVAL 100000000

void u_intr_handler(void) {
	uint64_t ucause = csr_read(0x042);
    uint64_t utval = csr_read(0x043);
    uint64_t uepc = csr_read(0x041);
	printf("[U_INTR_HANDLER] catch u-intr, ucause = 0x%lx, uepc = 0x%lx, utval = 0x%lx\n", ucause, uepc, utval);
	if (ucause == CAUSE_IRQ_U_TIMER){
        uint64_t next = get_time() + INTERVAL;
		printf("[U_INTR_HANDLER] set utimecmp to %lu\n", next);
		csr_write(0x45, next);
	}
}