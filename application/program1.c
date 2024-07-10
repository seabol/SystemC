


/*
 *
 * Copyright (c) 2005-2019 Imperas Software Ltd., www.imperas.com
 *
 * The contents of this file are provided under the Software License
 * Agreement that you accepted before downloading this file.
 *
 * This source forms part of the Software and can be used for educational,
 * training, and demonstration purposes but cannot be used for derivative
 * works except in cases where the derivative works require OVP technology
 * to run.
 *
 * For open source models released under licenses that you can use for
 * derivative works, please visit www.OVPworld.org or www.imperas.com
 * for the location of the open source models.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "dmacRegisters.h"

typedef unsigned int  Uns32;
typedef unsigned char Uns8;

#include "riscvInterrupts.h"

#define LOG(_FMT, ...)  printf( "TEST DMA: " _FMT,  ## __VA_ARGS__)

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


static inline void writeReg32(Uns32 address, Uns32 offset, Uns32 value)
{
    *(volatile Uns32*) (address + offset) = value;
}


static inline Uns32 readReg32(Uns32 address, Uns32 offset)
{
    return *(volatile Uns32*) (address + offset);
}


#define ENABLE      0x00000001
#define INTEN       0x00008000
// burst size is 1<<BURST_SIZE
#define BURST_SIZE       2

volatile static Uns32 interruptCount = 0;

void interruptHandler(void)
{

    printf("--------- RISC-V-1 interrupted ----------\n");

    // printf("RISC-V-1 interrupted!\n\n");

    writeReg32(DMA_BASE, (Uns32)(0xc), (Uns32)(0));

}



int main(int argc, char *argv[]) {
    int_init(trap_entry);
    int_enable();

    // A. At initialization RISC-V-1 writes 1 KB data into 0x200400 to 0x2007FF.
    int int_cnt = 0;
    printf("RISC-V-1 initialization\n");

    volatile unsigned int *p;
    for (p = (volatile unsigned int *)0x200400; p < (volatile unsigned int *)0x200800; p += 2) 
    {
        *p = 0x67452301;
        *(p + 1) = 0xEFCDAB89;
    }

    while (int_cnt < 3) {
         while (*(volatile unsigned int *)(0x200800) == 1);
        // C. RISC-V-1 monitors the data at address 0x200800. When the data is
        //    0x0, RISC-V-1 programs DMA's 1st set of control registers to copy
        //    a 1KB data starting from 0x200400 to 0x300000.

        printf("RISC-V-1 writing DMA control registers\n");

        writeReg32(DMA_BASE, (Uns32)(0x0), (Uns32)(0x200400));
        writeReg32(DMA_BASE, (Uns32)(0x4), (Uns32)(0x300000));
        writeReg32(DMA_BASE, (Uns32)(0x8), (Uns32)(0x400));
        writeReg32(DMA_BASE, (Uns32)(0xc), (Uns32)(1));

        wfi();

        int_cnt++;
        // // D. After the 1KB data is transmitted, RISC-V-1 writes a flag 0x1 to
        // //    address 0x200800 to indicate the end of data transmission.
        printf("RISC-V-1  flag 0x200800  = 0x1\n");
        *(volatile unsigned int *)(0x200800) = 1;

        // G. Repeat above data exchange/transmission 3 times and stop.
        printf("Program1 count (%d)\n", int_cnt);
        printf("\n");
        printf("\n");
    }
    printf(">>>>Program1 end (%d)<<<<\n", int_cnt);

    return 0;
}
