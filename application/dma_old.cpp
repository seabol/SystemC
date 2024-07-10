#include "dma.h"
#define DEBUGMODE 1
using namespace sc_core;
using namespace sc_dt;
using namespace std;

void DMA::b_transport(tlm::tlm_generic_payload &trans, sc_time &delay) {
    tlm::tlm_command cmd = trans.get_command();
    sc_dt::uint64 addr = trans.get_address();
    unsigned char *data_ptr = trans.get_data_ptr();
    unsigned int len = trans.get_data_length();
    // unsigned char*   byt = trans.get_byte_enable_ptr();
	// unsigned int     wid = trans.get_streaming_width();

    union {
        unsigned char bytes[4];
        unsigned int value;
    } data;
		// #ifdef DEBUGMODE
		// 	cout << setw(10) << setfill(' ') << "DMA 314: " << sc_time_stamp() << endl;
		// #endif
    // Extract data from transaction
    for (unsigned int i = 0; i < 4 && i < len; ++i)
        data.bytes[i] = data_ptr[i];

    if (cmd == tlm::TLM_WRITE_COMMAND) {
        // Handle write command
        handleWriteCommand(addr, data.value);
    } else if (cmd == tlm::TLM_READ_COMMAND) {
        // Handle read command
        handleReadCommand(addr, data.value);
    }

    // Copy data back to transaction
    for (unsigned int i = 0; i < 4 && i < len; ++i)
        data_ptr[i] = data.bytes[i];

    // Set response status
    wait(10, SC_NS);
    trans.set_response_status(tlm::TLM_OK_RESPONSE);
}

void DMA::handleWriteCommand(sc_dt::uint64 addr, unsigned int value) {
    unsigned long long offset = addr;

    // Check state and handle write based on offset
    if (arbitrator_state == SERVE_CORE1 && offset < 0x10) {
        handleCore1Write(offset, value);
    } else if (arbitrator_state == SERVE_CORE2 && offset >= 0x10) {
        handleCore2Write(offset, value);
    } else {
        // Invalid address or state
        printf("[DMA] invalid write address (0x%llx) or state\n", offset);
    }
}

void DMA::handleCore1Write(unsigned long long offset, unsigned int value) {
    switch (offset) {
        case 0x00:
            SOURCE1 = value;
            break;
        case 0x04:
            TARGET1 = value;
            break;
        case 0x08:
            SIZE1 = value;
            break;
        case 0x0C:
            if (!interrupt2) {
                START1 = value;
                if (START1 == 1) {
                    arbitrator_state = SERVE_CORE1;  // Started
                }
            }
            break;
        default:
            printf("[DMA] invalid write address (0x%llx) for core 1\n", offset);
            break;
    }
}

void DMA::handleCore2Write(unsigned long long offset, unsigned int value) {
    switch (offset) {
        case 0x10:
            SOURCE2 = value;
            break;
        case 0x14:
            TARGET2 = value;
            break;
        case 0x18:
            SIZE2 = value;
            break;
        case 0x1C:
            if (!interrupt1) {
                START2 = value;
                if (START2 == 1) {
                    arbitrator_state = SERVE_CORE2;  // Started
                }
            }
            break;
        default:
            printf("[DMA] invalid write address (0x%llx) for core 2\n", offset);
            break;
    }
}

void DMA::handleReadCommand(sc_dt::uint64 addr, unsigned int value) {

    unsigned long long offset = addr;

    // Handle read command
    switch (offset) {
        case 0x00:
            value = SOURCE1;
            break;
        case 0x04:
            value = TARGET1;
            break;
        case 0x08:
            value = SIZE1;
            break;
        case 0x0C:
            value = START1;
            break;
        case 0x10:
            value = SOURCE2;
            break;
        case 0x14:
            value = TARGET2;
            break;
        case 0x18:
            value = SIZE2;
            break;
        case 0x1C:
            value = START2;
            break;
        default:
            printf("[DMA] invalid read address (0x%llx)\n", offset);
            break;
    }
}


void DMA::handleTransaction(tlm::tlm_generic_payload *trans, sc_time& delay, bool is_reading) {
    // tlm::tlm_generic_payload *trans = new tlm::tlm_generic_payload;
    MasterPort->b_transport(*trans, delay);
    // tlm::tlm_generic_payload *trans = new tlm::tlm_generic_payload;
    union {
        unsigned char bytes[4];
        unsigned int value;
    } data;

    unsigned char *data_ptr = trans->get_data_ptr();
    unsigned int len = trans->get_data_length();
    for (unsigned int i = 0; i < 4 && i < len; ++i)
    {
        data.bytes[i] = data_ptr[i];
    }


    if (is_reading) 
    {
        buffer = data.value;  // Store read data
    }

    is_reading = !is_reading;
}

void DMA::resetDMAState() {
    offset = 0;
    buffer = 0;
    is_reading = true;
    arbitrator_state = IDLE;
}

void DMA::setInterrupt() {
    if (arbitrator_state == SERVE_CORE1) {
        Interrupt1.write(1);
        interrupt1 = 1;
    } else if (arbitrator_state == SERVE_CORE2) {
        Interrupt2.write(1);
        interrupt2 = 1;
    }
}

void DMA::dma_proc() {
    baseAddr = DMA_BASE_ADDR;
    Interrupt1.write(0);
    Interrupt2.write(0);
    interrupt1 = 0;
    interrupt2 = 0;
    offset = 0;
    buffer = 0;
    is_reading = true;
    arbitrator_state = IDLE;

    while (true) {
        tlm::tlm_generic_payload *trans = new tlm::tlm_generic_payload;
        wait();

        unsigned int source = 0;
        unsigned int target = 0;
        unsigned int size = 0;

        // Determine which core/channel is currently active
        switch (arbitrator_state) {
            case SERVE_CORE1:
                source = SOURCE1;
                target = TARGET1;
                size = SIZE1;
                break;
            case SERVE_CORE2:
                source = SOURCE2;
                target = TARGET2;
                size = SIZE2;
                break;
            case DONE:
                // Check for completion and handle interrupts
                if (interrupt1 && !START1) {
                    Interrupt1.write(0);
                    interrupt1 = 0;
                    resetDMAState();
                } else if (interrupt2 && !START2) {
                    Interrupt2.write(0);
                    interrupt2 = 0;
                    resetDMAState();
                }
                continue; // Skip the rest of the loop
            default:
                continue; // Skip the rest of the loop
        }

        // Prepare transaction for DMA operation
        // tlm::tlm_generic_payload *trans;
        union {
            unsigned char bytes[4];
            unsigned int value;
        } data;
        sc_time delay = SC_ZERO_TIME;
        trans->set_data_length(4);
        trans->set_streaming_width(4);
        trans->set_byte_enable_ptr(0);
        trans->set_dmi_allowed(false);
        trans->set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);

        if (is_reading) 
        {
            // Read data from memory
            trans->set_command(tlm::TLM_READ_COMMAND);
            trans->set_address(source + offset);
            trans->set_data_ptr(data.bytes);

            handleTransaction(trans, delay, is_reading);
        } 
        else 
        {
            // Write data to memory
            // unsigned int writeData = buffer;

            data.bytes = buffer;
            trans->set_data_ptr(data.bytes);
            trans->set_command(tlm::TLM_WRITE_COMMAND);
            trans->set_address(target + offset);
            
            MasterPort->b_transport(*trans, delay);
            // handleTransaction(*trans, delay, is_reading);
            is_reading = !is_reading;

            if (offset + 4 >= size) {
                // writeData = buffer & 0xFFFFFFFF; // Only write the remaining bytes
                printf("[DMA] write Interrupt\n");
                setInterrupt(); // Set interrupt status
            }

            arbitrator_state = DONE; 
        } 


        // if (is_reading) {
        //     // Read data from memory
        //     trans->set_command(tlm::TLM_READ_COMMAND);
        //     trans->set_address(source + offset);
        //     // handleTransaction(*trans, delay, is_reading);
        //     handleTransaction(trans, delay, is_reading);
        // } else {
        //     // Write data to memory
        //     // unsigned int writeData = buffer;
        //     if (offset + 4 >= size) {
        //         // writeData = buffer & 0xFFFFFFFF; // Only write the remaining bytes
        //         printf("[DMA] write Interrupt\n");
        //         setInterrupt(); // Set interrupt status
        //     }

        //     // handleTransaction(*trans, delay, is_reading);
        //     handleTransaction(trans, delay, is_reading);
        // }
    }
}