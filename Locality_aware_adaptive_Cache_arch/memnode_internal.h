#ifndef MEMNODE_INT_H
#define MEMNODE_INT_H

#include <queue>
#include <iostream>
#include <systemc.h>
#include "mem.h"
#include "datatype_ch.h"
#include "Directory.h"
#include "LLC.h"
#include "mem_lsu.h"

using namespace std;

SC_MODULE(MEMNODE_INT) {
public:

	
	DIR dirmem ;
	MEM_LSU lsu;
	MEM ram;
	LLC llc;

	SC_CTOR(MEMNODE_INT): dirmem ("dir",0x0),lsu("LSU",0), ram("ram",NODE_WITH_SDRAM),data_to_llc("mem_data_to_llc"),data_from_llc("mem_data_from_llc"),clk("clk", 1, SC_NS),llc("memllc",0x0,true), data_to_mem_llc("mem_node_data_to_mem_llc"), data_mem("mem_data_mem") {
	bind_internals();

	}

	bool set_packet(int src_id, LisNocPacket* packet) { 
		bool ret = (lsu.set_packet(src_id,packet));
		if(ret == true){
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
	llc.setinode(id);
	}
	void bind_internals (void);
private:

//LLC interaction with LSU
	sc_signal <int> nodeaddr_to_lru_llc;
	sc_signal <int> node_address_from_lru_llc;

	// dir interaction 
	sc_signal <bool> rd_wr_llc;
	sc_signal <int> addr_from_dir_to_llc;
	sc_signal <int> addr_to_dir_to_llc;
	sc_signal <int> req_count_from_dir_to_llc;
	sc_signal <int> req_count_to_dir_to_llc;
	sc_signal <int> dst_nodeaddr_llc; // _ce cud be enabled, addr_en is part of this
	sc_signal <bool> ce_llc;
	sc_signal <bool> ack_to_dir_llc;	
	sc_signal <bool> without_dir_ack_llc;
	// npc
	sc_signal <int> addr_from_npc_to_llc;
	sc_signal <int> addr_to_npc_to_llc;
	sc_signal <int> req_count_from_npc_to_llc;
	sc_signal <int> req_count_to_npc_to_llc;
	sc_signal <bool> addr_en_from_npc_to_llc; 
	datatype_ch data_to_llc;
	datatype_ch data_from_llc;
	sc_signal <bool> ack_to_npc_llc;
	sc_signal <bool> ack_rw_to_npc_llc;

	// mem
	sc_signal <int> addr_to_mem_llc;
	sc_signal <int> addr_from_mem_llc;
	sc_signal <int> req_count_to_mem_llc;
	sc_signal <int> req_count_from_mem_llc;
	sc_signal <bool> addr_en_to_mem_llc;
	sc_signal <bool> r_w_to_mem_llc;
	sc_signal <bool> trans_ack_from_mem_llc; 
	sc_signal <bool> ack_rw_from_mem_llc;
	datatype_ch data_to_mem_llc;

//dir lsu interactions


	//lsu to npc-dir-mem/llc
	
	// from-to dir
	sc_signal <int> nodeaddr_to_lru_dir; // all lru interactions with nodeaddress specified , 
	sc_signal <int> node_address_from_lru_dir;
	sc_signal <bool> inv_to_node_dir;  // dir can start an inv / 
	sc_signal <int> inv_addr_to_node_dir;
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
	sc_signal <bool> rd_wr_to_llc_dir; // ce to llc or mem (if no mem) whichever is available
	sc_signal <int> addr_to_llc_dir; //
	sc_signal <int> addr_from_llc_dir; //
	sc_signal <int> req_count_to_llc_dir; //
	sc_signal <int> req_count_from_llc_dir; //
	sc_signal <int> dst_nodeaddr_to_llc_dir;
	sc_signal <bool> ce_to_llc_dir;
	sc_signal <bool> ack_from_llc_dir;
	sc_signal <bool> mode_pri_remote_to_llc_dir;
	sc_signal <bool> mode_pri_remote_f_dir_llc;
	sc_signal <bool> mode_pri_remote_to_npc_llc;
	sc_signal <int> priv_utiliz_to_dir;
	sc_signal <bool> mode_pri_remote_from_dir_dir;

#if 1 // tofrom mem 
	sc_signal <int> nodeaddr_to_lru_mem; // is not taken care, when memlsu is made there shud b a single thread given for mem transactions
	sc_signal <int> node_address_from_lru_mem;

	sc_signal <int> addr_to_mem;
	sc_signal <int> addr_from_mem;
	sc_signal <int> req_count_to_mem;
	sc_signal <int> req_count_from_mem;
	sc_signal <bool> addr_en_to_mem;
	sc_signal <bool> r_w_to_mem;
	sc_signal <bool> trans_ack_from_mem; 
	sc_signal <bool> ack_rw_from_mem;
	datatype_ch data_mem;
#endif


	sc_clock clk;

};

#endif
