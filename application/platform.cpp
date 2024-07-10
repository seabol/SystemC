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

#ifndef SC_INCLUDE_DYNAMIC_PROCESSES
#define SC_INCLUDE_DYNAMIC_PROCESSES
#endif

#include "tlm/tlmModule.hpp"
#include "tlm/tlmDecoder.hpp"
#include "tlm/tlmMemory.hpp"

// Processor configuration
#include "riscv.ovpworld.org/processor/riscv/1.0/tlm/riscv_RV32I.igen.hpp"


// DMA
#include "dma.h"

using namespace sc_core;

////////////////////////////////////////////////////////////////////////////////
//                      BareMetal Class                                       //
////////////////////////////////////////////////////////////////////////////////

class DualCore : public sc_module {

  public:
    DualCore (sc_module_name name);

    tlmModule             Platform;
    tlmDecoder            bus1;
    tlmDecoder            bus2;
    tlmDecoder            bus_sh;   // shared bus
    tlmRam                ram1;
    tlmRam                ram2;
    tlmRam                ram3;
    tlmRam                ram4;
    tlmRam                bus1_filler;  // filled up remained addr mapping of bus1
    tlmRam                bus2_filler;  // filled up remained addr mapping of bus2
    riscv_RV32I           cpu1;
    riscv_RV32I           cpu2;

    sc_in<bool>           clk;
    sc_in<bool>           rst_n;
    DMA                   dma;

  private:

    params paramsForcpu1() {
        params p;
        p.set("defaultsemihost", true);
        p.set("cpuid", (Uns32)0);
        return p;
    }

    params paramsForcpu2() {
        params p;
        p.set("defaultsemihost", true);
        p.set("cpuid", (Uns32)1);
        return p;
    }

}; /* DualCore */

DualCore::DualCore (sc_module_name name)
    : sc_module (name)
    , Platform ("")
    , bus1 (Platform, "bus1", 2, 3) // 2 target (CPU1.DATA, CPU1.INST) 3 initiator (ram1, bus1_filler, bus_sh)
    , bus2 (Platform, "bus2", 2, 3) // 2 target (CPU2.DATA, CPU2.INST) 3 initiator (ram1, bus2_filler, bus_sh)
    , bus_sh (Platform, "bus_sh", 3, 3) // 3 master (DMAtoMem, bus1, bus2) 3 slaves (ram3, ram4, cpu to dma), shared bus
    , ram1 (Platform, "ram1", 0xfffff)
    , ram2 (Platform, "ram2", 0xfffff)
    , ram3 (Platform, "ram3", 0xfffff)
    , ram4 (Platform, "ram4", 0xfffff)
    , bus1_filler (Platform, "bus1_filler", 0xffbfffff)
    , bus2_filler (Platform, "bus2_filler", 0xffbfffff)
    , cpu1 ( Platform, "cpu1",  paramsForcpu1())
    , cpu2 ( Platform, "cpu2",  paramsForcpu2())
    , dma ("dma")
{

    // bus1 initiator (connect to slaves)
    bus1.connect(ram1.sp1, 0x0, 0xfffff);
    bus1.connect(bus_sh);
    bus1.connect(bus1_filler.sp1, 0x400000, 0xffffffff);

    // bus1 target (connect to masters)
    bus1.connect(cpu1.INSTRUCTION);
    bus1.connect(cpu1.DATA);

    // bus2 initiator (connect to slaves)
    bus2.connect(ram2.sp1, 0x0, 0xfffff);
    bus2.connect(bus_sh);
    bus2.connect(bus2_filler.sp1, 0x400000, 0xffffffff);

    // bus2 target (connect to masters)
    bus2.connect(cpu2.INSTRUCTION);
    bus2.connect(cpu2.DATA);

    // bus_sh initiator (connect to slaves)
    bus_sh.connect(ram3.sp1, 0x200000, 0x2fffff);
    bus_sh.connect(ram4.sp1, 0x300000, 0x3fffff);
    bus_sh.nextInitiatorSocket(0x100000, 0x10001f)->bind(dma.SlavePort);

    // bus_sh target (connect to masters)
    // other 2 master bus1/bus2 is connected in previous lines
    dma.MasterPort(*bus_sh.nextTargetSocket());

    dma.clk(clk);
    dma.rst_n(rst_n);
    dma.Interrupt1.bind(cpu1.MExternalInterrupt);
    dma.Interrupt2.bind(cpu2.MExternalInterrupt);
}

int sc_main (int argc, char *argv[]) {
    sc_clock clk("clk", 10, SC_NS);
    sc_signal<bool> rst_n("rst");

    // start the CpuManager session
    session s;

    // create a standard command parser and parse the command line
    parser  p(argc, (const char**) argv);

    // create an instance of the platform
    DualCore top ("top");
    top.clk(clk);
    top.rst_n(rst_n);

    rst_n.write(1);

    // start SystemC
    sc_start();
    cout << "Finished sc_main." << endl;
    return 0;
}

