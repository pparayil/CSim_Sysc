
#include "common_lsu.h"
#include <vector>


bool CM_LSU::set_packet(int src_id, LisNocPacket* packet){
	
	if(NoCtxQ.empty() == true) {
	return false;
	}
	else
	{
	if(packet == NULL) {
		cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<<__LINE__<<" Error"<<endl; 
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
	//cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<<__LINE__<<endl;
	switch(pkt.type){
		case IDLE: {
					//cout<<"NO PKT"<<endl;
					return false;
					}
					break;
		case NPC_NPC_DATA_FWD:{
					if((pkt.ack == true) && (pkt.rw == true)) {
						packet->size = 2;
					} else {
						packet->size = 10;
						for(int i=0;i<CL_SIZE_WORDWISE;i++){
						packet->data.push_back(pkt.data.data[i]);					
						}
					}
					//cout<<"data[7]: "<< pkt.data.data[7]<<" and from lisnoc side data[7]: "<<packet->data.back()<<endl;
					} break;
		case L2CEEXEVT:{
					packet->size = 2;
					}
					break;
		case L2DATA_TO_L3MEM:{
					if(pkt.rw == true) {
						packet->size = 10;
						for(int i=0;i<CL_SIZE_WORDWISE;i++){
						packet->data.push_back(pkt.data.data[i]);					
						}
					}
					else {
						packet->size = 2;
					}
					//cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<<__LINE__<<" data[7]: "<< pkt.data.data[7]<<" and from lisnoc side data[7]: "<<packet->data.back()<<endl;
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
		default: cout<<"ERROR: "<<pkt.type; 
				 cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<endl;				 
		}
	
	packet->flit_left = packet->size;
	}
	//cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<<__LINE__<<" return value:"<<(unsigned int) ret<<endl;
	return true;
}

void CM_LSU::lsu_tx_method(void ) {
	////cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<<__LINE__<<endl;
	cout<<flush <<dec;
	LSU_STATE st = IDLE; 
	LSU_STATE first_st =IDLE,second_st =IDLE;
	int i=3; //
	while (i!=0)
	{
		switch (st) {
		case IDLE: {
					//l2 side
					if(ce_to_dir_l2 == true || ex_to_dir_l2 == true || cl_evt_info_to_dir_l2 == true || inv_ack_to_dir_l2 == true)
					{
						st = L2CEEXEVT;
						//cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<endl;
						first_st = L2CEEXEVT;
					}
					else if((address_to_l3_en_l2 == true) && (generate_wr_req_npc_l2 == false)){
						st = L2DATA_TO_L3MEM;
						//cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<" addr:"<<hex<< address_to_l3_l2<<dec<<endl;
                        first_st = L2DATA_TO_L3MEM;
					} else if ((address_to_l3_en_l2 == true) && (generate_wr_req_npc_l2 == true)) {
						// direct WB from NPC to SC
						st = L2DATA_WB_TO_L3MEM;
						first_st = L2DATA_WB_TO_L3MEM;
					} else if((forward_data_to_npc_l2 == true) || (replace_data_dcc_ack_to_npc_l2 == true) || (replace_data_dcc_to_npc_l2 == true)) {
						st = NPC_NPC_DATA_FWD;
						first_st = NPC_NPC_DATA_FWD;
					}
					
					//dir side
					if((inv_to_node_dir == true) || (flush_frm_npc_dir == true)) {
						//cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<endl;
						if (first_st != IDLE) 
							second_st = DIRINV;
						else 
							{
							first_st = DIRINV;
							st = DIRINV;
							}
					}
					else if(ack_to_node_dir == true) {
						
						//cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<endl;
						if (first_st != IDLE) 
							second_st = DIRACK;
						else 
							{
							first_st = DIRACK;
							st = DIRACK;
							}
					}
					else if(ce_to_l3_dir == true) {
						
						//cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<endl;
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
					}
					} break;
					// check or find node address and see if that belongs to local node.
		case L2CEEXEVT: {
						//cout<<"CM_LSU::"<<inode<<" @"<<__func__<<" : "<<__LINE__<<endl;
						//decode address to destination node
						unsigned int cl_address = address_to_dir_l2>>(OFFSET_LENGTH);
						unsigned int dst_node = ((unsigned int)cl_address )&( (unsigned int)(NO_OF_NODES-1)); /*0x11 & address will give the dst dir bank address*/
						NoCCommPacket local_l2P;
						local_l2P.en = ce_to_dir_l2;
						local_l2P.ex = ex_to_dir_l2;
						local_l2P.rw = rd_wr_to_dir_l2;
						local_l2P.evt = cl_evt_info_to_dir_l2;
						local_l2P.ack = inv_ack_to_dir_l2;
						local_l2P.address = address_to_dir_l2;
						local_l2P.srcnode = inode;
						local_l2P.dstnode = dst_node;
						local_l2P.type = st;
						local_l2P.req_count = req_count_to_dir_l2;
						if(dst_node == inode)
						{
							local_dirQ.push(local_l2P);
							
						}
						else
						{
							//make packet and put in the queue							
							NoCtxQ.push(local_l2P);
							
						}
						first_st = IDLE;
						st = second_st;
						if(second_st == IDLE) {//wait();
							
							st = IDLE;
							i = 1;
							//next_trigger(1, SC_NS);
						}					
						}break;
		case L2DATA_WB_TO_L3MEM: 
		case L2DATA_TO_L3MEM: {
						//cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<" address:0x"<< hex<<address_to_l3_l2<<dec <<endl;
						//decode address to destination node
						unsigned int cl_address = address_to_l3_l2>>(OFFSET_LENGTH);
						unsigned int dst_node , dnode = ((unsigned int)cl_address )&( (unsigned int)(NO_OF_NODES-1)); 
						#ifdef L3_TRUE_1
						if(((cl_address >> NO_OF_NODES_2) & 0x01) == 0) // 7 = 5(offset) + 2(inode)
						{ 
							dst_node  = dnode;//cout<<"1"<<endl;
						}
						else
						{
							if((dnode & 0x01) == 0x0)
							{
								dst_node  = dnode+1;//cout<<"2"<<endl;
							}
							else
							{
								dst_node  = dnode-1;//cout<<"3"<<endl;
							}
						}
						#else
						#ifdef L3_TRUE_0
							dst_node  = dnode;
						#else
							dst_node  = NODE_WITH_SDRAM;
						#endif
						#endif

						//cout<<"dst_node: L2data to L3mem: "<<dst_node<<" dnode:"<<dnode<<" address:0x"<< hex<<address_to_l3_l2<<dec <<endl;
						NoCCommPacket pkt;
						if(st == L2DATA_TO_L3MEM) {
						(data_to_l3_l2)->read_data(&pkt.data);
						pkt.address = address_to_l3_l2;
						pkt.req_count = req_count_to_l3_l2;
						pkt.en = address_to_l3_en_l2;
						pkt.rw = true;
						pkt.srcnode = nodeid_to_lsu_l2;
						pkt.redirectionnode = nodeid_to_lsu_l2;
						pkt.dstnode = dst_node;
						pkt.type = st;
						cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<" address:0x"<< hex<<pkt.address<<dec<<" l2node:"<<pkt.redirectionnode<<endl;
						} 
						else if (st == L2DATA_WB_TO_L3MEM){
						pkt.address = address_to_l3_l2;
						pkt.req_count = req_count_to_l3_l2;
						pkt.en = address_to_l3_en_l2;
						pkt.rw = true;
						pkt.ex = rd_wr_to_dir_l2;
						pkt.srcnode = dnode;
						pkt.dstnode = dst_node;
						pkt.redirectionnode = nodeid_to_lsu_l2;
						pkt.type = DIRCE_TO_L3MEM;
						cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<" address:0x"<< hex<<pkt.address<<dec<<" l2node:"<<pkt.redirectionnode<<endl;
						}
						if(dst_node == inode)
						{
							#if 0						
							local_l3Q.push(pkt); 
							#else
							cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<"error !! as l2 and mem cant be in the same node"<<endl;
							#endif
						}
						
						else
						{
							//make packet and put in the queue
							NoCtxQ.push(pkt);							
						}
						first_st = IDLE;
						st = second_st;
						if(second_st == IDLE) {
							st = IDLE;
							i = 1;
							//next_trigger(1, SC_NS);
						}
					}break;
		case NPC_NPC_DATA_FWD:{
						NoCCommPacket pkt;	
						pkt.address = address_to_l3_l2;
						pkt.req_count = req_count_to_l3_l2;
						pkt.srcnode = inode;
						pkt.dstnode = nodeid_to_lsu_l2;
						pkt.type = st;
						if(pkt.dstnode == inode)
						{
							if(replace_data_dcc_ack_to_npc_l2 == true) {
								pkt.en = replace_data_dcc_ack_to_npc_l2;
								pkt.ack = true;
								pkt.rw = true;
								pkt.ex = false;							
							} else if (replace_data_dcc_to_npc_l2 == true){
								(data_to_l3_l2)->read_data(&pkt.data);
								pkt.en = replace_data_dcc_to_npc_l2;
								pkt.ack = false;
								pkt.rw = true;
								pkt.ex = spill_data_dirty_to_npc_l2;								
							} else if (forward_spill_data_to_npc_l2 == true){								
								(data_to_l3_l2)->read_data(&pkt.data);
								pkt.en = forward_spill_data_to_npc_l2;
								pkt.ack = false;
								pkt.rw = true;								
							} else {
								cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<< " Error fwd to self!!!"<<endl;
							}
							local_l2Q.push(pkt);
						}						
						else
						{
							//make packet and put in the queue							
							if(replace_data_dcc_ack_to_npc_l2 == true) {
								pkt.en = replace_data_dcc_ack_to_npc_l2;
								pkt.ack = true;
								pkt.rw = true;
								pkt.ex = false;							
							} else if (replace_data_dcc_to_npc_l2 == true){
								(data_to_l3_l2)->read_data(&pkt.data);
								pkt.en = replace_data_dcc_to_npc_l2;
								pkt.ack = false;
								pkt.rw = true;
								pkt.ex = spill_data_dirty_to_npc_l2;								
							} else if (forward_spill_data_to_npc_l2 == true) {
								(data_to_l3_l2)->read_data(&pkt.data);
								pkt.en = forward_spill_data_to_npc_l2;
								pkt.ack = false;
								pkt.rw = false;	
							} else {
								(data_to_l3_l2)->read_data(&pkt.data);								
								pkt.en = forward_data_to_npc_l2;
								pkt.ack = true;
								pkt.rw = false;
							}
							NoCtxQ.push(pkt);							
						}
						first_st = IDLE;
						st = second_st;
						if(second_st == IDLE) {
							st = IDLE;
							i = 1;
							//next_trigger(1, SC_NS);
						}
					}break;
		case DIRINV: {
						//cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<endl;
						NoCCommPacket local_dirP;
						local_dirP.address = inv_addr_to_node_dir;
						local_dirP.req_count = req_count_from_dir_dir;
						local_dirP.srcnode = inode;
						local_dirP.dstnode = nodeaddr_to_lru_dir;
						local_dirP.type = st;
						if((flush_frm_npc_dir == true) && (inv_to_node_dir == false)){
							local_dirP.evt = false;
							local_dirP.inv = true;
							local_dirP.redirectionnode = dst_nodeaddr_to_l3_dir;
						} else if ((flush_frm_npc_dir == false) && (inv_to_node_dir == true)){
							local_dirP.evt = true;
							local_dirP.inv = true;
							local_dirP.ex = spill_inv_to_node_dir;
						}
						if(nodeaddr_to_lru_dir == inode)
						{
							
							local_l2Q.push(local_dirP);		
						}
						else
						{
							//make packet and put in the queue							
							NoCtxQ.push(local_dirP);
							
						}
						if ( first_st == st) first_st = IDLE;
						else second_st = IDLE;
						
						st = IDLE;
						i = 1;
						//next_trigger(1, SC_NS);
					}break;
		case DIRACK: {
						//cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<endl;
						NoCCommPacket local_dirP;
						local_dirP.ack = ack_to_node_dir;
						local_dirP.address = address_from_dir_dir;
						local_dirP.req_count = req_count_from_dir_dir;
						local_dirP.srcnode = inode;
						local_dirP.dstnode = nodeaddr_to_lru_dir;
						local_dirP.type = st;
						if(nodeaddr_to_lru_dir == inode)
						{
							// local transfer							
							local_l2Q.push(local_dirP);		
						}
						else
						{
							//make packet and put in the queue
							
							NoCtxQ.push(local_dirP);
							
						}
						if ( first_st == st) first_st = IDLE;
						else second_st = IDLE;
						
						st = IDLE;
						i = 1;
						//next_trigger(1, SC_NS);
						}break;
		case DIRCE_TO_L3MEM: {
						//cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<< "from dir to l3:" <<nodeaddr_to_lru_dir <<dec <<endl;
					
						if(nodeaddr_to_lru_dir == inode)
						{
							// local transfer this can be filled in only once we have L3 or mem added to the lsu							
							cout<<"Error CMLSU doesnt have memory"<<endl;			
							
						}
						else
						{
							//make packet and put in the queue
							NoCCommPacket pkt;
							pkt.en = ce_to_l3_dir;
							pkt.ex = false;
							pkt.rw = rd_wr_to_l3_dir;
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
						//next_trigger(1, SC_NS);
						}
						break;
		case DIRFWD_TO_NPC: {
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
						if(nodeaddr_to_lru_dir == inode)
						{
							local_l2Q.push(pkt);										
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
						//next_trigger(1, SC_NS);
						} break;			
		default: cout<<"ERROR: "<<st; 
			cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<endl;
//			wait();
		
		}
		i--;
	}
					
}

void CM_LSU::lsu_rx_dir(void ){
//cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<<__LINE__<<endl;
while(1){
	while (local_dirQ.empty() == true) wait();
	switch(local_dirQ.front().type) {
	case L2CEEXEVT: {
			//cout<<"CM_LSU::"<<inode<<" @"<<__func__<<" : "<<__LINE__<<endl;
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
	case MEMDIRACK: {
			//cout<<"MEM_LSU::"<<inode<<", "<<__func__<<" : "<< __LINE__<<endl;
			node_address_from_lru_dir = local_dirQ.front().srcnode; 
			ack_from_l3_dir = local_dirQ.front().ack; 
			addr_from_l3_dir = local_dirQ.front().address; 
			req_count_from_l3_dir = local_dirQ.front().req_count; 
			wait();	
			ack_from_l3_dir = false;	
			wait();
			}break;
	default:cout<<"ERROR: "<<local_dirQ.front().type; 
				cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<endl;
	}
	local_dirQ.pop();
	}
}
void CM_LSU::lsu_rx_l2(void ){
//cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<<__LINE__<<endl;
while(1){
	while (local_l2Q.empty() == true) wait();
	NoCCommPacket local_l2P = local_l2Q.front();
	switch(local_l2P.type) {
	case DIRINV: {
			//cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<< " inv l2:" <<inode<< endl;
			if(local_l2P.evt == true){
				inv_frm_dir_l2 = local_l2P.inv; 
				spill_inv_frm_dir_l2 = local_l2P.ex;
			} else {
				flush_frm_npc_dir_l2 = local_l2P.inv; 
			}
			inv_addr_from_dir_l2 = local_l2P.address; 
			req_count_from_dir_l2 = local_l2P.req_count;
			nodeid_from_lsu_l2 = local_l2P.redirectionnode;
			wait();
			inv_frm_dir_l2 = false;
			flush_frm_npc_dir_l2 = false;
			spill_inv_frm_dir_l2 = false;			
			wait();
			}break;
	case DIRACK: {
			//cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<endl;
			ack_from_dir_l2 = local_l2P.ack; 
			address_from_dir_l2 = local_l2P.address; 
			req_count_from_dir_l2 = local_l2P.req_count;
			//node_address_from_lru_l2 = local_l2P.srcnode;
			wait();
			ack_from_dir_l2 = false;
			wait();
			}break;
	case DIRFWD_TO_NPC: {
			if(local_l2P.ack == true) {
				replace_req_dcc_ack_from_dir_l2 = local_l2P.en;
				ack_from_dir_l2 = local_l2P.ack;
				dcc_transfer_en_from_dir_l2 = local_l2P.ex; 
				cout<<"CM_LSU::"<<inode<<" "<<__func__<<" : "<< __LINE__<<"DCCACK from dir "<<endl;
			} else {
				if(local_l2P.rw == true) {
					forward_req_from_dir_l2 = true;
					ack_from_dir_l2 = false;
					forward_spillreq_from_dir_l2 = local_l2P.en;
					cout<<"CM_LSU::"<<inode<<" "<<__func__<<" : "<< __LINE__<<"SPILLRD from dir "<<endl;
				} else {
					forward_req_from_dir_l2 = local_l2P.en; 
					cout<<"CM_LSU::"<<inode<<" "<<__func__<<" : "<< __LINE__<<"FWRD from dir "<<endl;
				}
			}
			address_from_dir_l2 = local_l2P.address; 
			req_count_from_dir_l2 = local_l2P.req_count;
			nodeid_from_lsu_l2 = local_l2P.redirectionnode;
			wait();
			forward_req_from_dir_l2 = false;
			forward_spillreq_from_dir_l2 = false;
			replace_req_dcc_ack_from_dir_l2 = false;
			dcc_transfer_en_from_dir_l2 = false;
			ack_from_dir_l2 = false;
			wait();
			}break;
	case MEML2ACK: {
			ack_from_l3_l2 = local_l2P.ack;
			ack_from_npc_l2 = false;
			address_from_l3_l2 = local_l2P.address;
			if(local_l2P.rw == false){
			data_from_l3_l2->write_data(&local_l2P.data);		
			}
			req_count_from_l3_l2 = local_l2P.req_count;
			//node_address_from_lru_l2 = local_l2P.srcnode;	
			wait();
			ack_from_l3_l2 = false;	
			ack_from_npc_l2 = false;
			wait();
			}break;
	case NPC_NPC_DATA_FWD:{
			if(local_l2P.ack == true){
				if(local_l2P.rw == true){
					ack_from_l3_l2 = true;
					replace_data_dcc_ack_from_npc_l2 = true;
				} else {
					ack_from_l3_l2 = local_l2P.ack;
					ack_from_npc_l2 = local_l2P.en;
					forward_spilldata_ack_from_npc_l2 = false;
					data_from_l3_l2->write_data(&local_l2P.data);
				}
			} else{				
				if(local_l2P.rw == true){
					data_from_l3_l2->write_data(&local_l2P.data);
					ack_from_l3_l2 = true;
					replace_data_dcc_from_npc_l2 = true;
					spill_data_dirty_from_npc_l2 = local_l2P.ex;
				} else {
					data_from_l3_l2->write_data(&local_l2P.data);
					ack_from_l3_l2 = true;
					ack_from_npc_l2 = true;
					forward_spilldata_ack_from_npc_l2 = true;
				}
			}			
			address_from_l3_l2 = local_l2P.address;			
			req_count_from_l3_l2 = local_l2P.req_count;
			nodeid_from_lsu_l2 = local_l2P.srcnode;	
			wait();
			ack_from_l3_l2 = false;	
			ack_from_npc_l2 = false;
			replace_data_dcc_ack_from_npc_l2 = false;
			replace_data_dcc_from_npc_l2 = false;
			forward_spilldata_ack_from_npc_l2 = false;
			wait();
			}break;
	default:cout<<"ERROR: "<<local_l2P.type; 
		cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<endl;
	}
	local_l2Q.pop();
	}	
}

void CM_LSU::receive_packet(int src_id, LisNocPacket* packet){
	////cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<<__LINE__<<endl;
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
	//cout<<"CM_LSU::"<<inode<<" @"<<__func__<<" : "<<__LINE__<<endl;
	switch(packet->type){
		case L2CEEXEVT:{										
					}
					//cout<<"CM_LSU::"<<inode<<" @"<<__func__<<" : "<<__LINE__<<endl;
					break;
		case L2DATA_TO_L3MEM:{					
					if(packet->read_or_write == 1) {
					for(int i=CL_SIZE_WORDWISE;i>0;i--){
					pkt.data.data[i-1] = packet->data.back();
					packet->data.pop_back();
					} 
					}				
					//cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<<__LINE__<<endl;
					}
					break;
		case DIRINV:{					
					//cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<<__LINE__<<endl;
					}
					break;
		case DIRACK:{					
					//cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<<__LINE__<<endl;
					}
					break;
		case DIRFWD_TO_NPC: {
					} break;
		case DIRCE_TO_L3MEM:{	
					//cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<<__LINE__<<endl;			
					}
					break;
		case MEMDIRACK:{
					//cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<<__LINE__<<endl;
					}
					break;
		case MEML2ACK: {
					if(packet->read_or_write == 0) {
					for(int i=CL_SIZE_WORDWISE;i>0;i--){
					pkt.data.data[i-1] = packet->data.back();
					////cout<<"CM_LSU:: data[]: "<< pkt.data.data[i-1]<<" and from noxim side data[]: "<<packet->data.back()<<endl;
					packet->data.pop_back();					
					} 
					}	
					//cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<<__LINE__<<endl;
					} 
					break;	
		case NPC_NPC_DATA_FWD:{					
					if(packet->read_or_write == 0) {
					for(int i=CL_SIZE_WORDWISE;i>0;i--){
					pkt.data.data[i-1] = packet->data.back();
					////cout<<"CM_LSU:: data[]: "<< pkt.data.data[i-1]<<" and from noxim side data[]: "<<packet->data.back()<<endl;
					packet->data.pop_back();					
					} 
					}					
					} break;
		default: cout<<"ERROR: ptype: "<<packet->type; 
			  cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<endl;
					status = false;
		}
if (status == true)
	NoCrxQ.push(pkt);

}

void CM_LSU::lsu_rx_thread(void ) {
//cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<<__LINE__<<endl;
	while(1){
		while(NoCrxQ.empty() == true) {
		////cout<<"NO PKT"<<endl;
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
		case L2CEEXEVT: {
						//cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<" Dir received l2 req from "<<pkt.srcnode<<endl;						
						local_dirQ.push(local_P);
						}
						break;
		case NPC_NPC_DATA_FWD:{
						local_P.evt = false;						
						if(pkt.rw == false){
						local_P.data = pkt.data;		
						}
						local_l2Q.push(local_P);	
						}break;
		case DIRINV: {
						//cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<"INV from "<<pkt.srcnode<<endl;	
						local_l2Q.push(local_P);	
						}break;
		case DIRFWD_TO_NPC: {
						//cout<<"CM_LSU::"<<inode<<" "<<__func__<<" : "<< __LINE__<<"FWRD/SPILLRD/DCCACK from dir "<<pkt.dstnode<<endl;						
						local_l2Q.push(local_P);	
					} break;
		case DIRACK: {
						//cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<" from "<<pkt.srcnode<<endl;						
						local_l2Q.push(local_P);							
						}break;
		case MEMDIRACK: {		
						//cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<" from "<<pkt.srcnode<<endl;						
						local_dirQ.push(local_P);									
						}
						break;
		case MEML2ACK: {		//cout<<"@"<<inode<<" CM_LSU:: receive_packet:: data[7]: "<< pkt.data.data[7]<<" @ pkt.req_count: "<< pkt.req_count <<endl;						
						if(pkt.rw == false){
						local_P.data = pkt.data;		
						}
						local_l2Q.push(local_P);						
						}break;					
					
		default: 	cout<<"ERROR: "<<pkt.type;
				cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<endl;
			wait();
		}
	}

}
