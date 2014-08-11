
#include "node_internal.h"

void NODE_INT::bind_internals (void){

	pr0.ADDRESS_DB_OUT.bind(address_1);
	pr0.REQ_COUNT_IN.bind(req_count_1);
	pr0.REQ_COUNT_OUT.bind(req_count_2);
	pr0.DATA_DB_OUT.bind(datachannel_2);
	pr0.CE_DB.bind(ce);
	pr0.WE_DB_OUT.bind(we_2);
	pr0.ADDRESS_DB_IN.bind(address_2);
	pr0.DATA_DB_IN.bind(datachannel_1);
	pr0.WE_DB_IN.bind(we_1);
	pr0.ACK_DB.bind(ack);
	pr0.STATUS_DB.bind(status_db);
	pr0.clk(clk);
	
	l1.ADDRESS_DB_OUT.bind(address_2);
	l1.DATA_DB_OUT.bind(datachannel_1);
	l1.CE_DB.bind(ce);
	l1.WE_DB_OUT.bind(we_1);
	l1.ADDRESS_DB_IN.bind(address_1);
	l1.DATA_DB_IN.bind(datachannel_2);
	l1.REQ_COUNT_IN.bind(req_count_2);
	l1.REQ_COUNT_OUT.bind(req_count_1);
	l1.WE_DB_IN.bind(we_2);
	l1.clk(clk);
	l1.STATUS_DB.bind(status_db);
	l1.ACK_DB.bind(ack);
	l1.ce_to_l2(ce_to_l2_l1);
	l1.rd_wr_to_l2(rd_wr_to_l2_l1);
	l1.address_to_l2(address_to_l2_l1);
	l1.req_count_to_l2(req_count_to_l2_l1);
	l1.ack_to_l2(ack_to_l2_l1);	
	l1.data_to_l2(data_to_l2_l1);	
	l1.req_count_from_l2(req_count_from_l2_l1);
	l1.address_from_l2(address_from_l2_l1);
	l1.data_from_l2(data_from_l2_l1);
	l1.ack_from_l2(ack_from_l2_l1);
	l1.ack_we_l2(ack_we_l2_l1);
	l1.inv_from_l2(inv_from_l2_l1);
	
	l2.ce_from_l1(ce_to_l2_l1);
	l2.rd_wr_from_l1(rd_wr_to_l2_l1);
	l2.address_from_l1(address_to_l2_l1);
	l2.req_count_from_l1(req_count_to_l2_l1);
	l2.ack_from_l1(ack_to_l2_l1);	
	l2.data_from_l1(data_to_l2_l1);	
	l2.req_count_to_l1(req_count_from_l2_l1);
	l2.address_to_l1(address_from_l2_l1);
	l2.data_to_l1(data_from_l2_l1);
	l2.ack_to_l1(ack_from_l2_l1);
	l2.ack_we_to_l1(ack_we_l2_l1);
	l2.inv_to_l1(inv_from_l2_l1);
	l2.clk(clk);
	//noc side

	l2.inv_frm_dir(inv_to_node);
	l2.inv_addr_from_dir(inv_addr_to_node);
	l2.cl_evt_info_to_dir(cl_evt);
	l2.inv_ack_to_dir(inv_ack_to_dir);
	l2.flush_frm_npc_dir(flush_frm_npc_dir_l2);
	l2.generate_wr_req_npc(generate_wr_req_npc);
	l2.address_to_dir(address_to_dir);
	l2.address_from_dir(address_from_dir);
	l2.req_count_to_dir(req_count_to_dir);
	l2.req_count_from_dir(req_count_from_dir);
	l2.ex_to_dir(ex_to_dir);
	l2.ce_to_dir(ce_to_dir);
	l2.rd_wr_to_dir(rd_wr_to_dir);
	l2.ack_from_dir(ack_to_node);

	l2.ack_from_llc(ack_to_l2);
	l2.address_to_llc(address_from_l2);
	l2.address_from_llc(address_to_l2);
	l2.req_count_to_llc(req_count_from_l2);
	l2.req_count_from_llc(req_count_to_l2);
	l2.address_to_llc_en(address_from_l2_en);
	l2.data_to_llc.bind(data_from_l2);
	l2.data_from_llc.bind(data_to_l2);

	//dir 
	dirl2.clk(clk);
	dirl2.nodeaddr_to_lru(nodeaddr_to_lru_dir);
	dirl2.node_address_from_lru(node_address_from_lru_dir);
	// from othr nodes
	dirl2.inv_to_node(inv_to_node_dir);
	dirl2.flush_frm_npc(flush_frm_npc_dir);
	dirl2.inv_addr_to_node(inv_addr_to_node_dir);
	dirl2.cl_evt_info_to_dir(cl_evt_info_to_dir_dir);
	dirl2.inv_ack_to_dir(inv_ack_to_dir_dir);
	dirl2.address_to_dir(address_to_dir_dir);
	dirl2.address_from_dir(address_from_dir_dir);	
	dirl2.req_count_to_dir(req_count_to_dir_dir);
	dirl2.req_count_from_dir(req_count_from_dir_dir);
	dirl2.ex_to_dir(ex_to_dir_dir);
	dirl2.rd_wr_to_dir(rd_wr_to_dir_dir);
	dirl2.ack_to_node(ack_to_node_dir);
	dirl2.ce_to_dir(ce_to_dir_dir);

	// LLC interaction 
	dirl2.rd_wr_to_llc(rd_wr_to_llc_dir);
	dirl2.addr_to_llc(addr_to_llc_dir);
	dirl2.addr_from_llc(addr_from_llc_dir);
	dirl2.req_count_from_llc(req_count_from_llc_dir);
	dirl2.req_count_to_llc(req_count_to_llc_dir);
	dirl2.dst_nodeaddr_to_llc(dst_nodeaddr_to_llc_dir);
	dirl2.ce_to_llc(ce_to_llc_dir);
	dirl2.ack_from_llc(ack_from_llc_dir);


//LLC 		
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
// l2
	llc.addr_f_l2(addr_from_l2_to_llc);
	llc.req_count_t_l2(req_count_to_l2_to_llc);
	llc.req_count_f_l2(req_count_from_l2_to_llc);
	llc.addr_t_l2(addr_to_l2_to_llc);
	llc.addr_en_frm_l2(addr_en_from_l2_to_llc); 
	llc.data_in(data_to_llc);
	llc.data_out(data_from_llc);
	llc.ack_to_l2(ack_to_l2_llc);
	llc.ack_rw_to_l2(ack_rw_to_l2_llc);
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

	// from-to l2
	lsu.inv_frm_dir_l2(inv_to_node);   
	lsu.inv_addr_from_dir_l2(inv_addr_to_node);
	lsu.cl_evt_info_to_dir_l2(cl_evt);
	lsu.inv_ack_to_dir_l2(inv_ack_to_dir);  // l2 can start either ce / ex / flush / cl_evt with address
	lsu.flush_frm_npc_dir_l2(flush_frm_npc_dir_l2);
	lsu.flush_frm_npc_dir(flush_frm_npc_dir);	
	lsu.generate_wr_req_npc_l2(generate_wr_req_npc);
	lsu.address_to_dir_l2(address_to_dir);
	lsu.address_from_dir_l2(address_from_dir);
	lsu.req_count_to_dir_l2(req_count_to_dir);
	lsu.req_count_from_dir_l2(req_count_from_dir);
	lsu.req_count_to_llc_l2(req_count_from_l2);
	lsu.req_count_from_llc_l2(req_count_to_l2);
	lsu.ex_to_dir_l2(ex_to_dir);
	lsu.ce_to_dir_l2(ce_to_dir);
	lsu.rd_wr_to_dir_l2(rd_wr_to_dir);
	lsu.ack_from_dir_l2(ack_to_node);
	lsu.ack_from_llc_l2(ack_to_l2);
	lsu.address_to_llc_l2(address_from_l2);
	lsu.address_from_llc_l2(address_to_l2);
	lsu.address_to_llc_en_l2(address_from_l2_en); // l2 can start pass data to llc with address , if LLC is not present direct to MEM.
	lsu.data_to_llc_l2(data_from_l2);
	lsu.data_from_llc_l2(data_to_l2);

	// from-to dir
	lsu.nodeaddr_to_lru_dir(nodeaddr_to_lru_dir); // all lru interactions with nodeaddress specified , /*TODO: if not done in */
	lsu.node_address_from_lru_dir(node_address_from_lru_dir);
	lsu.inv_to_node_dir(inv_to_node_dir);  // dir can start an inv / 
	lsu.inv_addr_to_node_dir(inv_addr_to_node_dir);
	lsu.cl_evt_info_to_dir_dir(cl_evt_info_to_dir_dir);
	lsu.inv_ack_to_dir_dir(inv_ack_to_dir_dir);
	lsu.address_to_dir_dir(address_to_dir_dir);
	lsu.address_from_dir_dir(address_from_dir_dir);
	lsu.req_count_to_dir_dir(req_count_to_dir_dir);
	lsu.req_count_from_dir_dir(req_count_from_dir_dir);
	lsu.req_count_to_llc_dir(req_count_to_llc_dir); //
	lsu.req_count_from_llc_dir(req_count_from_llc_dir); //
	lsu.ex_to_dir_dir(ex_to_dir_dir);
	lsu.rd_wr_to_dir_dir(rd_wr_to_dir_dir);
	lsu.ce_to_dir_dir(ce_to_dir_dir);
	lsu.ack_to_node_dir(ack_to_node_dir); // an ack to node 
	lsu.rd_wr_to_llc_dir(rd_wr_to_llc_dir); // ce to llc or mem (if no mem) whichever is available
	lsu.addr_to_llc_dir(addr_to_llc_dir); //
	lsu.addr_from_llc_dir(addr_from_llc_dir); //
	lsu.dst_nodeaddr_to_llc_dir(dst_nodeaddr_to_llc_dir);
	lsu.ce_to_llc_dir(ce_to_llc_dir);
	lsu.ack_from_llc_dir(ack_from_llc_dir);

	//from-to llc
	lsu.nodeaddr_to_lru_llc(nodeaddr_to_lru_llc);
	lsu.node_address_from_lru_llc(node_address_from_lru_llc);
	lsu.rd_wr_llc(rd_wr_llc);
	lsu.addr_from_dir_to_llc(addr_from_dir_to_llc);
	lsu.addr_to_dir_to_llc(addr_to_dir_to_llc);
	lsu.dst_nodeaddr_llc(dst_nodeaddr_llc); // _ce cud be enabled, addr_en is part of this
	lsu.ce_llc(ce_llc);
	lsu.ack_to_dir_llc(ack_to_dir_llc);
	lsu.without_dir_ack_llc(without_dir_ack_llc);
	lsu.req_count_from_dir_to_llc(req_count_from_dir_to_llc);
	lsu.req_count_to_dir_to_llc(req_count_to_dir_to_llc);
	lsu.req_count_from_l2_to_llc(req_count_from_l2_to_llc);
	lsu.req_count_to_l2_to_llc(req_count_to_l2_to_llc);
	lsu.req_count_to_mem_llc(req_count_to_mem_llc);
	lsu.req_count_from_mem_llc(req_count_from_mem_llc);
	lsu.addr_from_l2_to_llc(addr_from_l2_to_llc);
	lsu.addr_to_l2_to_llc(addr_to_l2_to_llc);
	lsu.addr_en_from_l2_to_llc(addr_en_from_l2_to_llc); 
	lsu.data_to_llc(data_to_llc);
	lsu.data_from_llc(data_from_llc);
	lsu.ack_to_l2_llc(ack_to_l2_llc);
	lsu.ack_rw_to_l2_llc(ack_rw_to_l2_llc);
	lsu.addr_to_mem_llc(addr_to_mem_llc);
	lsu.addr_from_mem_llc(addr_from_mem_llc);
	lsu.addr_en_to_mem_llc(addr_en_to_mem_llc);
	lsu.r_w_to_mem_llc(r_w_to_mem_llc);
	lsu.trans_ack_from_mem_llc(trans_ack_from_mem_llc); 
	lsu.ack_rw_from_mem_llc(ack_rw_from_mem_llc); 
	lsu.data_to_mem_llc(data_to_mem_llc);
	lsu.clk(clk);	

}
