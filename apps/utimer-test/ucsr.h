#ifndef _UCSR_H_
#define _UCSR_H_

typedef enum {
    CSR_USTATUS = 0x000,
    CSR_UIE = 0x004,
    CSR_UTVEC = 0x005,
    CSR_USCRATCH = 0x040,
    CSR_UEPC = 0x041,
    CSR_UCAUSE = 0x042,
    CSR_UTVAL = 0x043,
    CSR_UIP = 0x044,
    CSR_UTIMECMP = 0x045,
} csr_registers_t;

/* Interrupt causes */
#define IRQ_U_SOFT                         0
#define IRQ_S_SOFT                         1
#define IRQ_VS_SOFT                        2
#define IRQ_M_SOFT                         3
#define IRQ_U_TIMER                        4
#define IRQ_S_TIMER                        5
#define IRQ_VS_TIMER                       6
#define IRQ_M_TIMER                        7
#define IRQ_U_EXT                          8
#define IRQ_S_EXT                          9
#define IRQ_VS_EXT                         10
#define IRQ_M_EXT                          11
#define IRQ_S_GEXT                         12
#define IRQ_PMU_OVF                        13
#define IRQ_LOCAL_MAX                      16

/* mip masks */
#define MIP_USIP                           (1 << IRQ_U_SOFT)
#define MIP_SSIP                           (1 << IRQ_S_SOFT)
#define MIP_VSSIP                          (1 << IRQ_VS_SOFT)
#define MIP_MSIP                           (1 << IRQ_M_SOFT)
#define MIP_UTIP                           (1 << IRQ_U_TIMER)
#define MIP_STIP                           (1 << IRQ_S_TIMER)
#define MIP_VSTIP                          (1 << IRQ_VS_TIMER)
#define MIP_MTIP                           (1 << IRQ_M_TIMER)
#define MIP_UEIP                           (1 << IRQ_U_EXT)
#define MIP_SEIP                           (1 << IRQ_S_EXT)
#define MIP_VSEIP                          (1 << IRQ_VS_EXT)
#define MIP_MEIP                           (1 << IRQ_M_EXT)
#define MIP_SGEIP                          (1 << IRQ_S_GEXT)
#define MIP_LCOFIP                         (1 << IRQ_PMU_OVF)

#define csr_read(reg) ({ unsigned long __tmp; \
  asm volatile ("csrr %0, " #reg : "=r"(__tmp)); \
  __tmp; })

#define csr_write(reg, val) ({ \
  asm volatile ("csrw " #reg ", %0" :: "rK"(val)); })

#endif