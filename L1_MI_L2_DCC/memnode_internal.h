#ifndef MEMNODE_INT_H
#define MEMNODE_INT_H

#include <queue>
#include <iostream>
#include <systemc.h>
#include "mem.h"
#include "datatype_ch.h"
#include "Directory.h"
#include "mem_lsu.h"

using namespace std;

SC_MODULE(MEMNODE_INT) {
public:

	
	DIR dirmem ;
	MEM_LSU lsu;
	MEM ram;

	SC_CTOR(MEMNODE_INT): dirmem ("dir",0x0),lsu("LSU",0), ram("ram",NODE_WITH_SDRAM),data_from_l2_mem("data_from_l2tomem"),data_to_l2_mem("data_to_l2frommem"),clk("clk", 1, SC_NS) {
	bind_internals();

	}

	bool set_packet(int src_id, LisNocPacket* packet) { 
		bool ret = (lsu.set_packet(src_id,packet));
		if(ret == true) {
			//cout <<"MEMNODE_INT::"<<__func__<<" : from "<< packet->src_id << " to " <<packet->dst_id <<endl;
			}
		return ret;
		}
	void receive_packet(int src_id, LisNocPacket* packet) {lsu.receive_packet(src_id,packet);
		//cout <<"MEMNODE_INT::"<<__func__<<" : from "<< packet->src_id << " to " <<packet->dst_id <<endl;
		}
	void setinode(int id) { 
	lsu.setinode(id);
	dirmem.setinode(id);

	}
	void bind_internals (void);
private:

	//dir -- mem

	sc_signal <int> nodeaddr_from_lru; //into mem from dir
	sc_signal <int> node_address_to_lru; // out of mem

	//lsu to l2-dir-mem/l3
	
	// from-to dir
	sc_signal <int> nodeaddr_to_lru_dir; // all lru interactions with nodeaddress specified , /*TODO: if not done in */
	sc_signal <int> node_address_from_lru_dir;
	sc_signal <bool> inv_to_node_dir;  // dir can start an inv / 
	sc_signal <bool> spill_inv_to_node; 
	sc_signal <bool> dcc_transfer_en_to_npc_dir;
	sc_signal <bool> spill_inv_to_node_dir;
	sc_signal <int> inv_addr_to_node_dir;
	sc_signal <bool> forward_req_from_dir_dir;
	sc_signal <bool> forward_spillreq_from_dir_dir;
	sc_signal <bool> replace_req_dcc_ack_from_dir_dir;
	sc_signal <bool> cl_evt_info_to_dir_dir;
	sc_signal <bool> inv_ack_to_dir_dir;
	sc_signal <bool> flush_frm_npc_dir;
	sc_signal <int> req_count_to_dir_dir;
	sc_signal <int> address_from_dir_dir;
	sc_signal <int> address_to_dir_dir;
	sc_signal <int> req_count_from_dir_dir;
	sc_signal <bool> ex_to_dir_dir;
	sc_signal <bool> rd_wr_to_dir_dir;
	sc_signal <bool> ce_to_dir_dir;
	sc_signal <bool> ack_to_node_dir; // an ack to node 
	sc_signal <bool> rd_wr_to_l3_dir; // ce to l3 or mem (if no mem) whichever is available
	sc_signal <int> addr_to_l3_dir; //
	sc_signal <int> addr_from_l3_dir;
	sc_signal <int> req_count_to_l3_dir; //
	sc_signal <int> req_count_from_l3_dir; //
	sc_signal <int> dst_nodeaddr_to_l3_dir;
	sc_signal <bool> ce_to_l3_dir;
	sc_signal <bool> ack_from_l3_dir;

	// to-from mem

	sc_signal <int> nodeaddr_from_lru_dir_mem;
	sc_signal <int> nodeaddr_from_lru_l2_mem;
	sc_signal <int> node_address_to_lru_mem;

	// from dir
	sc_signal <bool> rd_wr_to_dir_mem;
	sc_signal <int> addr_from_dir_mem;
	sc_signal <int> addr_to_dir_mem;
	sc_signal <int> req_count_from_dir_mem;
	sc_signal <int> req_count_to_dir_mem;
	sc_signal <int> dst_nodeaddr_to_dir_mem;
	sc_signal <bool> ce_from_dir_mem;
	sc_signal <bool> ack_to_dir_mem;
	sc_signal <bool> without_dir_ack_mem;

	// from l2
	sc_signal <bool> ack_to_l2_mem;
	sc_signal <bool> ack_rw_to_l2_mem;
	sc_signal <int> address_from_l2_mem;
	sc_signal <int> address_to_l2_mem;
	sc_signal <int> req_count_from_l2_mem;
	sc_signal <int> req_count_to_l2_mem;
	sc_signal <bool> address_from_l2_en_mem;
	datatype_ch data_from_l2_mem;
	datatype_ch data_to_l2_mem;
	sc_clock clk;

};

#endif
