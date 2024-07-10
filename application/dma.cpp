#include "dma.h"
// #define DEBUGMODE 1
// #define INFO 1 
using namespace sc_core;
using namespace sc_dt;
using namespace std;

void DMA::b_transport(tlm::tlm_generic_payload &trans, sc_time &delay) {
    tlm::tlm_command cmd_s = trans.get_command();
    sc_dt::uint64 addr_s = trans.get_address();
    unsigned char *data_ptr_s = trans.get_data_ptr();


    #ifdef DEBUGMODE
	cout << "-----------------------------------------" << endl;
	cout << ">> now start transaction (CPU and DMA) " << endl;
	cout << setw(10) << setfill(' ') << sc_time_stamp() << endl;
	cout << setw(20) << setfill(' ') <<' ';
	cout << "Command(R/W) : " << (cmd_s ? "write" : "read") << endl;
	cout << setw(20) << setfill(' ') <<' ';
	cout << "Address : 0x" << setw(8) << setfill('0') << hex << addr_s << endl;
	#endif

    data = *(reinterpret_cast<int*>(data_ptr_s));

    if (cmd_s == tlm::TLM_WRITE_COMMAND) 
    {
        // Handle write command

        #ifdef DEBUGMODE
		cout << setw(20) << setfill(' ') <<' ';
		cout << "data: 0x" << setfill('0');
		cout << hex << *(reinterpret_cast<int*>(data_ptr_s)) << endl;
		#endif

        handleWriteCommand(addr_s, data);
    } 
    else if (cmd_s == tlm::TLM_READ_COMMAND)
    {
        // Handle read command

        #ifdef INFO
		cout << setw(20) << setfill(' ') <<' ';
		cout << "data: 0x" << setfill('0');
		cout << hex << *(reinterpret_cast<int*>(data_ptr_s)) << endl;
		#endif
    }

    // Set response status
    wait(10, SC_NS);
    trans.set_response_status(tlm::TLM_OK_RESPONSE);
}

void DMA::handleWriteCommand(sc_dt::uint64 addr, unsigned int value) {
    unsigned long long offset = addr;


    #ifdef DEBUGMODE

    cout << setw(20) << setfill(' ') <<' ';
    cout << "offset: 0x" << setfill('0');
    cout << hex << offset << endl;
    cout << "value: 0x" << value << endl;
   
    #endif

    // Check state and handle write based on offset
    if (offset < 0x10) 
    {
        handleCore1Write(offset, value);
    } 
    else if ( offset >= 0x10) 
    {
        handleCore2Write(offset, value);
    } 
    else 
    {
        #ifdef DEBUGMODE
        cout << "89 [DMA] invalid write address"<< setfill('0');
        cout << "0x" << setfill('0');
        cout << hex << offset << endl;
        #endif
        // Invalid address or state
        // printf("[DMA] invalid write address (0x%llx) or state\n", offset);
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
            // if (!interrupt2) 
            if(interrupt2 == 0)
            {
                // START1 = value;
                if (value == 1) 
                {
                    START1 = value;
                    arbitrator_state = SERVE_CORE1_SET;  // Started
                }
                else if((value == 0 && START1 == 1) && interrupt1 == 1)
                {
                    //clear
                    Interrupt1.write(0);
                    interrupt1 = 0;
                    clear = 1;
                    arbitrator_state = IDLE;

                    #ifdef DEBUGMODE
                    cout << "clear = 1"<< endl;
                    #endif

                }
            }
            else
            {
                #ifdef DEBUGMODE
                cout << ">> Interrupt2 raise " << endl;
                #endif
            }
            break;
        default:

            #ifdef DEBUGMODE
            cout << "128 [DMA] invalid write address"<< setfill('0');
            cout << "0x" << setfill('0');
            cout << hex << offset << endl;
            #endif
            // printf("[DMA] invalid write address (0x%llx) for core 1\n", offset);
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
            if(interrupt1 == 0)
            {
                // START1 = value;
                if (value == 1) 
                {
                    START2 = value;
                    arbitrator_state = SERVE_CORE2_SET;  // Started
                }
                else if((value == 0 && START2 == 1) && interrupt2 == 1)
                {
                    //clear
                    Interrupt2.write(0);
                    interrupt2 = 0;
                    clear = 1;
                    arbitrator_state = IDLE;

                    #ifdef DEBUGMODE
                    cout << "clear = 1"<< endl;
                    #endif

                }
            }
            else
            {
                #ifdef DEBUGMODE
                cout << ">> Interrupt1 raise " << endl;
                #endif
            }
            break;
        default:
            #ifdef DEBUGMODE
            cout << "167 [DMA] invalid write address"<< setfill('0');
            cout << "0x" << setfill('0');
            cout << hex << offset << endl;
            #endif
            // printf("[DMA] invalid write address (0x%llx) for core 2\n", offset);
            break;
    }
}







void DMA::dma_proc() {
    int i, count;
    sc_uint<32> temp_source, temp_target;
    baseAddr = DMA_BASE_ADDR;
    Interrupt1.write(0);
    Interrupt2.write(0);
    interrupt1 = 0;
    interrupt2 = 0;
    offset = 0;
    // buffer = 0;

    temp_source = 0;
	temp_target = 0;
    arbitrator_state = IDLE;
    clear = 0;

    while (true) {
        tlm::tlm_generic_payload *trans_m = new tlm::tlm_generic_payload;
        sc_time delay = sc_time(10,SC_NS);
        wait();

        unsigned int source = 0;
        unsigned int target = 0;
        unsigned int size = 0;

        // Determine which core/channel is currently active
        switch (arbitrator_state) 
        {
            case IDLE:

                break;

            case SERVE_CORE1_SET:


                #ifdef INFO
                cout << "-----------------------------------------" << endl;
                cout << ">> SERVE_CORE1_SET " << endl;
                #endif

                source = SOURCE1;
                target = TARGET1;
                size = SIZE1;
                arbitrator_state = SERVE_CORE1_PROCESS;
                break;

            case SERVE_CORE1_PROCESS:

                break;

            case SERVE_CORE2_SET:

                #ifdef INFO
                cout << "-----------------------------------------" << endl;
                cout << ">> SERVE_CORE2_SET " << endl;
                #endif

                source = SOURCE2;
                target = TARGET2;
                size = SIZE2;
                arbitrator_state = SERVE_CORE2_PROCESS;
                break;

            case SERVE_CORE2_PROCESS:

                break;

            
            case SERVE_CORE1_DONE:

                #ifdef DEBUGMODE
                    cout << "-------------------------------" << endl;
                    cout << setw(10) << setfill(' ') << "DMA 367: " << sc_time_stamp() << endl;
                    cout << setw(10) << setfill(' ') << "Interrupt 1  raise " << endl;
                    cout << "-------------------------------" << endl;
                #endif
                Interrupt1.write(1);
                interrupt1 = 1 ; 
                break;


            case SERVE_CORE2_DONE:

                #ifdef DEBUGMODE
                    cout << "-------------------------------" << endl;
                    cout << setw(10) << setfill(' ') << "DMA 381: " << sc_time_stamp() << endl;
                    cout << setw(10) << setfill(' ') << "Interrupt 2  raise " << endl;
                    cout << "-------------------------------" << endl;
                #endif

                Interrupt2.write(1);
                interrupt2 = 1 ; 
                break;


            default:
                // continue; // Skip the rest of the loop
                #ifdef DEBUGMODE
				cout << setw(10) << setfill(' ') << "arbitrator_state" << i << "	   "<< sc_time_stamp() << endl;
				cout << setw(10) << setfill(' ') << "default " << sc_time_stamp() << endl;
				#endif
                break;
        }

        // Prepare transaction for DMA operation
        // tlm::tlm_generic_payload *trans;
        // union {
        //     unsigned char bytes[4];
        //     unsigned int value;
        // } data;
        // sc_time delay = SC_ZERO_TIME;


        if((arbitrator_state == SERVE_CORE1_PROCESS || arbitrator_state == SERVE_CORE2_PROCESS)  && size != 0) 
		{

            temp_source = source;
			temp_target = target;
			
			#ifdef DEBUGMODE
			cout << endl;
			cout << endl;
			cout << "-----------------------------------------" << endl;
			cout << ">> start mem transfer " << endl;
			cout << "size : " << size << endl;
			cout << "source : " << temp_source << endl;
			cout << "target : " << temp_target << endl;
			#endif

			for(i = size; i > 0; i = i - 4) 
			{
				count = 0;
				while(count < 2) 
				{
					if(count == 0) 
					{
                        #ifdef DEBUGMODE
						cout << "-------------------------------" << endl;
						cout << ">> DMA read data from source memory" << endl;
                        #endif
                        // trans_m.set_command(tlm::TLM_READ_COMMAND);
						// trans_m.set_address(temp_source);
						// trans_m.set_data_ptr( reinterpret_cast<unsigned char*>(&data_m) );
						// trans_m.set_data_length(4);
						// trans_m.set_streaming_width(4);
						// trans_m.set_byte_enable_ptr(0);
						// trans_m.set_dmi_allowed(false);
						// trans_m.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);

						trans_m->set_command(tlm::TLM_READ_COMMAND);
						trans_m->set_address(temp_source);
						trans_m->set_data_ptr( reinterpret_cast<unsigned char*>(&data_m) );
						trans_m->set_data_length(4);
						trans_m->set_streaming_width(4);
						trans_m->set_byte_enable_ptr(0);
						trans_m->set_dmi_allowed(false);
						trans_m->set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);
						MasterPort->b_transport(*trans_m, delay );
					} 
					else if(count == 1) 
					{
                        #ifdef DEBUGMODE
						cout << "-------------------------------" << endl;
						cout << ">> DMA write data to target memory" << endl;
                        #endif
                        // trans_m.set_command(tlm::TLM_WRITE_COMMAND);
						// trans_m.set_address(temp_target);
						// trans_m.set_data_ptr( reinterpret_cast<unsigned char*>(&data_m) );
						// trans_m.set_data_length(4);
						// trans_m.set_streaming_width(4);
						// trans_m.set_byte_enable_ptr(0);
						// trans_m.set_dmi_allowed(false);
						// trans_m.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);
						trans_m->set_command(tlm::TLM_WRITE_COMMAND);
						trans_m->set_address(temp_target);
						trans_m->set_data_ptr( reinterpret_cast<unsigned char*>(&data_m) );
						trans_m->set_data_length(4);
						trans_m->set_streaming_width(4);
						trans_m->set_byte_enable_ptr(0);
						trans_m->set_dmi_allowed(false);
						trans_m->set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);
						MasterPort->b_transport(*trans_m, delay);
					}
					count = count + 1;
					R_data = *(reinterpret_cast<int*>(&data_m));
					#ifdef DEBUGMODE
					cout << "addr_source = " << temp_source << ",	addr_target = " << temp_target <<endl;
					cout << "data = " << R_data << endl;
                    cout << "i = " << i << endl;
					#endif
					if(i == 4 && count == 2) 
					{
                        
                        if (arbitrator_state == SERVE_CORE1_PROCESS)
                        {
                            #ifdef INFO
                            cout << "-----------------------------------------" << endl;
                            cout << ">> SERVE_CORE1_DONE " << endl;
                            #endif

                            arbitrator_state = SERVE_CORE1_DONE;
                        } 
                        else if ( arbitrator_state == SERVE_CORE2_PROCESS)
                        {

                            #ifdef INFO
                            cout << "-----------------------------------------" << endl;
                            cout << ">> SERVE_CORE2_DONE " << endl;
                            #endif

                            arbitrator_state = SERVE_CORE2_DONE;
                        }
					}
					wait();
				}
				temp_source = temp_source + 4;
				temp_target = temp_target + 4;
				#ifdef DEBUGMODE
				cout << setw(10) << setfill(' ') << "Rest Data size (i) : " << i << "	   "<< sc_time_stamp() << endl;
				cout << setw(10) << setfill(' ') << "DMA 410: " << sc_time_stamp() << endl;
				#endif
				// if(i == 4 && count == 2) 
				// {
				// 	#ifdef DEBUGMODE
				// 		cout << "-------------------------------" << endl;
				// 		cout << setw(10) << setfill(' ') << "DMA 164: " << sc_time_stamp() << endl;
				// 		cout << setw(10) << setfill(' ') << "Interrupt raise " << endl;
				// 		cout << "-------------------------------" << endl;
				// 	#endif
				// 	interrupt.write(1);
				// }

            }

        }
        else if (arbitrator_state == IDLE)
        {
            if(clear == 1)
            {
                SOURCE1 = 0;
                TARGET1 = 0;
                SIZE1= 0;
                START1= 0;

                SOURCE2= 0;
                TARGET2= 0;
                SIZE2= 0;
                START2= 0;

                temp_source = 0;
                temp_target = 0;
                clear = 0; 
            }


        }
        // wait();
    }
}