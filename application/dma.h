
#ifndef _DMA_H
#define _DMA_H

#define DMA_BASE_ADDR 0x100000
#define PERIOD 10

#define IDLE  0
#define SERVE_CORE1_SET 1
#define SERVE_CORE2_SET 2
#define SERVE_CORE1_PROCESS 3
#define SERVE_CORE2_PROCESS 4
#define SERVE_CORE1_DONE 5
#define SERVE_CORE2_DONE 6
#define DONE 10

#include "systemc.h"
#include "tlm.h"
#include "tlm_utils/simple_initiator_socket.h"
#include "tlm_utils/simple_target_socket.h"

using namespace sc_core;
using namespace sc_dt;
using namespace std;
#include <iomanip> //for debug



SC_MODULE(DMA) {
    sc_in<bool> clk;
    sc_in<bool> rst_n;  // active-low reset

    tlm_utils::simple_target_socket<DMA> SlavePort;      // slave port
    tlm_utils::simple_initiator_socket<DMA> MasterPort;  // master port

    tlm::tlm_analysis_port<unsigned int> Interrupt1;  // 1-bit Interrupt pin for channel 1
    tlm::tlm_analysis_port<unsigned int> Interrupt2;  // 1-bit Interrupt pin for channel 2

    sc_uint<32> baseAddr;  // static address register
    unsigned char data_m;
    unsigned int R_data;
    unsigned int data;//assign to reg
    bool clear;	 


    sc_uint<32> SOURCE1;  // 0x00
    sc_uint<32> TARGET1;  // 0x04
    sc_uint<32> SIZE1;    // 0x08
    sc_uint<32> START1;   // 0x0C

    sc_uint<32> SOURCE2;  // 0x10
    sc_uint<32> TARGET2;  // 0x14
    sc_uint<32> SIZE2;    // 0x18
    sc_uint<32> START2;   // 0x1C


    void dma_proc();  // main DMA function
    void b_transport(tlm::tlm_generic_payload&, sc_time&);

    SC_CTOR(DMA) : SlavePort("SlavePort"), MasterPort("MasterPort") { 
        SlavePort.register_b_transport(this, &DMA::b_transport);
        SC_CTHREAD(dma_proc, clk.pos());      // posedge triggered function
        reset_signal_is(rst_n,false);

    }

private:
    // Private member functions
    void handleWriteCommand(sc_dt::uint64 addr, unsigned int value);
    void handleCore1Write(unsigned long long offset, unsigned int value);
    void handleCore2Write(unsigned long long offset, unsigned int value);


    // Member variables
    int arbitrator_state;
    bool interrupt1, interrupt2;
    unsigned int offset;

};

#endif
