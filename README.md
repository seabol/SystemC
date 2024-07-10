# SystemC
Using TLM2.0 Library
## Abstract
Explore OVP SystemC, TLM 2.0 and Multicore examples/demos. Then evolve from these examples to port two (2) RISC-V cores onto the Imperas M*SDK/OVP platform, and exchange data using DMA on shared memory.

Please read carefully. All outputs required are described in the text. Five (5) points will be taken for each missing required output and behavior.
## Description
This is an individual project with all classmates sharing findings on the class Line group. An individual project with group efforts, so to speak.

### PART I
#### 1.There are many examples and demos in the M*SDK/OVP package, i.e. $IMPERAS_HOME/Examples and $IMPERAS_HOME/Demos. You will find SystemC, TLM 2.0, single-core and multi-core examples in these folders.

For example$IMPERAS_HOME/Demo/Processors/RISCV/riscv32/RV32G/single_core

#### 2.You cannot execute these examples in $IMPERAS_HOME. Please copy over to your home directory to play around with them. Fine examples that can help you do PART II.
### Figure 1 Target Architecture
![image](https://github.com/seabol/SystemC/assets/68637611/4f796d1a-689b-4d39-9d07-7199fdf918a0)

### PART II
#### 1. First put two (2) RISC-V cores, four (4) 1MB RAM’s, a DMA, and three (3) TLM 2.0 busses into the system, addressed and connected as described in the above figure.

#### 2. The DMA needs to be modified out of the DMA from Project 1, with two (2) interrupt output ports, INT1 and INT2. And there are two (2) sets of control registers, one set is addressed between 0x0 and 0xF and the second set is addressed between 0x10 and 0x1F. The first set is used by RISC-V-1 and the second by RISC-V-2. And we have only one (1) slave port so an arbitrator is needed to arbitrate which RISC-V to program and use the DMA. The First-Come-First-Serve policy is used. Once the DMA decides which RISC-V to serve, it is remained occupied until the end of the transmission and the CLEAR control message from the owner RISC-V is received.

#### 3. At system initialization, load RISC-V-1 and RISC-V-2 programs onto RAM1 and RAM2 respectively in a bare metal fashion. And reset all cells in RAM3 and RAM4 to 0, then put designated data into them, as described below.

#### 4. The RISC-V-1 and RISC-V-2 bare metal programs execute following data exchange actions:

  A. At initialization, RISC-V-1 writes 1KB data into 0x200400 to 0x2007FF in a repetition of “0123456789ABCDEF” without quotes, i.e. “0123456789ABCDEF0123456789ABCDEF01…”.

  B. At initialization, RISC-V-2 writes 1KB data into 0x300400 to 0x3007FF in a repetition of “FEDCBA9876543210” without quotes, i.e. “FEDCBA9876543210FEDCBA9876543210FE…”.

  C. RISC-V-1 monitors the data at address 0x200800. When the data is 0x0, RISC-V-1 programs DMA’s 1st set of control registers to copy a 1KB data starting from 0x200400 (on RAM3) to 0x300000 (on RAM4).

  D. After the 1KB data is transmitted, RISC-V-1 writes a flag 0x1 to address 0x200800 to indicate the end of data transmission.

  E. RISC-V-2 monitors the data at address 0x200800. When the data is 0x1, RISC-V-2 programs DMA’s 2nd set of control registers to move a 1KB data starting at 0x300400 (on RAM4) to 0x200000 (on RAM3).

  F. After the 1KB data is transmitted, RISC-V-2 then writes a flag 0x0 to address 0x200800 to indicate the end of data transmission.

  G. Repeat above data exchange/transmission three (3) times and stop.

#### 5.Properly use printf (please refer to the “Hello World” example) to show the progress of RISC-V-1 and RISC-V-2 programs.

#### 6.Write a less than 5-page report to describe how you implement the system and the interrupt synchronization.

#### 7.Submit your work to Moodle before the project deadline and run a demo in the SoC Lab. In the demo you need to demonstrate your work using M*SDK GUI to view RISC-V code execution and bus transactions.

