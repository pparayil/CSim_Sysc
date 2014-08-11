#ifndef MEM_LSU_H
#define MEM_LSU_H

#include <queue>
#include <iostream>
#include <systemc.h>
#include "write_read_if.h"
#include "datatype_ch.h"
#include "LSU_defines.h"
#include "LiSNoC_defines.h"

//TODO: note all data widths needs to be changed from int to 8 word length
using namespace std;

SC_MODULE(MEM_LSU) {
public:


	// from-to dir
	sc_in <int> nodeaddr_to_lru_dir; // all lru interactions with nodeaddress specified , /*TODO: if not done in */
	sc_out <int> node_address_from_lru_dir;
	sc_in <bool> inv_to_node_dir;  // dir can start an inv / 
	sc_in <bool> spill_inv_to_node_dir;	
	sc_in <bool> dcc_transfer_en_to_npc_dir;
	sc_in <bool> flush_frm_npc_dir;
	sc_in <int> inv_addr_to_node_dir;	
	sc_in <bool> forward_req_from_dir_dir;		
	sc_in <bool> forward_spillreq_from_dir_dir;
	sc_in <bool> replace_req_dcc_ack_from_dir_dir;
	sc_out <bool> cl_evt_info_to_dir_dir;
	sc_out <bool> inv_ack_to_dir_dir;
	sc_out <int> address_to_dir_dir;
	sc_in <int> address_from_dir_dir;
	sc_out <int> req_count_to_dir_dir;
	sc_in <int> req_count_from_dir_dir;
	sc_out <bool> ex_to_dir_dir;
	sc_out <bool> rd_wr_to_dir_dir;
	sc_out <bool> ce_to_dir_dir;
	sc_in <bool> ack_to_node_dir; // an ack to node 
	sc_in <bool> rd_wr_to_l3_dir; // ce to l3 or mem (if no mem) whichever is available
	sc_in <int> addr_to_l3_dir; //
	sc_out <int> addr_from_l3_dir; //
	sc_in <int> req_count_to_l3_dir; //
	sc_out <int> req_count_from_l3_dir; //
	sc_in <int> dst_nodeaddr_to_l3_dir;
	sc_in <bool> ce_to_l3_dir;
	sc_out <bool> ack_from_l3_dir;
	
	
	sc_in <bool> clk;
	
	// from mem 
	sc_in <int> node_address_to_lru_mem;
	sc_out <int> nodeaddr_from_lru_dir_mem;
	sc_out <int> nodeaddr_from_lru_l2_mem;
	
	sc_out <bool> rd_wr_from_dir_mem;
	sc_out <int> addr_from_dir_mem;
	sc_in <int> addr_to_dir_mem;
	sc_out <int> req_count_from_dir_mem;
	sc_in <int> req_count_to_dir_mem;
	sc_out <int> dst_nodeaddr_from_dir_mem;
	sc_out <bool> ce_from_dir_mem;
	sc_in <bool> ack_from_dir_mem; // ack to dir with that node address of dir
	sc_out <bool> without_dir_ack_mem;

	// from l2 to mem
	sc_in <bool> ack_to_l2_mem; // ack to l2 with that node address to l2
	sc_in <bool> ack_rw_to_l2_mem;
	sc_out <int> address_from_l2_mem;
	sc_in <int> req_count_to_l2_mem;
	sc_out <int> req_count_from_l2_mem;
	sc_in <int> address_to_l2_mem;
	sc_out <bool> address_from_l2_en_mem;
	sc_port<write_read_if> data_from_l2_mem;
	sc_port<write_read_if> data_to_l2_mem;

	/*TODO: add L3*/
		
	SC_HAS_PROCESS(MEM_LSU);
	MEM_LSU(sc_module_name name, int id): sc_module(name), inode(id)
	{
		SC_METHOD(lsu_tx_method);
		sensitive << clk.pos();
		SC_THREAD(lsu_rx_thread);
		sensitive << clk.pos();
		SC_THREAD(lsu_rx_dir);
		sensitive << clk.pos();
		SC_THREAD(lsu_rx_mem);
		sensitive << clk.pos();

	}
	bool set_packet(int src_id, LisNocPacket* packet); // function which is called from processing element and which hence sets up the packet!
	void receive_packet(int src_id, LisNocPacket* packet);
	void setinode(int id) { inode = id;}
private:
	void lsu_tx_method(void );
	void lsu_rx_thread(void );
	void lsu_rx_dir(void );
	void lsu_rx_mem(void );
	queue <NoCCommPacket> NoCtxQ;
	queue <NoCCommPacket> NoCrxQ;
	int inode;
	queue <NoCCommPacket> local_dirQ, local_memQ;
};

#endif //MEM_LSU_H
