
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
	l2.spill_inv_frm_dir(spill_inv_to_node);
	l2.inv_addr_from_dir(inv_addr_to_node);
	l2.cl_evt_info_to_dir(cl_evt);
	l2.inv_ack_to_dir(inv_ack_to_dir);
	l2.forward_req_from_dir(forward_req_from_dir_l2);
	l2.forward_data_to_npc(forward_data_to_npc_l2);
	l2.replace_req_dcc_ack_from_dir(replace_req_dcc_ack_from_dir_l2);
	l2.replace_data_dcc_ack_to_npc(replace_data_dcc_ack_to_npc_l2);
	l2.replace_data_dcc_ack_from_npc(replace_data_dcc_ack_from_npc_l2);
	l2.replace_data_dcc_to_npc(replace_data_dcc_to_npc_l2);
	l2.replace_data_dcc_from_npc(replace_data_dcc_from_npc_l2);
	l2.forward_spillreq_from_dir(forward_spillreq_from_dir_l2);
	l2.forward_spill_data_to_npc(forward_spill_data_to_npc_l2);
	l2.forward_spilldata_ack_from_npc(forward_spilldata_ack_from_npc_l2);
	l2.spill_data_dirty_from_npc(spill_data_dirty_from_npc_l2);
	l2.spill_data_dirty_to_npc(spill_data_dirty_to_npc_l2);
	l2.dcc_transfer_en_from_dir(dcc_transfer_en_from_dir_l2);
	l2.nodeid_from_lsu(nodeid_from_lsu_l2);
	l2.nodeid_to_lsu(nodeid_to_lsu_l2);
	l2.ack_from_npc(ack_from_npc_l2);
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

	l2.ack_from_l3(ack_to_l2);
	l2.address_to_l3(address_from_l2);
	l2.address_from_l3(address_to_l2);
	l2.req_count_to_l3(req_count_from_l2);
	l2.req_count_from_l3(req_count_to_l2);
	l2.address_to_l3_en(address_from_l2_en);
	l2.data_to_l3.bind(data_from_l2);
	l2.data_from_l3.bind(data_to_l2);

	//dir 
	dirl2.clk(clk);
	dirl2.nodeaddr_to_lru(nodeaddr_to_lru_dir);
	dirl2.node_address_from_lru(node_address_from_lru_dir);
	// from othr nodes
	dirl2.inv_to_node(inv_to_node_dir);
	dirl2.spill_inv_to_node(spill_inv_to_node_dir);
	dirl2.dcc_transfer_en_to_npc(dcc_transfer_en_to_npc_dir);
	dirl2.flush_frm_npc(flush_frm_npc_dir);
	dirl2.forward_req_from_dir(forward_req_from_dir_dir);
	dirl2.replace_req_dcc_ack_from_dir(replace_req_dcc_ack_from_dir_dir);
	dirl2.forward_spillreq_to_npc(forward_spillreq_from_dir_dir);
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

	// L3 interaction 
	dirl2.rd_wr_to_l3(rd_wr_to_l3_dir);
	dirl2.addr_to_l3(addr_to_l3_dir);
	dirl2.addr_from_l3(addr_from_l3_dir);
	dirl2.req_count_from_l3(req_count_from_l3_dir);
	dirl2.req_count_to_l3(req_count_to_l3_dir);
	dirl2.dst_nodeaddr_to_l3(dst_nodeaddr_to_l3_dir);
	dirl2.ce_to_l3(ce_to_l3_dir);
	dirl2.ack_from_l3(ack_from_l3_dir);
	
	// from-to l2
	lsu.inv_frm_dir_l2(inv_to_node);  
	lsu.spill_inv_frm_dir_l2(spill_inv_to_node);
	lsu.dcc_transfer_en_from_dir_l2(dcc_transfer_en_from_dir_l2);
	lsu.inv_addr_from_dir_l2(inv_addr_to_node);
	lsu.cl_evt_info_to_dir_l2(cl_evt);
	lsu.inv_ack_to_dir_l2(inv_ack_to_dir);  // l2 can start either ce / ex / flush / cl_evt with address
	lsu.flush_frm_npc_dir_l2(flush_frm_npc_dir_l2);
	lsu.forward_req_from_dir_l2(forward_req_from_dir_l2);
	lsu.forward_data_to_npc_l2(forward_data_to_npc_l2);
	
	lsu.forward_spillreq_from_dir_l2(forward_spillreq_from_dir_l2);
	lsu.forward_spill_data_to_npc_l2(forward_spill_data_to_npc_l2);
	lsu.forward_spilldata_ack_from_npc_l2(forward_spilldata_ack_from_npc_l2);
	lsu.spill_data_dirty_from_npc_l2(spill_data_dirty_from_npc_l2);
	lsu.spill_data_dirty_to_npc_l2(spill_data_dirty_to_npc_l2);
	
	lsu.replace_req_dcc_ack_from_dir_l2(replace_req_dcc_ack_from_dir_l2);
	lsu.replace_data_dcc_ack_to_npc_l2(replace_data_dcc_ack_to_npc_l2);
	lsu.replace_data_dcc_ack_from_npc_l2(replace_data_dcc_ack_from_npc_l2);
	lsu.replace_data_dcc_to_npc_l2(replace_data_dcc_to_npc_l2);
	lsu.replace_data_dcc_from_npc_l2(replace_data_dcc_from_npc_l2);
	lsu.nodeid_from_lsu_l2(nodeid_from_lsu_l2);
	lsu.nodeid_to_lsu_l2(nodeid_to_lsu_l2);
	lsu.ack_from_npc_l2(ack_from_npc_l2);
	lsu.flush_frm_npc_dir(flush_frm_npc_dir);	
	lsu.forward_req_from_dir_dir(forward_req_from_dir_dir);
	lsu.replace_req_dcc_ack_from_dir_dir(replace_req_dcc_ack_from_dir_dir);
	lsu.forward_spillreq_from_dir_dir(forward_spillreq_from_dir_dir);
	lsu.generate_wr_req_npc_l2(generate_wr_req_npc);
	lsu.address_to_dir_l2(address_to_dir);
	lsu.address_from_dir_l2(address_from_dir);
	lsu.req_count_to_dir_l2(req_count_to_dir);
	lsu.req_count_from_dir_l2(req_count_from_dir);
	lsu.req_count_to_l3_l2(req_count_from_l2);
	lsu.req_count_from_l3_l2(req_count_to_l2);
	lsu.ex_to_dir_l2(ex_to_dir);
	lsu.ce_to_dir_l2(ce_to_dir);
	lsu.rd_wr_to_dir_l2(rd_wr_to_dir);
	lsu.ack_from_dir_l2(ack_to_node);
	lsu.ack_from_l3_l2(ack_to_l2);
	lsu.address_to_l3_l2(address_from_l2);
	lsu.address_from_l3_l2(address_to_l2);
	lsu.address_to_l3_en_l2(address_from_l2_en); // l2 can start pass data to l3 with address , if L3 is not present direct to MEM.
	lsu.data_to_l3_l2(data_from_l2);
	lsu.data_from_l3_l2(data_to_l2);

	// from-to dir
	lsu.nodeaddr_to_lru_dir(nodeaddr_to_lru_dir); // all lru interactions with nodeaddress specified , /*TODO: if not done in */
	lsu.node_address_from_lru_dir(node_address_from_lru_dir);
	lsu.spill_inv_to_node_dir(spill_inv_to_node_dir);  // dir can start an inv / 
	lsu.dcc_transfer_en_to_npc_dir(dcc_transfer_en_to_npc_dir);
	lsu.inv_to_node_dir(inv_to_node_dir);
	lsu.inv_addr_to_node_dir(inv_addr_to_node_dir);
	lsu.cl_evt_info_to_dir_dir(cl_evt_info_to_dir_dir);
	lsu.inv_ack_to_dir_dir(inv_ack_to_dir_dir);
	lsu.address_to_dir_dir(address_to_dir_dir);
	lsu.address_from_dir_dir(address_from_dir_dir);
	lsu.req_count_to_dir_dir(req_count_to_dir_dir);
	lsu.req_count_from_dir_dir(req_count_from_dir_dir);
	lsu.req_count_to_l3_dir(req_count_to_l3_dir); //
	lsu.req_count_from_l3_dir(req_count_from_l3_dir); //
	lsu.ex_to_dir_dir(ex_to_dir_dir);
	lsu.rd_wr_to_dir_dir(rd_wr_to_dir_dir);
	lsu.ce_to_dir_dir(ce_to_dir_dir);
	lsu.ack_to_node_dir(ack_to_node_dir); // an ack to node 
	lsu.rd_wr_to_l3_dir(rd_wr_to_l3_dir); // ce to l3 or mem (if no mem) whichever is available
	lsu.addr_to_l3_dir(addr_to_l3_dir); //
	lsu.addr_from_l3_dir(addr_from_l3_dir); //
	lsu.dst_nodeaddr_to_l3_dir(dst_nodeaddr_to_l3_dir);
	lsu.ce_to_l3_dir(ce_to_l3_dir);
	lsu.ack_from_l3_dir(ack_from_l3_dir);
	lsu.clk(clk);	

}
