#include "stdio.h"
#include "riscvInterrupts.h"
#define DMA_BASE 0x100000

static int wait_for_int;

void int_handler() {
    *(volatile unsigned int *)(DMA_BASE + 0x1C) = 0;  // clear
    wait_for_int = 0;
}

void int_init(void (*handler)()) {

    // Set MTVEC register to point to handler function in direct mode
    int handler_int = (int) handler & ~0x1;
    write_csr(mtvec, handler_int);

    // Enable Machine mode external interrupts
    set_csr(mie, MIE_MEIE);
}

void int_enable() {
    set_csr(mstatus, MSTATUS_MIE);
}

int main(int argc, char *argv[]) {
    int_init(trap_entry);
    int_enable();

    // B. At initialization RISC-V-2 writes 1 KB data into 0x300400 to 0x3007FF.
    int int_cnt = 0;
    volatile unsigned int *p;
    for (p = (volatile unsigned int *)0x300400; p < (volatile unsigned int *)0x300800; p += 2) {
        *p = 0x98BADCFE;
        *(p + 1) = 0x10325476;
    }
    wait_for_int = 0;

    while (int_cnt < 3) {
        // E. RISC-V-2 monitors the data at address 0x200800. When the data is
        //    0x1, RISC-V-2 programs DMA's 2nd set of control registers to copy
        //    a 1KB data starting from 0x300400 to 0x200000.
        while (*(volatile unsigned int *)(0x200800) == 0);

        printf("RISC-V-2 writing DMA control registers\n");
        *(volatile unsigned int *)(DMA_BASE + 0x10) = 0x300400;  // source
        *(volatile unsigned int *)(DMA_BASE + 0x14) = 0x200000;  // target
        *(volatile unsigned int *)(DMA_BASE + 0x18) = 1024;      // size
        *(volatile unsigned int *)(DMA_BASE + 0x1C) = 1;         // start
        wait_for_int = 1;

        while (wait_for_int);
        printf("RISC-V-2 interrupted!\n\n");
        int_cnt++;
        // F. After the 1KB data is transmitted, RISC-V-2 writes a flag 0x0 to
        //    address 0x200800 to indicate the end of data transmission.
        *(volatile unsigned int *)(0x200800) = 0;

        // G. Repeat above data exchange/transmission 3 times and stop.
    }
    printf("Program2 end (%d)\n", int_cnt);
    return 0;
}
