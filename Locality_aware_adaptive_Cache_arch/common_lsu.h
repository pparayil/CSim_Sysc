#ifndef CM_LSU_H
#define CM_LSU_H

#include <queue>
#include <iostream>
#include <systemc.h>
#include "write_read_if.h"
#include "datatype_ch.h"
#include "LSU_defines.h"
#include "LiSNoC_defines.h"

//TODO: note all data widths needs to be changed from int to 8 word length
using namespace std;


SC_MODULE(CM_LSU) {
public:
	// from-to npc
	sc_out <bool> inv_frm_dir_npc;   
	sc_inout <int> inv_addr_from_dir_npc;
	sc_out <bool> cl_evt_info_to_dir_npc;
	sc_in <bool> inv_ack_to_dir_npc;  // npc can start either ce / ex / flush / cl_evt with address
	sc_out <bool> flush_frm_npc_dir_npc;
	sc_in <bool> generate_wr_req_npc_npc;
	sc_in <int> address_to_dir_npc;
	sc_out <int> address_from_dir_npc;
	sc_in <int> req_count_to_dir_npc;
	sc_out <int> req_count_from_dir_npc;
	sc_in <int> req_count_to_llc_npc;
	sc_out <int> req_count_from_llc_npc;
	sc_in <bool> ex_to_dir_npc;
	sc_in <bool> ce_to_dir_npc;
	sc_in <bool> rd_wr_to_dir_npc;
	sc_out <bool> ack_from_dir_npc;
	sc_out <bool> ack_from_llc_npc;
	sc_in <int> address_to_llc_npc;
	sc_out <int> address_from_llc_npc;
	sc_in <bool> address_to_llc_en_npc; // npc can start pass data to llc with address , if LLC is not present direct to MEM.
	sc_port<write_read_if> data_to_llc_npc;
	sc_port<write_read_if> data_from_llc_npc;
	sc_out <bool> mode_pri_remote_from_llc_npc;
	sc_out <bool> mode_pri_remote_from_dir_npc;
	sc_in <int> utiliz_to_dir_npc;	

	// from-to dir
	sc_in <int> nodeaddr_to_lru_dir; // all lru interactions with nodeaddress specified , 
	sc_out <int> node_address_from_lru_dir;
	sc_in <bool> inv_to_node_dir;  // dir can start an inv / 	
	sc_in <bool> flush_frm_npc_dir;
	sc_in <int> inv_addr_to_node_dir;
	sc_out <bool> cl_evt_info_to_dir_dir;
	sc_out <bool> inv_ack_to_dir_dir;
	sc_out <int> address_to_dir_dir;
	sc_in <int> address_from_dir_dir;
	sc_out <int> req_count_to_dir_dir;
	sc_in <int> req_count_from_dir_dir;
	sc_in <int> req_count_to_llc_dir; //
	sc_out <int> req_count_from_llc_dir; //
	sc_out <bool> ex_to_dir_dir;
	sc_out <bool> rd_wr_to_dir_dir;
	sc_out <bool> ce_to_dir_dir;
	sc_in <bool> ack_to_node_dir; // an ack to node 
	sc_in <bool> rd_wr_to_llc_dir; // ce to llc or mem (if no mem) whichever is available
	sc_in <int> addr_to_llc_dir; //
	sc_out <int> addr_from_llc_dir; //
	sc_in <int> dst_nodeaddr_to_llc_dir;
	sc_in <bool> ce_to_llc_dir;
	sc_out <bool> ack_from_llc_dir;
	sc_in <bool> mode_pri_remote_to_llc_dir;
	sc_out <int> priv_utiliz_to_dir;	
	sc_in <bool> mode_pri_remote_from_dir_dir;
	
	sc_in <bool> clk;
	
	// from llc 
	sc_in <int> nodeaddr_to_lru_llc;
	sc_out <int> node_address_from_lru_llc;

// dir interaction 
	sc_out <bool> rd_wr_llc;
	sc_out <int> addr_from_dir_to_llc;
	sc_in <int> addr_to_dir_to_llc;
	sc_out <int> dst_nodeaddr_llc; // _ce cud be enabled, addr_en is part of this
	sc_out <bool> ce_llc;
	sc_in <bool> ack_to_dir_llc;
	sc_out <bool> without_dir_ack_llc;
	sc_out <int> req_count_from_dir_to_llc;
	sc_in <int> req_count_to_dir_to_llc;
	sc_out <bool> mode_pri_remote_f_dir_llc;
// npc
	sc_out <int> req_count_from_npc_to_llc;
	sc_in <int> req_count_to_npc_to_llc;
	sc_out <int> addr_from_npc_to_llc;
	sc_in <int> addr_to_npc_to_llc;
	sc_out <bool> addr_en_from_npc_to_llc; 
	sc_port<write_read_if> data_to_llc;
	sc_port<write_read_if> data_from_llc;
	sc_in <bool> ack_to_npc_llc;
	sc_in <bool> ack_rw_to_npc_llc;
	sc_in <bool> mode_pri_remote_to_npc_llc;

	sc_in <int> addr_to_mem_llc;
	sc_out <int> addr_from_mem_llc;
	sc_in <int> req_count_to_mem_llc;
	sc_out <int> req_count_from_mem_llc;
	sc_in <bool> addr_en_to_mem_llc;
	sc_in <bool> r_w_to_mem_llc;
	sc_out <bool> trans_ack_from_mem_llc; 
	sc_out <bool> ack_rw_from_mem_llc;
	sc_port<write_read_if> data_to_mem_llc;

		
	SC_HAS_PROCESS(CM_LSU);
	CM_LSU(sc_module_name name, int id): sc_module(name), inode(id),trans_ack_from_mem_llc(0),addr_en_from_npc_to_llc(0),ack_from_llc_dir(0),ack_from_dir_npc(0)
	{
		SC_THREAD(lsu_tx_method);
		sensitive << clk.pos();
		SC_THREAD(lsu_rx_thread);
		sensitive << clk.pos();
		SC_THREAD(lsu_rx_npc);
		sensitive << clk.pos();
		SC_THREAD(lsu_rx_dir);
		sensitive << clk.pos();
		SC_THREAD(lsu_rx_llc);
		sensitive << clk.pos();
	}
	bool set_packet(int src_id, LisNocPacket* packet); // function which is called from processing element and which hence sets up the packet!
	void receive_packet(int src_id, LisNocPacket* packet);
	void setinode(int id) { inode = id;}
private:
	void lsu_tx_method(void );
	void lsu_rx_thread(void );
	void lsu_rx_npc(void );
	void lsu_rx_dir(void );
	void lsu_rx_llc(void );
	queue <NoCCommPacket> NoCtxQ;
	queue <NoCCommPacket> NoCrxQ;
	int inode;
	queue <NoCCommPacket>  local_npcQ, local_dirQ, local_llcQ;
};

#endif //CM_LSU_H
