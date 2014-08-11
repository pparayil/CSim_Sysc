
#ifndef __LISNOCPROCESSINGELEMENT_H__
#define __LISNOCPROCESSINGELEMENT_H__

#include <queue>
#include <fstream>
#include <systemc.h>
#include "LiSNoC_defines.h"
#include "memnode_internal.h"
#include "node_internal.h"
#include "verilated_sc.h"
#include "verilated.h"

#define BUSWIDTH 64

using namespace std;

SC_MODULE(LisNoCProcessingElement)
{
    
    	// I/O Ports
	sc_in<bool>	clk;
	sc_in<bool>	rst;
	sc_out<vluint64_t>	link_in_flit_i;
	sc_in<vluint64_t>	link_out_flit_o;
	sc_out<sc_bv<2> >	link_in_valid_i;
	sc_in<sc_bv<2> >	link_in_ready_o;
	sc_in<sc_bv<2> >	link_out_valid_o;
	sc_out<sc_bv<2> >	link_out_ready_i;
#if 0
	sc_in < LisNocFlit > flit_rx;	// The input channel
	sc_in < bool > req_rx;	// The request associated with the input channel
	sc_out < bool > ack_rx;	// The outgoing ack signal associated with the input channel
	sc_out < LisNocFlit > flit_tx;	// The output channel
	sc_out < bool > req_tx;	// The request associated with the output channel
	sc_in < bool > ack_tx;	// The outgoing ack signal associated with the output channel
	bool current_level_rx;	// Current level for Alternating Bit Protocol (ABP)
	bool current_level_tx;	// Current level for Alternating Bit Protocol (ABP)
#endif
	queue < LisNocPacket > packet_queue;	// Local queue of packets

	// Registers
	int local_id;		// Unique identification number

	bool transmittedAtPreviousCycle;	// Used for distributions with memory

	// Functions
	void rxpacketizer(void);
	void rxProcess(void);		// The receiving process
	void txProcess(void);		// The transmitting process
	bool canShot(LisNocPacket & packet);	// True when the packet must be shot
	vluint64_t nextFlit(bool vc);	// Take the next flit of the current packet

	NODE_INT * node_internals;
	MEMNODE_INT * memnode_internals;
	long total_noc_count_in (void);
	long total_noc_count_out (void);
	static int read, write,internal;
	
	// Constructor
	SC_HAS_PROCESS(LisNoCProcessingElement);
	LisNoCProcessingElement(sc_module_name nm, bool mem_node, unsigned inode, const char* fname) : sc_module(nm),delay_counter(0),node_type(mem_node),local_id(inode)
	{
		SC_METHOD(rxProcess);
		sensitive << rst;
		sensitive << clk.pos();

		SC_THREAD(rxpacketizer);
		sensitive << clk.pos();

		SC_METHOD(txProcess);
		sensitive << rst;
		sensitive << clk.pos();                             
	    	
		if(mem_node == true){		
			memnode_internals = new MEMNODE_INT("memorynode");
			node_internals = NULL;
			memnode_internals->setinode(inode);
		}
		else
		{
			memnode_internals = NULL;
			node_internals = new NODE_INT("node");	
			node_internals->setinode(inode, fname);		
		}

		/*LisNoCProcessingElement::write = 0x0;
		LisNoCProcessingElement::read = 0x0;
		LisNoCProcessingElement::internal = 0x0;*/
		flit_filled = false;
		flit_tmp = 0x0;		
		noc_packet_counter_in = 0x0;		
		noc_packet_counter_out = 0x0;
		noc_in_done = true;		
		noc_out_done = true;
		
	}
	~LisNoCProcessingElement() { 
		/*cout<<"No of reads: "<<read<<endl;
		cout<<"No of writes: "<<write<<endl;*/
		delete node_internals;  
		delete memnode_internals;
	}
	int inode;
	
private:

	int delay_counter;
	bool node_type;	
	bool flip;
	vluint64_t flit_tmp;
	bool flit_filled;
	long noc_packet_counter_in;
	long noc_packet_counter_out;	
	bool noc_in_done;
	bool noc_out_done;
};



#endif //LISNOCPROCESSINGELEMENT
