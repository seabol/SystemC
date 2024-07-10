/*
 *
 * Copyright (c) 2005-2021 Imperas Software Ltd., www.imperas.com
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

#include "tlm/tlmModule.hpp"
#include "tlm/tlmDecoder.hpp"
#include "tlm/tlmMemory.hpp"

// Processor configuration
#include "riscv.ovpworld.org/processor/riscv/1.0/tlm/riscv_RV32I.igen.hpp"

// DMA
#include "dma.h"
#include "adaptor.h"

using namespace sc_core;
#define IMPERAS_TLM_CPU_TRACE 1

#define IMPERAS_BACKTRACE  = 1
////////////////////////////////////////////////////////////////////////////////
//                      Dual Core Class                                       //
////////////////////////////////////////////////////////////////////////////////

class DualCore : public sc_module {

  public:
    DualCore (sc_module_name name);

    sc_in<bool>           clk;
    sc_in<bool>           rst_n;
    tlmModule             Platform;
    tlmRam                ram1;
    tlmRam                ram2;
    tlmRam                ram3;
    tlmRam                ram4;
    tlmRam                ram1_1;
    tlmRam                ram2_1;
    tlmDecoder            bus1;
    tlmDecoder            bus2;
    tlmDecoder            bus_sh;
    riscv_RV32I           cpu1;
    riscv_RV32I           cpu2;
    DMA*                   dma;
    ADAPTOR*		  adaptor1;
    ADAPTOR*		  adaptor2;

    sc_signal <bool> inter1;
    sc_signal <bool> inter2;

  private:

    params paramsForCPU1() {
        params p;
        p.set("defaultsemihost", true);
        p.set("cpuid", (Uns32)0);
        return p;
    }

    params paramsForCPU2() {
        params p;
        p.set("defaultsemihost", true);
        p.set("cpuid", (Uns32)1);
        return p;
    }

}; /* DualCore */

DualCore::DualCore (sc_module_name name)
    : sc_module (name)

    , Platform("")
    // Four 1MB RAM's'
    , ram1(Platform, "ram1", 0xfffff)
    , ram2(Platform, "ram2", 0xfffff)
    , ram3(Platform, "ram3", 0xfffff)
    , ram4(Platform, "ram4", 0xfffff)

    , ram1_1(Platform, "ram1_1", 0xffbfffff)  //is unused
    , ram2_1(Platform, "ram2_1", 0xffbfffff)  //is unused

    , bus1(Platform, "bus1", 2, 3)      // Bus for CPU1
    , bus2(Platform, "bus2", 2, 3)      // Bus for CPU2
    , bus_sh(Platform, "bus_sh", 3, 3)  // Shared bus
    , cpu1(Platform, "cpu1", paramsForCPU1())
    , cpu2(Platform, "cpu2", paramsForCPU2())
{

    dma = new DMA("dma");
    dma->clk(clk);
    dma->rst_n(rst_n);



    adaptor1 = new ADAPTOR("ADAPTOR1");
    adaptor1->interrupt_in(inter1);
    dma->Interrupt1(inter1);


    adaptor2 = new ADAPTOR("ADAPTOR2");
    adaptor2->interrupt_in(inter2);
    dma->Interrupt2(inter2);

    adaptor1->interrupt_out(cpu1.MExternalInterrupt);
    adaptor2->interrupt_out(cpu2.MExternalInterrupt);


    // bus1 masters
    bus1.connect(cpu1.INSTRUCTION);  // RISC-V-1
    bus1.connect(cpu1.DATA);
    // bus1 slaves
    bus1.connect(ram1.sp1, 0x0, 0xfffff);  // RAM1
    bus1.connect(bus_sh);// Bridge
    // bus1.connect(bus_sh, 0x100000, 0x3fffff);// Bridge
    bus1.connect(ram1_1.sp1, 0x400000, 0xffffffff);

    // bus2 masters
    bus2.connect(cpu2.INSTRUCTION);  // RISC-V-2
    bus2.connect(cpu2.DATA);
    // bus2 slaves
    bus2.connect(ram2.sp1, 0x0, 0xfffff);  // RAM2
    bus2.connect(bus_sh);// Bridge
    // bus2.connect(bus_sh, 0x100000, 0x3fffff);// Bridge
    bus2.connect(ram2_1.sp1, 0x400000, 0xffffffff);

    // bus_sh slaves
    bus_sh.connect(ram3.sp1, 0x200000, 0x2fffff);  // RAM3
    bus_sh.connect(ram4.sp1, 0x300000, 0x3fffff);  // RAM4
    bus_sh.nextInitiatorSocket(0x100000, 0x1000ff)->bind(dma->SlavePort);

    // bus_sh masters
    dma->MasterPort(*bus_sh.nextTargetSocket());
    
    // DMA
    // dma = new DMA("dma");
    // dma->clk(clk);
    // dma->rst_n(rst_n);
    // dma->Interrupt1.bind(cpu1.MExternalInterrupt);
    // dma->Interrupt2.bind(cpu2.MExternalInterrupt);

    // dma->M_socket(*bus_sh.nextTargetSocket());
    // // bus_sh.nextInitiatorSocket(0x100000, 0x10001F)->bind(dma->SlavePort);    
}

int sc_main(int argc, char *argv[]) {
    // sc_clock clk("clk", 10, SC_NS);
    sc_time clkprd(10,SC_NS),clkDly(0,SC_NS);
    sc_clock clk("clk",clkprd,0.50,clkDly,false);
    sc_signal<bool> rst_n("rst");

    // start the CpuManager session
    session s;

    // create a standard command parser and parse the command line
    parser  p(argc, (const char **) argv);

    // create an instance of the platform
    DualCore top("top");
    top.clk(clk);
    top.rst_n(rst_n);

    rst_n.write(1);

    // start SystemC
    sc_start();

    cout << "Finished sc_main." << endl;
    return 0;
}

