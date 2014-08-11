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
	// from-to l2
	sc_out <bool> inv_frm_dir_l2; 
	sc_out <bool> spill_inv_frm_dir_l2;   	
	sc_inout <int> inv_addr_from_dir_l2;
	sc_out <bool> cl_evt_info_to_dir_l2;
	sc_out <bool> forward_req_from_dir_l2;
	sc_in <bool> forward_data_to_npc_l2;
	sc_out <bool> forward_spillreq_from_dir_l2;
	sc_in <bool> forward_spill_data_to_npc_l2;
	sc_out <bool> forward_spilldata_ack_from_npc_l2;
	sc_out <bool> spill_data_dirty_from_npc_l2;
	sc_in <bool> spill_data_dirty_to_npc_l2;
	sc_out <bool> dcc_transfer_en_from_dir_l2;
	sc_out <int> nodeid_from_lsu_l2; //forward destination nodeid
	sc_in <int> nodeid_to_lsu_l2;
	sc_out <bool> ack_from_npc_l2;
	sc_out <bool> replace_req_dcc_ack_from_dir_l2;	
	sc_in <bool> replace_data_dcc_ack_to_npc_l2;
	sc_out <bool> replace_data_dcc_ack_from_npc_l2;
	sc_in <bool> replace_data_dcc_to_npc_l2;
	sc_out <bool> replace_data_dcc_from_npc_l2;
	sc_in <bool> inv_ack_to_dir_l2;  // l2 can start either ce / ex / flush / cl_evt with address
	sc_out <bool> flush_frm_npc_dir_l2;
	sc_in <bool> generate_wr_req_npc_l2;
	sc_in <int> address_to_dir_l2;
	sc_out <int> address_from_dir_l2;
	sc_in <int> req_count_to_dir_l2;
	sc_out <int> req_count_from_dir_l2;
	sc_in <int> req_count_to_l3_l2;
	sc_out <int> req_count_from_l3_l2;
	sc_in <bool> ex_to_dir_l2;
	sc_in <bool> ce_to_dir_l2;
	sc_in <bool> rd_wr_to_dir_l2;
	sc_out <bool> ack_from_dir_l2;
	sc_out <bool> ack_from_l3_l2;
	sc_in <int> address_to_l3_l2;
	sc_out <int> address_from_l3_l2;
	sc_in <bool> address_to_l3_en_l2; // l2 can start pass data to l3 with address , if L3 is not present direct to MEM.
	sc_port<write_read_if> data_to_l3_l2;
	sc_port<write_read_if> data_from_l3_l2;

	// from-to dir
	sc_in <int> nodeaddr_to_lru_dir; // all lru interactions with nodeaddress specified , /*TODO: if not done in */
	sc_out <int> node_address_from_lru_dir;
	sc_in <bool> inv_to_node_dir;  // dir can start an inv /  
	sc_in <bool> spill_inv_to_node_dir;	
	sc_in <bool> dcc_transfer_en_to_npc_dir;
	sc_in <bool> forward_req_from_dir_dir;	
	sc_in <bool> forward_spillreq_from_dir_dir;
	sc_in <bool> replace_req_dcc_ack_from_dir_dir;
	sc_in <bool> flush_frm_npc_dir;
	sc_in <int> inv_addr_to_node_dir;
	sc_out <bool> cl_evt_info_to_dir_dir;
	sc_out <bool> inv_ack_to_dir_dir;
	sc_out <int> address_to_dir_dir;
	sc_in <int> address_from_dir_dir;
	sc_out <int> req_count_to_dir_dir;
	sc_in <int> req_count_from_dir_dir;
	sc_in <int> req_count_to_l3_dir; //
	sc_out <int> req_count_from_l3_dir; //
	sc_out <bool> ex_to_dir_dir;
	sc_out <bool> rd_wr_to_dir_dir;
	sc_out <bool> ce_to_dir_dir;
	sc_in <bool> ack_to_node_dir; // an ack to node 
	sc_in <bool> rd_wr_to_l3_dir; // ce to l3 or mem (if no mem) whichever is available
	sc_in <int> addr_to_l3_dir; //
	sc_out <int> addr_from_l3_dir; //
	sc_in <int> dst_nodeaddr_to_l3_dir;
	sc_in <bool> ce_to_l3_dir;
	sc_out <bool> ack_from_l3_dir;
	
	
	sc_in <bool> clk;
	
	// from l3 /*TODO: add L3*/
	
		
	SC_HAS_PROCESS(CM_LSU);
	CM_LSU(sc_module_name name, int id): sc_module(name), inode(id)
	{
		SC_METHOD(lsu_tx_method);
		sensitive << clk.pos();
		SC_THREAD(lsu_rx_thread);
		sensitive << clk.pos();
		SC_THREAD(lsu_rx_l2);
		sensitive << clk.pos();
		SC_THREAD(lsu_rx_dir);
		sensitive << clk.pos();

	}
	bool set_packet(int src_id, LisNocPacket* packet); // function which is called from processing element and which hence sets up the packet!
	void receive_packet(int src_id, LisNocPacket* packet);
	void setinode(int id) { inode = id;}
private:
	void lsu_tx_method(void );
	void lsu_rx_thread(void );
	void lsu_rx_l2(void );
	void lsu_rx_dir(void );
	queue <NoCCommPacket> NoCtxQ;
	queue <NoCCommPacket> NoCrxQ;
	int inode;
	queue <NoCCommPacket>  local_l2Q, local_dirQ;
};

#endif //CM_LSU_H
