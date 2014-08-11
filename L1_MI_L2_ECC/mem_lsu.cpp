
#include "mem_lsu.h"
#include <vector>


bool MEM_LSU::set_packet(int src_id, LisNocPacket* packet){
	
	if(NoCtxQ.empty() == true) {
	return false;
	}
	else
	{
	if(packet == NULL) {
		 cout<<"MEM_LSU::"<<inode<<", "<<__func__<<" : "<< __LINE__<<" Error NULL packet"<<endl; 
		return false;
	}
	NoCCommPacket pkt = NoCtxQ.front();	
	NoCtxQ.pop();
	packet->type = pkt.type;
	packet->req_count = pkt.req_count;
	packet->address.push_back(pkt.address);
	packet->dst_id = pkt.dstnode;
	packet->src_id = pkt.srcnode;	
	packet->redirectionnode = pkt.redirectionnode;
	packet->read_or_write = pkt.rw;
	packet->inv = pkt.inv;
	packet->evt = pkt.evt;
	packet->en = pkt.en;
	packet->ex = pkt.ex;
	packet->ack = pkt.ack;
	packet->burst = 0;
	packet->offset = 0;	
	//cout<<"MEM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<" to "<< packet->dst_id<<endl;
	switch(pkt.type){
		case IDLE: {
					//cout<<"NO PKT"<<endl;
					return false;
					}
					break;
		case MEMDIRACK:{					
					packet->size = 2;
					}
					break;
		case MEML2ACK:{					
					if(pkt.rw == false){
					for(int i=0;i<CL_SIZE_WORDWISE;i++){
					packet->data.push_back(pkt.data.data[i]);
					}
					packet->size = 10;
					} 
					else
					{
					packet->size = 2;
					}
					//cout<<"MEM_LSU:: data[7]: "<< pkt.data.data[7]<<" and from lisnoc side data[7]: "<<packet->data.back()<<endl;
					}
					break;
		case DIRINV:{
					packet->size = 2;
					}
					break;
		case DIRFWD_TO_NPC: {
					packet->size = 2;
					} break;
		case DIRACK:{
					packet->size = 2;
					}
					break;
		case DIRCE_TO_L3MEM:{	
					packet->size = 2;
					}
					break;
		default: cout<<"ERROR: "; 
				 cout<<"MEM_LSU::"<<__func__<<" : "<< __LINE__<<endl;				 
		}
	packet->flit_left = packet->size;
	}
	return true;
}

void MEM_LSU::lsu_tx_method(void ) {
	////cout<<"MEM_LSU::"<<__func__<<" : "<< __LINE__<<endl;
	//cout<<dec;
	LSU_STATE st = IDLE; 
	LSU_STATE first_st =IDLE,second_st =IDLE;
	int i=3; //
	while (i!=0)
	{
		switch (st) {
		case IDLE: {
					//mem side
					if(ack_from_dir_mem == true)
					{
						st = MEMDIRACK;
						//cout<<"MEM_LSU::"<<__func__<<" : "<< __LINE__<<endl;
						first_st = MEMDIRACK;
					}
					else if(ack_to_l2_mem == true){
						st = MEML2ACK;
						//cout<<"MEM_LSU::"<<__func__<<" : "<< __LINE__<<endl;
						first_st = MEML2ACK;
					}
					
					//dir side
					if((inv_to_node_dir == true) || (flush_frm_npc_dir == true)) {
						//cout<<"MEM_LSU::"<<__func__<<" : "<< __LINE__<<endl;
						if (first_st != IDLE) 
							second_st = DIRINV;
						else 
							{
							first_st = DIRINV;
							st = DIRINV;
							}
					}
					else if(ack_to_node_dir == true) {
						//cout<<"MEM_LSU::"<<__func__<<" : "<< __LINE__<<endl;
						if (first_st != IDLE) 
							second_st = DIRACK;
						else 
							{
							first_st = DIRACK;
							st = DIRACK;
							}
					}
					else if(ce_to_l3_dir == true) {
						//cout<<"MEM_LSU::"<<__func__<<" : "<< __LINE__<<endl;
						if (first_st != IDLE) 
							second_st = DIRCE_TO_L3MEM;
						else 
							{
							first_st = DIRCE_TO_L3MEM;
							st = DIRCE_TO_L3MEM;
							}
					} else if((forward_req_from_dir_dir == true) || (replace_req_dcc_ack_from_dir_dir == true)) {
						if (first_st != IDLE) 
							second_st = DIRFWD_TO_NPC;
						else 
							{
							first_st = DIRFWD_TO_NPC;
							st = DIRFWD_TO_NPC;
							}
					}	
					if((first_st == IDLE) && (second_st == IDLE))
					{
					st = IDLE;
					i = 1;
					//next_trigger(1,SC_NS);
					}
					} break;
					// check or find node address and see if that belongs to local node.
		case MEMDIRACK: {
						//cout<<"MEM_LSU::"<<__func__<<" : "<< __LINE__<<" "<<node_address_to_lru_mem<<" address:0x"<<hex<<addr_to_dir_mem<<dec<<endl;
						int dst_node = node_address_to_lru_mem; /*0x11 & address will give the dst dir bank address*/
						NoCCommPacket pkt;
						pkt.ack = ack_from_dir_mem;
						pkt.srcnode = inode;
						pkt.dstnode = dst_node;
						pkt.address = addr_to_dir_mem;
						pkt.req_count = req_count_to_dir_mem;
						pkt.type = st;
						if(dst_node == inode)
						{
							// local transfer							
							local_dirQ.push(pkt);
						}
						else
						{
							//make packet and put in the queue							
							NoCtxQ.push(pkt);
							
						}
						first_st = IDLE;
						st = second_st;
						if(st == IDLE) {
						i = 1;
						//next_trigger(1,SC_NS);
						}			
						}break;
		case MEML2ACK: {
						//cout<<"MEM_LSU::"<<__func__<<" : "<< __LINE__<<" "<<node_address_to_lru_mem<<endl;
						
						int dst_node = node_address_to_lru_mem; /*0x11 & address will give the dst dir bank address*/
						if(dst_node == inode)
						{
							// local transfer
							cout<<"MEM_LSU::"<<inode<<", "<<__func__<<" : "<< __LINE__<<"Error memory node doesnt contain l2" << endl;													
						}
						else
						{
							//make packet and put in the queue
							NoCCommPacket pkt;
							pkt.ack = ack_to_l2_mem;
							pkt.rw = ack_rw_to_l2_mem;
							pkt.address = address_to_l2_mem;
							pkt.req_count = req_count_to_l2_mem;
							data_to_l2_mem->read_data(&pkt.data);
							pkt.srcnode = inode;
							pkt.dstnode = dst_node;
							pkt.type = st;
							NoCtxQ.push(pkt);
							//cout<<"MEM_LSU::"<<__func__<<" : "<< __LINE__<<" pushed the packet with data[0]: "<<pkt.data.data[0]<<endl;
						
						}
						first_st = IDLE;
						st = second_st;
						if(st == IDLE) {
						i = 1;
						//next_trigger(1,SC_NS);
						}
						}break;
		case DIRINV: {
						//cout<<"MEM_LSU::"<<__func__<<" : "<< __LINE__<<" DIRINV sending to l2:"<< nodeaddr_to_lru_dir <<endl;
						if(nodeaddr_to_lru_dir == inode)
						{
							// local transfer
							cout<<"MEM_LSU::"<<inode<<", "<<__func__<<" : "<< __LINE__<<"error!! "<<endl;							
												
						}
						else
						{
							//make packet and put in the queue
							NoCCommPacket pkt;							
							if((flush_frm_npc_dir == true) && (inv_to_node_dir == false)){
								pkt.evt = false;
								pkt.inv = true;
								pkt.redirectionnode = dst_nodeaddr_to_l3_dir;
							} else if ((flush_frm_npc_dir == false) && (inv_to_node_dir == true)){
								pkt.evt = true;
								pkt.inv = true;
								pkt.ex = spill_inv_to_node_dir;
							}
							pkt.address = inv_addr_to_node_dir;
							pkt.req_count = req_count_from_dir_dir;
							pkt.srcnode = inode;
							pkt.dstnode = nodeaddr_to_lru_dir;
							pkt.type = st;
							NoCtxQ.push(pkt);
							
						}
						if ( first_st == st) first_st = IDLE;
						else second_st = IDLE;
						
						st = IDLE;
						i = 1;
						//next_trigger(1,SC_NS);
						
					}break;
		case DIRACK: {
						//cout<<"MEM_LSU::"<<__func__<<" : "<< __LINE__<<endl;
						if(nodeaddr_to_lru_dir == inode)
						{
							cout<<"MEM_LSU::"<<inode<<", "<<__func__<<" : "<< __LINE__<<"error!! "<<endl;							
						}
						else
						{
							//make packet and put in the queue
							NoCCommPacket pkt;
							pkt.ack = ack_to_node_dir;
							pkt.address = address_from_dir_dir;
							pkt.req_count = req_count_from_dir_dir;
							pkt.srcnode = inode;
							pkt.dstnode = nodeaddr_to_lru_dir;
							pkt.type = st;
							NoCtxQ.push(pkt);
						}
						if ( first_st == st) first_st = IDLE;
						else second_st = IDLE;
						
						st = IDLE;
						i = 1;
						//next_trigger(1,SC_NS);
						}break;
		case DIRCE_TO_L3MEM: {
						//cout<<"MEM_LSU::"<<inode<<", "<<__func__<<" : "<< __LINE__<<endl;
						NoCCommPacket pkt;
						pkt.en = ce_to_l3_dir;
						pkt.ex = false;
						pkt.rw = rd_wr_to_l3_dir;
						pkt.address = addr_to_l3_dir;
						pkt.req_count = req_count_to_l3_dir;
						pkt.srcnode = inode;
						pkt.dstnode = nodeaddr_to_lru_dir;
						pkt.redirectionnode = dst_nodeaddr_to_l3_dir;
						pkt.type = st;
						if(nodeaddr_to_lru_dir == inode)
						{							
							local_memQ.push(pkt);
							
						}
						else
						{
							//make packet and put in the queue							
							NoCtxQ.push(pkt);
							
						}
						if ( first_st == st) first_st = IDLE;
						else second_st = IDLE;
						
						st = IDLE;
						i = 1;
						//next_trigger(1,SC_NS);
						}
						break;
		case DIRFWD_TO_NPC: {
						if(nodeaddr_to_lru_dir == inode)
						{							
							cout<<"MEM_LSU::"<<inode<<", "<<__func__<<" : "<< __LINE__<<" Error!!"<<endl;							
						}
						else
						{
							//make packet and put in the queue
							NoCCommPacket pkt;
							if(forward_spillreq_from_dir_dir == true) {
								pkt.en = forward_spillreq_from_dir_dir;
								pkt.rw = true; // jus a decode 
								pkt.ack = false;
							} else if(forward_req_from_dir_dir == true) {
								pkt.en = forward_req_from_dir_dir;
								pkt.rw = false;
								pkt.ack = false;
							} else {
								pkt.en = replace_req_dcc_ack_from_dir_dir;
								pkt.rw = true;
								pkt.ack = true;
								pkt.ex = dcc_transfer_en_to_npc_dir;
							}
							pkt.req_count = req_count_to_l3_dir;
							pkt.address = addr_to_l3_dir;
							pkt.srcnode = inode;
							pkt.dstnode = nodeaddr_to_lru_dir;
							pkt.redirectionnode = dst_nodeaddr_to_l3_dir;
							pkt.type = st;
							NoCtxQ.push(pkt);
						}
						if ( first_st == st) first_st = IDLE;
						else second_st = IDLE;
						
						st = IDLE;
						i = 1;
						//next_trigger(1,SC_NS);
						} break;
		default: cout<<"ERROR: "; 
			cout<<"MEM_LSU::"<<inode<<", "<<__func__<<" : "<< __LINE__<<endl;
		
		}
		
		i--;
	}
					
}

void MEM_LSU::lsu_rx_dir(void ) {
//cout<<"MEM_LSU::"<<inode<<", "<<__func__<<" : "<< __LINE__<<endl;
while(1){
	while (local_dirQ.empty() == true) wait();
	switch(local_dirQ.front().type) {
	case MEMDIRACK: {
			//cout<<"MEM_LSU::"<<inode<<", "<<__func__<<" : "<< __LINE__<<endl;
			node_address_from_lru_dir = local_dirQ.front().srcnode; //node_address_to_lru_mem;
			ack_from_l3_dir = local_dirQ.front().ack; //ack_from_dir_mem;
			addr_from_l3_dir = local_dirQ.front().address; //addr_from_dir_mem;
			req_count_from_l3_dir = local_dirQ.front().req_count; 
			wait();	
			ack_from_l3_dir = false;					
			}break;
	case L2CEEXEVT: {
			inv_ack_to_dir_dir = local_dirQ.front().ack ;
			cl_evt_info_to_dir_dir = local_dirQ.front().evt;				
			req_count_to_dir_dir = local_dirQ.front().req_count; 
			address_to_dir_dir = local_dirQ.front().address; 
			ex_to_dir_dir = local_dirQ.front().ex; 
			rd_wr_to_dir_dir = local_dirQ.front().rw; 
			ce_to_dir_dir = local_dirQ.front().en;
			node_address_from_lru_dir = local_dirQ.front().srcnode;
			wait();
			cl_evt_info_to_dir_dir = false;
			ex_to_dir_dir = false;
			ce_to_dir_dir = false;		
			inv_ack_to_dir_dir = false;
			wait();
			}break;
	
	default:cout<<"ERROR: "<<local_dirQ.front().type; 
				cout<<"MEM_LSU::"<<inode<<", "<<__func__<<" : "<< __LINE__<<endl;
			wait();
	}
	local_dirQ.pop();
	
	}
}
void MEM_LSU::lsu_rx_mem(void ){
//cout<<"MEM_LSU::"<<inode<<", "<<__func__<<" : "<< __LINE__<<endl;
while(1){
	while (local_memQ.empty() == true) wait();
	switch(local_memQ.front().type) {
	case DIRCE_TO_L3MEM: {
			//cout<<"MEM_LSU::"<<inode<<", "<<__func__<<" : "<< __LINE__<<" with redirection l2 address: "<<local_dirP.redirectionnode<<endl;
			if(local_memQ.front().ex == true){
				without_dir_ack_mem = true;
			} else 
				without_dir_ack_mem = false;
			nodeaddr_from_lru_dir_mem = local_memQ.front().srcnode;
			rd_wr_from_dir_mem = local_memQ.front().rw; // rd_wr_to_l3_dir;
			addr_from_dir_mem = local_memQ.front().address; // addr_to_l3_dir;
			req_count_from_dir_mem = local_memQ.front().req_count;
			dst_nodeaddr_from_dir_mem = local_memQ.front().redirectionnode; //dst_nodeaddr_to_l3_dir;
			ce_from_dir_mem = local_memQ.front().en; //ce_to_l3_dir;
			wait();
			ce_from_dir_mem = false;
			without_dir_ack_mem = false;
			cout<<"MEM_LSU::"<<inode<<", "<<__func__<<" : "<< __LINE__<<endl;
			}
			break;	
	case  L2DATA_TO_L3MEM: {
			cout<<"MEM_LSU::"<<inode<<", "<<__func__<<" : "<< __LINE__<<endl;
			address_from_l2_en_mem = local_memQ.front().en;
			address_from_l2_mem = local_memQ.front().address;
			req_count_from_l2_mem = local_memQ.front().req_count;
			nodeaddr_from_lru_l2_mem = local_memQ.front().srcnode;
			if(local_memQ.front().rw == true){
			data_from_l2_mem->write_data(&local_memQ.front().data);
			}
			wait();	
			address_from_l2_en_mem = false;
			}break;
	default:cout<<"ERROR: "<<local_memQ.front().type; 
		cout<<"MEM_LSU::"<<inode<<", "<<__func__<<" : "<< __LINE__<<endl;
		wait();
	}
	local_memQ.pop();
	}
}

void MEM_LSU::receive_packet(int src_id, LisNocPacket* packet){
	//cout<<"MEM_LSU::"<<inode<<", "<<__func__<<" : "<< __LINE__<<" from "<< packet->src_id<<endl;
	NoCCommPacket pkt;
	bool status = true;
	pkt.dstnode = packet->dst_id;
	pkt.srcnode = packet->src_id;
	pkt.redirectionnode = packet->redirectionnode;
	pkt.type = (LSU_STATE)packet->type;
	pkt.req_count = packet->req_count;
	pkt.rw = (packet->read_or_write == 1) ? true:false;
	if(packet->address.empty()!=true){
	pkt.address = packet->address.back();
	packet->address.pop_back();
	}
	pkt.ex = packet->ex;
	pkt.evt = packet->evt;
	pkt.en = packet->en;
	pkt.ack = packet->ack;	
	pkt.inv = packet->inv;
	switch(packet->type){
		case MEMDIRACK:{					
					}
					break;
		case L2DATA_TO_L3MEM:{					
					if(packet->read_or_write == 1) {
					for(int i=CL_SIZE_WORDWISE;i>0;i--){
					pkt.data.data[i-1] = packet->data.back();
					packet->data.pop_back();
					} 
					}					
					}
					//cout<<"L2DATA_TO_L3MEM "<<__func__<<" : "<< __LINE__<<endl;
					break;
		case L2CEEXEVT:{										
					}
					//cout<<"L2CEEXEVT "<<__func__<<" : "<< __LINE__<<endl;
					break; 		
		case DIRCE_TO_L3MEM:{				
					}
					
					break;
		default: cout<<"ERROR: "<<packet->type; 
				 cout<<"MEM_LSU::"<<inode<<", "<<__func__<<" : "<< __LINE__<<endl;
					status = false;
					//wait();
		}
	if (status == true)
		NoCrxQ.push(pkt);

}

void MEM_LSU::lsu_rx_thread(void ) {
//cout<<"MEM_LSU::"<<inode<<", "<<__func__<<" : "<< __LINE__<<endl;
	while(1){
		while(NoCrxQ.empty() == true) {
		wait();
		}
		NoCCommPacket pkt = NoCrxQ.front();
		NoCrxQ.pop();
		NoCCommPacket local_P;
		local_P.ack = pkt.ack;
		local_P.srcnode = pkt.srcnode;
		local_P.dstnode = pkt.dstnode;
		local_P.address = pkt.address;
		local_P.redirectionnode = pkt.redirectionnode;
		local_P.req_count = pkt.req_count;
		local_P.type = pkt.type;
		local_P.evt = pkt.evt;
		local_P.ex = pkt.ex;
		local_P.rw = pkt.rw;
		local_P.en = pkt.en ;
		local_P.inv = pkt.inv;
		switch(pkt.type) {				
		case MEMDIRACK: {
						local_dirQ.push(local_P);											
						}
						break;
		case L2DATA_TO_L3MEM: {		
						if(pkt.rw == true){
						local_P.data = pkt.data;
						}
						local_memQ.push(local_P);						
						}break;
		case L2CEEXEVT: {
						//cout<<"MEM_LSU::"<<inode<<", "<<__func__<<" : "<< __LINE__<<" received L2 req from "<<pkt.srcnode<<endl;
						local_dirQ.push(local_P);						
						}
						break;
		case DIRCE_TO_L3MEM: {			
						//cout<<"MEM_LSU::"<<inode<<", "<<__func__<<" : "<< __LINE__<<" with redirection l2 address: "<<pkt.redirectionnode<<" address:0x"<<hex<<pkt.address<<dec<<endl;
						local_memQ.push(local_P);
						}
						break;					
		default: cout<<"ERROR: "; 
				cout<<"MEM_LSU::"<<inode<<", "<<__func__<<" : "<< __LINE__<<endl;
		wait();
		}
	}

}
