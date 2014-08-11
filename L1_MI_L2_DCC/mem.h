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
	
	sc_in <int> nodeaddr_from_lru_l2;
	sc_in <int> nodeaddr_from_lru_dir;
	sc_out <int> node_address_to_lru;

	// from dir
	sc_in <bool> rd_wr_from_dir;
	sc_in <int> addr_from_dir;
	sc_out <int> addr_to_dir;
	sc_in <int> req_count_from_dir;
	sc_out <int> req_count_to_dir;
	sc_in <int> dst_nodeaddr_from_dir;
	sc_in <bool> ce_from_dir;
	sc_out <bool> ack_to_dir;
	sc_in <bool> without_dir_ack;

	// from l2
	sc_out <bool> ack_to_l2;
	sc_out <bool> ack_rw_to_l2;
	sc_in <int> address_from_l2;
	sc_out <int> address_to_l2;
	sc_in <int> req_count_from_l2;
	sc_out <int> req_count_to_l2;
	sc_in <bool> address_from_l2_en;
	sc_port<write_read_if> data_from_l2;
	sc_port<write_read_if> data_to_l2;
	sc_in<bool> clk;
	SC_HAS_PROCESS(MEM);
	MEM(sc_module_name name, int id):sc_module(name),ack_rw_to_l2(0),ack_to_dir(0),ack_to_l2(0),inode(id),node_address_to_lru(0x0)
	{
		MEMptr = NULL;
		MEM_init();
		SC_METHOD(MEM_req_collector);
		sensitive << clk.pos(); //ce_from_dir << address_from_l2_en;
		SC_THREAD(MEM_self);  
		sensitive << clk.pos();
		mem_access_count_wr = 0x0;
		mem_access_count_rd = 0x0;

	}
long mem_access_count_wr;
long mem_access_count_rd;
void MEM_self(void);
void MEM_req_collector(void);
void MEM_init(void);
~MEM() {free(MEMptr);}
private:  
datatype * MEMptr;
int inode;
queue <NoCCommPacket> ReqQ1;
queue <NoCCommPacket> ReqQ2;
};


#endif
