#ifndef MEM_H
#define MEM_H

#include<systemc.h>
#include <queue>
#include <iostream>
#include "write_read_if.h" 
#include "datatype_ch.h"
#include "LSU_defines.h"


using namespace std;

SC_MODULE(MEM)
{
public:
	
	sc_in <int> nodeaddr_from_lru;
	sc_out <int> node_address_to_lru;

	// from llc
	sc_in <bool> rd_wr_;
	sc_out <int> addr_from_mem;
	sc_in <int> addr_to_mem;
	sc_in <int> req_count_to_mem;
	sc_out <int> req_count_from_mem;
	sc_in <bool> ce_;	
	sc_port<write_read_if> data_;

	// to llc
	sc_out <bool> ack_;
	sc_out <bool> ack_rw;
	sc_in<bool> clk;
	SC_HAS_PROCESS(MEM);
	MEM(sc_module_name name, int id):sc_module(name),ack_(0),inode(id),node_address_to_lru(0x0)
	{
		MEMptr = NULL;
		MEM_init();
		SC_THREAD(MEM_req_collector);
		sensitive << clk.pos();
		SC_THREAD(MEM_self);  
		sensitive << clk.pos();
		mem_access_count_wr = 0x0;
		mem_access_count_rd = 0x0;

	}
long mem_access_count_rd;
long mem_access_count_wr;
void MEM_self(void);
void MEM_req_collector(void);
void MEM_init(void);
~MEM() {free(MEMptr);}
private:  
datatype * MEMptr;
int inode;
queue <NoCCommPacket> ReqQ1;
};


#endif
