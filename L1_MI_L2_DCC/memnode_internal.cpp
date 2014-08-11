
#include "memnode_internal.h"

void MEMNODE_INT::bind_internals (void){
	//dir 
	dirmem.clk(clk);
	dirmem.nodeaddr_to_lru(nodeaddr_to_lru_dir);
	dirmem.node_address_from_lru(node_address_from_lru_dir);
	// from othr nodes
	dirmem.spill_inv_to_node(spill_inv_to_node_dir);
	dirmem.inv_to_node(inv_to_node_dir);
	dirmem.dcc_transfer_en_to_npc(dcc_transfer_en_to_npc_dir);
	dirmem.flush_frm_npc(flush_frm_npc_dir);
	dirmem.forward_req_from_dir(forward_req_from_dir_dir);
	dirmem.replace_req_dcc_ack_from_dir(replace_req_dcc_ack_from_dir_dir);
	dirmem.forward_spillreq_to_npc(forward_spillreq_from_dir_dir);
	dirmem.inv_addr_to_node(inv_addr_to_node_dir);
	dirmem.cl_evt_info_to_dir(cl_evt_info_to_dir_dir);
	dirmem.inv_ack_to_dir(inv_ack_to_dir_dir);
	dirmem.address_to_dir(address_to_dir_dir);
	dirmem.address_from_dir(address_from_dir_dir);
	dirmem.req_count_to_dir(req_count_to_dir_dir);
	dirmem.req_count_from_dir(req_count_from_dir_dir);
	dirmem.ex_to_dir(ex_to_dir_dir);
	dirmem.rd_wr_to_dir(rd_wr_to_dir_dir);
	dirmem.ack_to_node(ack_to_node_dir);
	dirmem.ce_to_dir(ce_to_dir_dir);

	// L3 interaction 
	dirmem.rd_wr_to_l3(rd_wr_to_l3_dir);
	dirmem.addr_to_l3(addr_to_l3_dir);
	dirmem.addr_from_l3(addr_from_l3_dir);
	dirmem.req_count_to_l3(req_count_to_l3_dir);
	dirmem.req_count_from_l3(req_count_from_l3_dir);
	dirmem.dst_nodeaddr_to_l3(dst_nodeaddr_to_l3_dir);
	dirmem.ce_to_l3(ce_to_l3_dir);
	dirmem.ack_from_l3(ack_from_l3_dir);
	

	// from-to dir
	lsu.nodeaddr_to_lru_dir(nodeaddr_to_lru_dir); // all lru interactions with nodeaddress specified , /*TODO: if not done in */
	lsu.node_address_from_lru_dir(node_address_from_lru_dir);
	lsu.spill_inv_to_node_dir(spill_inv_to_node_dir);  // dir can start an inv /	
	lsu.dcc_transfer_en_to_npc_dir(dcc_transfer_en_to_npc_dir);
	lsu.inv_to_node_dir(inv_to_node_dir);	
	lsu.inv_addr_to_node_dir(inv_addr_to_node_dir);
	lsu.forward_req_from_dir_dir(forward_req_from_dir_dir);
	lsu.replace_req_dcc_ack_from_dir_dir(replace_req_dcc_ack_from_dir_dir);
	lsu.forward_spillreq_from_dir_dir(forward_spillreq_from_dir_dir);
	lsu.cl_evt_info_to_dir_dir(cl_evt_info_to_dir_dir);
	lsu.flush_frm_npc_dir(flush_frm_npc_dir);
	lsu.inv_ack_to_dir_dir(inv_ack_to_dir_dir);
	lsu.address_to_dir_dir(address_to_dir_dir);
	lsu.address_from_dir_dir(address_from_dir_dir);
	lsu.req_count_to_dir_dir(req_count_to_dir_dir);
	lsu.req_count_from_dir_dir(req_count_from_dir_dir);
	lsu.ex_to_dir_dir(ex_to_dir_dir);
	lsu.rd_wr_to_dir_dir(rd_wr_to_dir_dir);
	lsu.ce_to_dir_dir(ce_to_dir_dir);
	lsu.ack_to_node_dir(ack_to_node_dir); // an ack to node 
	lsu.rd_wr_to_l3_dir(rd_wr_to_l3_dir); // ce to l3 or mem (if no mem) whichever is available
	lsu.addr_to_l3_dir(addr_to_l3_dir); //
	lsu.addr_from_l3_dir(addr_from_l3_dir); //
	lsu.req_count_to_l3_dir(req_count_to_l3_dir); //
	lsu.req_count_from_l3_dir(req_count_from_l3_dir); //
	lsu.dst_nodeaddr_to_l3_dir(dst_nodeaddr_to_l3_dir);
	lsu.ce_to_l3_dir(ce_to_l3_dir);
	lsu.ack_from_l3_dir(ack_from_l3_dir);
	lsu.clk(clk);	

	lsu.nodeaddr_from_lru_dir_mem(nodeaddr_from_lru_dir_mem);
	lsu.nodeaddr_from_lru_l2_mem(nodeaddr_from_lru_l2_mem);
	lsu.node_address_to_lru_mem(node_address_to_lru_mem);
	lsu.rd_wr_from_dir_mem(rd_wr_to_dir_mem);
	lsu.req_count_from_dir_mem(req_count_from_dir_mem);
	lsu.req_count_to_dir_mem(req_count_to_dir_mem);
	lsu.addr_from_dir_mem(addr_from_dir_mem);
	lsu.addr_to_dir_mem(addr_to_dir_mem);
	lsu.dst_nodeaddr_from_dir_mem(dst_nodeaddr_to_dir_mem);
	lsu.ce_from_dir_mem(ce_from_dir_mem);
	lsu.ack_from_dir_mem(ack_to_dir_mem); // ack to dir with that node address of dir
	lsu.without_dir_ack_mem(without_dir_ack_mem);

	// from l2 to mem
	lsu.ack_to_l2_mem(ack_to_l2_mem); // ack to l2 with that node address to l2
	lsu.ack_rw_to_l2_mem(ack_rw_to_l2_mem);
	lsu.address_from_l2_mem(address_from_l2_mem);
	lsu.address_to_l2_mem(address_to_l2_mem);
	lsu.req_count_from_l2_mem(req_count_from_l2_mem);
	lsu.req_count_to_l2_mem(req_count_to_l2_mem);
	lsu.address_from_l2_en_mem(address_from_l2_en_mem);
	lsu.data_from_l2_mem(data_from_l2_mem);
	lsu.data_to_l2_mem(data_to_l2_mem);

	ram.nodeaddr_from_lru_dir(nodeaddr_from_lru_dir_mem);
	ram.nodeaddr_from_lru_l2(nodeaddr_from_lru_l2_mem);
	ram.node_address_to_lru(node_address_to_lru_mem);

	// from dir
	ram.rd_wr_from_dir(rd_wr_to_dir_mem);
	ram.addr_from_dir(addr_from_dir_mem);
	ram.addr_to_dir(addr_to_dir_mem);
	ram.req_count_from_dir(req_count_from_dir_mem);
	ram.req_count_to_dir(req_count_to_dir_mem);
	ram.dst_nodeaddr_from_dir(dst_nodeaddr_to_dir_mem);
	ram.ce_from_dir(ce_from_dir_mem);
	ram.ack_to_dir(ack_to_dir_mem);
	ram.ack_rw_to_l2(ack_rw_to_l2_mem);
	ram.without_dir_ack(without_dir_ack_mem);

	// from l2
	ram.ack_to_l2(ack_to_l2_mem);
	ram.address_from_l2(address_from_l2_mem);
	ram.address_to_l2(address_to_l2_mem);
	ram.req_count_from_l2(req_count_from_l2_mem);
	ram.req_count_to_l2(req_count_to_l2_mem);
	ram.address_from_l2_en(address_from_l2_en_mem);
	ram.data_from_l2(data_from_l2_mem);
	ram.data_to_l2(data_to_l2_mem);
	ram.clk(clk);
}
