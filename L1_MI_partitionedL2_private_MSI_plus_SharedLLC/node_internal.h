#ifndef NODE_INT_H
#define NODE_INT_H

#include <queue>
#include <iostream>
#include <systemc.h>
#include "Req_generator_Prsr.h"
#include "Directory.h"
#include "LLC.h"
#include "common_lsu.h"
#include "NPC.h"
#include "PRIV_L1.h"

using namespace std;

SC_MODULE(NODE_INT) {
public:

	
	PR_REQ_GEN pr0;
	DIR dirl2 ;
	NPC l2;
	LLC llc;
	CM_LSU lsu;
	PRIV_L1 l1;
	SC_CTOR(NODE_INT): pr0("Pr0", 0), dirl2 ("dir",0x0),l2("l2",0x0), lsu("LSU",0),datachannel_1("ch1"),datachannel_2("ch2"),data_from_l2("data_from_l2tollc"),data_to_l2("data_to_l2fromllc"),clk("clk", 1, SC_NS),l1("level1cache",0),llc("CM_llc",0x0,true),data_to_llc("data_to_llc"),data_from_llc("data_from_llc"), data_to_mem_llc("mnode_data_to_mem_llc"), data_to_l2_l1("l1_to_l2_data"),data_from_l2_l1("l2_to_l1_data") {
	bind_internals();

	}

	bool set_packet(int src_id, LisNocPacket* packet) { 
		bool ret = (lsu.set_packet(src_id,packet));
		if(ret == true) {
			//cout <<"NODE_INT::"<<__func__<<" : from "<< packet->src_id << " to " <<packet->dst_id <<endl;
			}
		return ret;
		}
	void receive_packet(int src_id, LisNocPacket* packet) {lsu.receive_packet(src_id,packet);
		//cout <<"NODE_INT::"<<__func__<<" : from "<< packet->src_id << " to " <<packet->dst_id <<endl;
		}
	void setinode(int id, const char * fname) { 
	lsu.setinode(id);
	dirl2.setinode(id);
	l2.setinode(id);
	llc.setinode(id);
	pr0.setnode(id,fname);
	l1.setinode(id);
	}
	void bind_internals (void);
private:
	datatype_ch data_from_l2;//noc
	datatype_ch data_to_l2;// noc
	datatype_ch datachannel_1;
	datatype_ch datachannel_2;
	
	datatype_ch data_to_l2_l1;	
	datatype_ch data_from_l2_l1;
	
	//processor - l1
	sc_signal< int > address_1;
	sc_signal<bool> ce;
	sc_signal<bool> we_1;
	sc_signal< int > address_2;
	sc_signal<bool> we_2;
	sc_signal<bool> ack;	
	sc_signal< int > req_count_2;
	sc_signal< int > req_count_1;
	sc_signal<bool> status_db;
	
	//	between l1-l2
	sc_signal <bool> ce_to_l2_l1;
	sc_signal <bool> rd_wr_to_l2_l1;
	sc_signal <int> address_to_l2_l1;
	sc_signal <int> req_count_to_l2_l1;
	sc_signal <bool> ack_to_l2_l1;	
	
	sc_signal <int> req_count_from_l2_l1;
	sc_signal <int> address_from_l2_l1;	
	sc_signal <bool> ack_from_l2_l1;
	sc_signal <bool> ack_we_l2_l1;
	sc_signal <bool> inv_from_l2_l1;
	
	//dir -- mem

	sc_signal <int> nodeaddr_from_lru; //into mem from dir
	sc_signal <int> node_address_to_lru; // out of mem


	// from l2 to llc
	sc_signal <bool> ack_to_l2;
	sc_signal <int> address_from_l2;
	sc_signal <int> address_to_l2;
	sc_signal <int> req_count_from_l2;
	sc_signal <int> req_count_to_l2;
	sc_signal <bool> address_from_l2_en;

	// l2-- dir

	sc_signal <bool> inv_to_node;
	sc_signal <int> inv_addr_to_node;
	sc_signal <bool> cl_evt;
	sc_signal <bool> inv_ack_to_dir;
	sc_signal <bool> flush_frm_npc_dir;	
	sc_signal <bool> flush_frm_npc_dir_l2;
	sc_signal <bool> generate_wr_req_npc;
	sc_signal <int> address_to_dir;
	sc_signal <int> address_from_dir;
	sc_signal <int> req_count_to_dir;
	sc_signal <int> req_count_from_dir;
	sc_signal <bool> ex_to_dir;
	sc_signal <bool> rd_wr_to_dir;
	sc_signal <bool> ack_to_node;
	sc_signal <bool> ce_to_dir;
	
	//lsu to l2-dir-mem/llc
	
	// from-to dir
	sc_signal <int> nodeaddr_to_lru_dir; // all lru interactions with nodeaddress specified , /*TODO: if not done in */
	sc_signal <int> node_address_from_lru_dir;
	sc_signal <bool> inv_to_node_dir;  // dir can start an inv / 
	sc_signal <int> inv_addr_to_node_dir;
	sc_signal <bool> cl_evt_info_to_dir_dir;
	sc_signal <bool> inv_ack_to_dir_dir;
	sc_signal <int> address_to_dir_dir;
	sc_signal <int> address_from_dir_dir;
	sc_signal <int> req_count_to_dir_dir;
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
	sc_clock clk;

		// from llc 
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
// l2
	sc_signal <int> addr_from_l2_to_llc;
	sc_signal <int> addr_to_l2_to_llc;
	sc_signal <int> req_count_from_l2_to_llc;
	sc_signal <int> req_count_to_l2_to_llc;
	
	sc_signal <bool> addr_en_from_l2_to_llc; 
	datatype_ch data_to_llc;
	datatype_ch data_from_llc;
	sc_signal <bool> ack_to_l2_llc;
	sc_signal <bool> ack_rw_to_l2_llc;

	sc_signal <int> addr_to_mem_llc;
	sc_signal <int> addr_from_mem_llc;
	sc_signal <int> req_count_to_mem_llc;
	sc_signal <int> req_count_from_mem_llc;
	sc_signal <bool> addr_en_to_mem_llc;
	sc_signal <bool> r_w_to_mem_llc;
	sc_signal <bool> trans_ack_from_mem_llc; 
	sc_signal <bool> ack_rw_from_mem_llc;
	datatype_ch data_to_mem_llc;

};

#endif
