
#include "memnode_internal.h"

void MEMNODE_INT::bind_internals (void){
	//dir 
	dirmem.clk(clk);
	dirmem.nodeaddr_to_lru(nodeaddr_to_lru_dir);
	dirmem.node_address_from_lru(node_address_from_lru_dir);
	// from othr nodes
	dirmem.inv_to_node(inv_to_node_dir);
	dirmem.flush_frm_npc(flush_frm_npc_dir);
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

	// LLC interaction 
	dirmem.rd_wr_to_llc(rd_wr_to_llc_dir);
	dirmem.addr_to_llc(addr_to_llc_dir);
	dirmem.addr_from_llc(addr_from_llc_dir);
	dirmem.req_count_to_llc(req_count_to_llc_dir);
	dirmem.req_count_from_llc(req_count_from_llc_dir);
	dirmem.dst_nodeaddr_to_llc(dst_nodeaddr_to_llc_dir);
	dirmem.ce_to_llc(ce_to_llc_dir);
	dirmem.ack_from_llc(ack_from_llc_dir);
	dirmem.mode_pri_remote_to_llc(mode_pri_remote_to_llc_dir);
	dirmem.priv_utiliz_to_dir(priv_utiliz_to_dir);
	dirmem.mode_pri_remote_from_dir(mode_pri_remote_from_dir_dir);

	// from-to dir
	lsu.nodeaddr_to_lru_dir(nodeaddr_to_lru_dir); // all lru interactions with nodeaddress specified , 
	lsu.node_address_from_lru_dir(node_address_from_lru_dir);
	lsu.inv_to_node_dir(inv_to_node_dir);  // dir can start an inv / 
	lsu.inv_addr_to_node_dir(inv_addr_to_node_dir);
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
	lsu.rd_wr_to_llc_dir(rd_wr_to_llc_dir); // ce to llc or mem (if no mem) whichever is available
	lsu.addr_to_llc_dir(addr_to_llc_dir); //
	lsu.addr_from_llc_dir(addr_from_llc_dir); //
	lsu.req_count_to_llc_dir(req_count_to_llc_dir); //
	lsu.req_count_from_llc_dir(req_count_from_llc_dir); //
	lsu.dst_nodeaddr_to_llc_dir(dst_nodeaddr_to_llc_dir);
	lsu.ce_to_llc_dir(ce_to_llc_dir);
	lsu.ack_from_llc_dir(ack_from_llc_dir);
	lsu.mode_pri_remote_to_llc_dir(mode_pri_remote_to_llc_dir);
	lsu.clk(clk);	


	lsu.nodeaddr_to_lru_llc(nodeaddr_to_lru_llc);
	lsu.node_address_from_lru_llc(node_address_from_lru_llc);
// dir interaction 
	lsu.rd_wr_llc(rd_wr_llc);
	lsu.addr_from_dir_to_llc(addr_from_dir_to_llc);
	lsu.addr_to_dir_to_llc(addr_to_dir_to_llc);
	lsu.req_count_from_dir_to_llc(req_count_from_dir_to_llc);
	lsu.req_count_to_dir_to_llc(req_count_to_dir_to_llc);
	lsu.dst_nodeaddr_llc(dst_nodeaddr_llc); // _ce cud be enabled, addr_en is part of this
	lsu.ce_llc(ce_llc);
	lsu.ack_to_dir_llc(ack_to_dir_llc);
	lsu.without_dir_ack_llc(without_dir_ack_llc);
// npc
	lsu.addr_from_npc_to_llc(addr_from_npc_to_llc);
	lsu.addr_to_npc_to_llc(addr_to_npc_to_llc);
	lsu.req_count_from_npc_to_llc(req_count_from_npc_to_llc);
	lsu.req_count_to_npc_to_llc(req_count_to_npc_to_llc);
	lsu.addr_en_from_npc_to_llc(addr_en_from_npc_to_llc); 
	lsu.data_to_llc(data_to_llc);
	lsu.data_from_llc(data_from_llc);
	lsu.ack_to_npc_llc(ack_to_npc_llc);
	lsu.ack_rw_to_npc_llc(ack_rw_to_npc_llc);
// mem
	lsu.addr_to_mem_llc(addr_to_mem_llc);
	lsu.addr_from_mem_llc(addr_from_mem_llc);
	lsu.req_count_to_mem_llc(req_count_to_mem_llc);
	lsu.req_count_from_mem_llc(req_count_from_mem_llc);
	lsu.addr_en_to_mem_llc(addr_en_to_mem_llc);
	lsu.r_w_to_mem_llc(r_w_to_mem_llc);
	lsu.trans_ack_from_mem_llc(trans_ack_from_mem_llc); 
	lsu.ack_rw_from_mem_llc(ack_rw_from_mem_llc); 
	lsu.data_to_mem_llc(data_to_mem_llc);
	lsu.mode_pri_remote_f_dir_llc(mode_pri_remote_f_dir_llc);
	lsu.mode_pri_remote_to_npc_llc(mode_pri_remote_to_npc_llc);
	lsu.priv_utiliz_to_dir(priv_utiliz_to_dir);
	lsu.mode_pri_remote_from_dir_dir(mode_pri_remote_from_dir_dir);


#if 1
	lsu.nodeaddr_to_lru_mem(nodeaddr_to_lru_mem); 
	lsu.node_address_from_lru_mem(node_address_from_lru_mem);
	lsu.addr_to_mem(addr_to_mem);
	lsu.addr_from_mem(addr_from_mem);
	lsu.req_count_to_mem(req_count_to_mem);
	lsu.req_count_from_mem(req_count_from_mem);
	lsu.addr_en_to_mem(addr_en_to_mem);
	lsu.r_w_to_mem(r_w_to_mem);
	lsu.trans_ack_from_mem(trans_ack_from_mem); 
	lsu.ack_rw_from_mem(ack_rw_from_mem); 
	lsu.data_mem(data_mem);


	ram.nodeaddr_from_lru(node_address_from_lru_mem);
	ram.node_address_to_lru(nodeaddr_to_lru_mem);

	// from dir
	ram.rd_wr_(r_w_to_mem);
	ram.addr_to_mem(addr_to_mem);
	ram.addr_from_mem(addr_from_mem);
	ram.req_count_to_mem(req_count_to_mem);
	ram.req_count_from_mem(req_count_from_mem);
	ram.ce_(addr_en_to_mem);
	ram.ack_(trans_ack_from_mem);
	ram.ack_rw(ack_rw_from_mem); 
	ram.data_(data_mem);
	ram.clk(clk);
#endif

	//llc
	llc.clk(clk);
	llc.nodeaddr_to_lru(nodeaddr_to_lru_llc);
	llc.node_address_from_lru(node_address_from_lru_llc);
// dir interaction 
	llc.rd_wr_(rd_wr_llc);
	llc.addr_f_dir(addr_from_dir_to_llc);
	llc.addr_t_dir(addr_to_dir_to_llc);
	llc.req_count_f_dir(req_count_from_dir_to_llc);
	llc.req_count_t_dir(req_count_to_dir_to_llc);
	llc.dst_nodeaddr_(dst_nodeaddr_llc); // _ce cud be enabled, addr_en is part of this
	llc.ce_(ce_llc);
	llc.ack_to_dir(ack_to_dir_llc);
	llc.without_dir_ack(without_dir_ack_llc);
	llc.mode_pri_remote_f_dir(mode_pri_remote_f_dir_llc);
	llc.mode_pri_remote_to_npc(mode_pri_remote_to_npc_llc);
// npc
	llc.addr_f_npc(addr_from_npc_to_llc);
	llc.addr_t_npc(addr_to_npc_to_llc);
	llc.req_count_f_npc(req_count_from_npc_to_llc);
	llc.req_count_t_npc(req_count_to_npc_to_llc);
	llc.addr_en_frm_npc(addr_en_from_npc_to_llc); 
	llc.data_in(data_to_llc);
	llc.data_out(data_from_llc);
	llc.ack_to_npc(ack_to_npc_llc);
	llc.ack_rw_to_npc(ack_rw_to_npc_llc);
// mem
	llc.addr_to_mem(addr_to_mem_llc);
	llc.addr_from_mem(addr_from_mem_llc);
	llc.req_count_to_mem(req_count_to_mem_llc);
	llc.req_count_from_mem(req_count_from_mem_llc);
	llc.addr_en_to_mem(addr_en_to_mem_llc);
	llc.r_w_to_mem(r_w_to_mem_llc);
	llc.ack_from_mem(trans_ack_from_mem_llc); 
	llc.ack_rw_from_mem(ack_rw_from_mem_llc); 
	llc.data_mem(data_to_mem_llc);
}
