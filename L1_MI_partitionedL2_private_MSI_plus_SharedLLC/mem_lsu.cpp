
#include "mem_lsu.h"
#include <vector>


bool MEM_LSU::set_packet(int src_id, LisNocPacket* packet){
	
	if(NoCtxQ.empty() == true) {
	return false;
	}
	else
	{
	if(packet == NULL) { cout<<"MEM_LSU::"<<inode<<", "<<__func__<<" : "<< __LINE__<<" Error NULL packet"<<endl; //packet = new LisNocPacket;
	}
	NoCCommPacket pkt = NoCtxQ.front();	
	packet->type = pkt.type;
	packet->req_count = pkt.req_count;
	//cout<<"MEM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<" to "<< packet->dst_id<<endl;
	switch(pkt.type){
		case IDLE: {
					//cout<<"NO PKT"<<endl;
					return false;
					}
					break;
		case DIRINV:{
					packet->inv = pkt.inv;
					packet->evt = pkt.evt;
					packet->address.push_back(pkt.address);
					packet->dst_id = pkt.dstnode;
					packet->size = 2;
					}
					break;
		case DIRACK:{
					packet->ack = pkt.ack;
					packet->evt = false;
					packet->address.push_back(pkt.address);
					packet->dst_id = pkt.dstnode;
					packet->size = 2;
					}
					break;
		case DIRCE_TO_LLCMEM:{
					packet->read_or_write = pkt.rw;
					packet->evt = pkt.evt;
					packet->inv = pkt.inv;					
					packet->ex = pkt.ex;
					packet->en = pkt.en;	
					packet->address.push_back(pkt.address);
					packet->redirectionnode = pkt.redirectionnode;
					packet->dst_id = pkt.dstnode;
					packet->size = 2;
					}
					break;
		case LLCACKL2: {
					packet->ack = pkt.ack;
					packet->read_or_write = pkt.rw;
					if(pkt.rw == false) {
						for(int i=0;i<CL_SIZE_WORDWISE;i++){
						packet->data.push_back(pkt.data.data[i]);
						}
						packet->size = 10;
					} else {
						packet->size = 2;
					}
					packet->address.push_back(pkt.address);					
					packet->dst_id = pkt.dstnode;					
					}
					break;
		case LLCACKDIR:{
					packet->ack = pkt.ack;
					packet->address.push_back(pkt.address);
					packet->dst_id = pkt.dstnode;
					packet->size = 2;
					}
					break;
		case LLCREQMEM: {
					packet->read_or_write = pkt.rw;
					packet->address.push_back(pkt.address);
					packet->en = pkt.en;
					if(pkt.rw) {
					for(int i=0;i<CL_SIZE_WORDWISE;i++){
					packet->data.push_back(pkt.data.data[i]);
					}
					}
					packet->dst_id = pkt.dstnode;
					if(pkt.rw)
					packet->size = 10;
					else
					packet->size = 2;
					}
					break;
		case MEMDIRACK:{
					cout<<"MEM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<" Error"<<endl;
					} break;
		case MEMLLCACK: {
					packet->ack = pkt.ack;
					packet->read_or_write = pkt.rw;
					packet->address.push_back(pkt.address);
					if(pkt.rw == true) {
						packet->size = 2;
					}
					else {
						packet->size = 10;
						for(int i=0;i<CL_SIZE_WORDWISE;i++){
						packet->data.push_back(pkt.data.data[i]);
						}
					}
					packet->dst_id = pkt.dstnode;
				} break;
		default: 	cout<<"ERROR: "<<pkt.type; 
				cout<<"MEM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<endl;
		}
	packet->src_id = pkt.srcnode;
	NoCtxQ.pop();
	}
	packet->flit_left = packet->size;
	return true;
}

void MEM_LSU::lsu_tx_method(void ) {
	////cout<<"MEM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<endl;
	cout<<flush <<dec;
	while(1) {
	LSU_STATE st = IDLE; 
	LSU_STATE first_st =IDLE,second_st =IDLE,third_st = IDLE;
	int i=3; //
	while (i!=0)
	{
		switch (st) {
		case IDLE: {
					//dir side
					if((inv_to_node_dir == true) || (flush_frm_npc_dir == true)) {
						//cout<<"MEM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<endl;						
						first_st = DIRINV;
						st = DIRINV;
					}
					else if(ack_to_node_dir == true) {
						//cout<<"MEM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<endl;
						first_st = DIRACK;
						st = DIRACK;
					}
					else if(ce_to_llc_dir == true) {
						//cout<<"MEM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<endl;
						first_st = DIRCE_TO_LLCMEM;
						st = DIRCE_TO_LLCMEM;
					}	
					//llc side
					if (ack_to_dir_llc == true) {
						//cout<<"MEM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<endl;
						if(first_st == IDLE){
							first_st = LLCACKDIR;
							st = LLCACKDIR;
						}
						else 
							second_st = LLCACKDIR;						
					  
					}
					else if(ack_to_l2_llc == true) {
						//cout<<"MEM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<endl;
						if(first_st == IDLE){
							first_st = LLCACKL2;
							st = LLCACKL2;
						}
						else 
							second_st = LLCACKL2;
						
					}
					else if(addr_en_to_mem_llc == true) {
						//cout<<"MEM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<endl;
						if(first_st == IDLE){
							first_st = LLCREQMEM;
							st = LLCREQMEM;
						}
						else 
							second_st = LLCREQMEM;
						
					}

					if (trans_ack_from_mem == true)
					{
						//cout<<"MEM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<endl;
						if(first_st == IDLE){
							first_st = MEMLLCACK;
							st = MEMLLCACK;
						}
						else if(second_st == IDLE){
							second_st = MEMLLCACK;
						}
						else third_st = MEMLLCACK;
						
					}
					if(first_st == IDLE && second_st == IDLE && third_st == IDLE)
					{
					st = IDLE;	
					i = 1;
					//next_trigger(1, SC_NS);
					}
					
					}
					break;
					// check or find node address and see if that belongs to local node.
		
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
							} else if ((flush_frm_npc_dir == false) && (inv_to_node_dir == true)){
								pkt.evt = true;
								pkt.inv = true;
							}
							pkt.address = inv_addr_to_node_dir;
							pkt.req_count = req_count_from_dir_dir;
							pkt.srcnode = inode;
							pkt.dstnode = nodeaddr_to_lru_dir;
							pkt.type = st;
							NoCtxQ.push(pkt);
							
						}
						first_st = IDLE;
						st = second_st;			
						if(second_st == IDLE) { 
						i = 1; 
						//next_trigger(1,SC_NS);
						}
						
					}break;
		case DIRACK: {
						//cout<<"MEM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<endl;
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
						first_st = IDLE;
						st = second_st;			
						if(second_st == IDLE) { 
						i = 1; 
						//next_trigger(1,SC_NS);
						}
						}break;
		case DIRCE_TO_LLCMEM: {
						//cout<<"MEM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<endl;
						NoCCommPacket local_dirP;
						local_dirP.en = ce_to_llc_dir;
						local_dirP.ex = false;
						local_dirP.rw = rd_wr_to_llc_dir;
						local_dirP.address = addr_to_llc_dir;
						local_dirP.req_count = req_count_to_llc_dir;
						local_dirP.srcnode = inode;
						local_dirP.dstnode = nodeaddr_to_lru_dir;
						local_dirP.redirectionnode = dst_nodeaddr_to_llc_dir;
						local_dirP.type = st;
						if(nodeaddr_to_lru_dir == inode)
						{
							// local transfer this can be filled in only once we have LLC or mem added to the lsu
							////cout<<"NO MEM yet"<<endl;							
							local_llcQ.push(local_dirP); 

							
						}
						else
						{							
							NoCtxQ.push(local_dirP);
							
						}
						first_st = IDLE;
						st = second_st;			
						if(second_st == IDLE) { 
						i = 1; 
						//next_trigger(1,SC_NS);
						}
					}break;
		case LLCACKL2: {			
						//cout<<"MEM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<" to node "<<nodeaddr_to_lru_llc<<endl;

						if(nodeaddr_to_lru_llc == inode)
						{
							
							
							cout<<"MEM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<" Error"<<endl;						
						}
						else
						{
							//make packet and put in the queue
							NoCCommPacket pkt;
							pkt.ack = ack_to_l2_llc;
							pkt.rw = ack_rw_to_l2_llc;
							pkt.address = addr_to_l2_to_llc;	
							pkt.req_count = req_count_to_l2_to_llc;							
							data_from_llc->read_data(&pkt.data);
							pkt.srcnode = inode;
							pkt.dstnode = nodeaddr_to_lru_llc;
							pkt.type = st;
							NoCtxQ.push(pkt);
							//cout<<"MEM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<endl;
							
						}
						if ( first_st == st) 
						{ 
						first_st = IDLE;
						st = second_st;
						}
						else 
						{
						second_st = IDLE;
						st = third_st;
						}
						
						if(st == IDLE) { 
						i = 1; 
						//next_trigger(1,SC_NS);
						}
					}
					break;
		case LLCACKDIR:{			
						//cout<<"MEM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<endl;
						NoCCommPacket pkt;
						pkt.ack = ack_to_dir_llc;
						pkt.address = addr_to_dir_to_llc;
						pkt.req_count = req_count_to_dir_to_llc;
						pkt.srcnode = inode;
						pkt.dstnode = nodeaddr_to_lru_llc;
						pkt.type = st;
						if(nodeaddr_to_lru_llc == inode)
						{							
							local_dirQ.push(pkt);			
							
						}
						else
						{
							//make packet and put in the queue							
							NoCtxQ.push(pkt);
							
						}
					        if ( first_st == st) 
						{ 
						first_st = IDLE;
						st = second_st;
						}
						else 
						{
						second_st = IDLE;
						st = third_st;
						}
						
						if(st == IDLE) { 
						i = 1; 
						//next_trigger(1,SC_NS);
						}
					}
					break;
		case LLCREQMEM: {		//cout<<"MEM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<endl;
						NoCCommPacket pkt;
						pkt.rw = r_w_to_mem_llc;
						pkt.address = addr_to_mem_llc;
						pkt.req_count = req_count_to_mem_llc;
						pkt.en = addr_en_to_mem_llc;
						if(r_w_to_mem_llc == true)
						data_to_mem_llc->read_data(&pkt.data);
						pkt.srcnode = inode;
						pkt.dstnode = nodeaddr_to_lru_llc;
						pkt.type = st;
						if(nodeaddr_to_lru_llc == inode)
						{							
							local_memQ.push(pkt);
							//cout<<"MEM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<endl;
						}
						else
						{
							//make packet and put in the queue							
							NoCtxQ.push(pkt);
							//cout<<"MEM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<endl;
							
						}
						if ( first_st == st) 
						{ 
						first_st = IDLE;
						st = second_st;
						}
						else 
						{
						second_st = IDLE;
						st = third_st;
						}
						
						if(st == IDLE) { 
						i = 1; 
						//next_trigger(1,SC_NS);
						}
					}
					break;
			case MEMLLCACK:{		
						cout<<inode<<" MEM_LSU::"<<__func__<<" : "<< __LINE__<<" sending ack to llc:"<<nodeaddr_to_lru_mem<<" for address"<<hex<< addr_from_mem <<dec<<endl;
						NoCCommPacket local_memP;
						local_memP.ack = true;
						local_memP.rw = ack_rw_from_mem;
						local_memP.address = addr_from_mem;
						local_memP.req_count = req_count_from_mem;
						data_mem->read_data(&local_memP.data);
						local_memP.srcnode = inode;
						local_memP.dstnode = nodeaddr_to_lru_mem;
						local_memP.type = st;
						if(nodeaddr_to_lru_mem == inode)
						{							
							local_llcQ.push(local_memP);
							
						}
						else
						{
							//make packet and put in the queue							
							NoCtxQ.push(local_memP);
							
						}
						if ( second_st == IDLE && third_st == IDLE ) 
							first_st = IDLE; 
						else if(third_st == IDLE) 
							second_st = IDLE;
						else 
							third_st = IDLE;						
						st = IDLE;
						i = 1;
						//next_trigger(1, SC_NS);						
					}
					break;
					
		default: cout<<"ERROR: "; 
			cout<<"MEM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<endl;
		
		}
		
		i--;
		////cout<<"MEM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<endl;
	} wait();
}
					
}

void MEM_LSU::lsu_rx_dir(void ){
//cout<<"MEM_LSU::"<<inode<<" , "<<__func__<<" : "<<__LINE__<<endl;
while(1){
	while (local_dirQ.empty() == true) wait();
	switch(local_dirQ.front().type) {
	case L2CEEXEVT: {
			//cout<<"MEM_LSU::"<<inode<<" @"<<__func__<<" : "<<__LINE__<<endl;
			cl_evt_info_to_dir_dir = local_dirQ.front().evt; 
			inv_ack_to_dir_dir = local_dirQ.front().ack ;
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
	case LLCACKDIR:
	case MEMDIRACK: {
			#if 1
			//cout<<"MEM_LSU::"<<inode<<", "<<__func__<<" : "<< __LINE__<<endl;
			node_address_from_lru_dir = local_dirQ.front().srcnode; //node_address_to_lru_mem;
			ack_from_llc_dir = local_dirQ.front().ack; //ack_to_dir_mem;
			addr_from_llc_dir = local_dirQ.front().address; //addr_from_dir_mem;
			req_count_from_llc_dir = local_dirQ.front().req_count; 
			wait();	
			ack_from_llc_dir = false;	
			wait();
			#else 
			//cout<<"MEM_LSU::"<<inode<<", "<<__func__<<" : "<< __LINE__<<" Error"<<endl;	
			#endif
			}break;
	default:cout<<"ERROR: "<<local_dirQ.front().type; 
				cout<<"MEM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<endl;
	}
	local_dirQ.pop();
	}
}
void MEM_LSU::lsu_rx_mem(void ) {
//cout<<"MEM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<endl;
while(1){
while (local_memQ.empty() == true) wait();
	NoCCommPacket local_P = local_memQ.front();
	switch(local_memQ.front().type) {
	case LLCREQMEM: {			
			//cout<<"MEM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<endl;			
			#if 1
			addr_to_mem = local_P.address; 
			req_count_to_mem = local_P.req_count; 
			addr_en_to_mem = local_P.en; 
			r_w_to_mem = local_P.rw; 
			node_address_from_lru_mem = local_P.srcnode;
			if (local_P.rw == true) {
			data_mem->write_data(&local_P.data); 
			}
			wait();
			addr_en_to_mem = false;
			wait();
			#endif
			} break;
	default:cout<<"ERROR: "<<local_P.type; 
		 cout<<"MEM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<endl;
			wait();
	}
	local_memQ.pop();
	}
}

void MEM_LSU::lsu_rx_llc(void ){
//cout<<"MEM_LSU::"<<inode<<" , "<<__func__<<" : "<<__LINE__<<endl;
while(1){
	while (local_llcQ.empty() == true) wait();
	NoCCommPacket local_P = local_llcQ.front();
	switch(local_P.type) {
	case DIRCE_TO_LLCMEM: {
			if(local_llcQ.front().ex == true){
				without_dir_ack_llc = true;
			} else 
				without_dir_ack_llc = false;
			rd_wr_llc = local_P.rw;
			addr_from_dir_to_llc = local_P.address;
			req_count_from_dir_to_llc = local_P.req_count;
			dst_nodeaddr_llc = local_P.redirectionnode;
			node_address_from_lru_llc = local_P.srcnode;
			ce_llc = local_P.en;
			wait();
			ce_llc = false;
			wait();
			} break;
	case L2DATA_TO_LLCMEM: {	
			// local transfer needs llc / mem	
			req_count_from_l2_to_llc = local_P.req_count;	
			addr_from_l2_to_llc = local_P.address;	
			node_address_from_lru_llc = local_P.srcnode;				
			addr_en_from_l2_to_llc = local_P.en;
			data_to_llc->write_data(&local_P.data);			
			//cout<<"MEM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<" for ddress"<<hex<<local_P.address<<", l2 node"<<inode<<dec<<endl;
			wait();
			addr_en_from_l2_to_llc = false;
			wait();
			}break;	
	case MEMLLCACK:{
			trans_ack_from_mem_llc.write(local_P.ack);
			ack_rw_from_mem_llc = local_P.rw;
			addr_from_mem_llc = local_P.address;
			req_count_from_mem_llc = local_P.req_count;
			node_address_from_lru_llc = local_P.srcnode;
			if(local_P.rw == false)	{					
				data_to_mem_llc->write_data(&local_P.data);	
			}		
			wait();						
			trans_ack_from_mem_llc.write(false);	
			wait();		
			//cout<<"MEM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<endl;
		} break;
	default:cout<<"ERROR: "<<local_P.type; 
		cout<<"MEM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<endl;
	}
	local_llcQ.pop();
	}
}

void MEM_LSU::receive_packet(int src_id, LisNocPacket* packet){
	//cout<<"MEM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<" from "<< packet->src_id<<endl;
	NoCCommPacket pkt;
	pkt.dstnode = packet->dst_id;
	pkt.srcnode = packet->src_id;
	pkt.req_count = packet->req_count;
	pkt.type = (LSU_STATE)packet->type;
	switch(packet->type){
		case L2CEEXEVT:{
					pkt.rw = (packet->read_or_write == 1) ? true:false;
					if(packet->address.empty()!=true){
					pkt.address = packet->address.back();
					packet->address.pop_back();
					}
					pkt.ex = packet->ex;
					pkt.evt = packet->evt;
					pkt.en = packet->en;
					pkt.ack = packet->ack;					
					}
					//cout<<"MEM_LSU::"<<inode<<" @"<<__func__<<" : "<<__LINE__<<endl;
					break;
		case L2DATA_TO_LLCMEM:{
					if(packet->address.empty()!=true){
					pkt.address = packet->address.back();
					packet->address.pop_back();
					} /* if its to mem then we might need to split it up and make 8 different transactions*/
					pkt.rw = (packet->read_or_write == 1) ? true:false;
					if(packet->read_or_write == 1) {
					for(int i=CL_SIZE_WORDWISE;i>0;i--){
					pkt.data.data[i-1] = packet->data.back();
					packet->data.pop_back();
					} 
					}// might cause issue while building	
					pkt.en = packet->en;				
					//cout<<"MEM_LSU::"<<inode<<" , "<<__func__<<" : "<<__LINE__<<endl;
					}
					break;
		case DIRINV:{
					pkt.inv = packet->inv;
					pkt.evt = packet->evt;
					if(packet->address.empty()!=true){
					pkt.address = packet->address.back();
					packet->address.pop_back();
					}
					//cout<<"MEM_LSU::"<<inode<<" , "<<__func__<<" : "<<__LINE__<<endl;
					}
					break;
		case DIRACK:{
					#if 0
					pkt.ack = packet->ack ;		
					if(packet->address.empty()!=true){
					pkt.address = packet->address.back();
					packet->address.pop_back();
					}
					//cout<<"MEM_LSU::"<<inode<<" , "<<__func__<<" : "<<__LINE__<<endl;
					#else	
					cout<<"MEM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<" Error"<<endl;
					#endif
					}
					break;
		case DIRCE_TO_LLCMEM:{
					pkt.rw = (packet->read_or_write == 1) ? true:false;
					if(packet->address.empty()!=true){
					pkt.address = packet->address.back();
					packet->address.pop_back();
					}
					pkt.ex = packet->ex;
					pkt.en = packet->en;	
					pkt.redirectionnode = packet->redirectionnode;	
					//cout<<"MEM_LSU::"<<inode<<" , "<<__func__<<" : "<<__LINE__<<endl;			
					}
					break;
		case MEMDIRACK:{
					#if 0
					pkt.ack = packet->ack;	
					if(packet->address.empty()!=true){
					pkt.address = packet->address.back();
					packet->address.pop_back();
					}
					#else
					cout<<"MEM_LSU::"<<inode<<" , "<<__func__<<" : "<<__LINE__<<" Error"<<endl;
					#endif
					}
					break;					
		case MEML2ACK: {
					#if 0	
					if(packet->address.empty()!=true){
					pkt.address = packet->address.back();
					packet->address.pop_back();
					}
					pkt.rw = (packet->read_or_write == 1) ? true:false;
					if(packet->read_or_write == 0) {
					for(int i=CL_SIZE_WORDWISE;i>0;i--){
					pkt.data.data[i-1] = packet->data.back();
					////cout<<"MEM_LSU:: data[]: "<< pkt.data.data[i-1]<<" and from noxim side data[]: "<<packet->data.back()<<endl;
					packet->data.pop_back();					
					} 
					}
					pkt.ack = packet->ack;	
					#else
					cout<<"MEM_LSU::"<<inode<<" , "<<__func__<<" : "<<__LINE__<<" Error"<<endl;
					#endif
					} 
					break;	
		case LLCACKL2: {
					pkt.ack =packet->ack;
					pkt.rw = (packet->read_or_write == 1) ? true:false;
					if(packet->read_or_write == 0) {
					for(int i=CL_SIZE_WORDWISE;i>0;i--){
					pkt.data.data[i-1] = packet->data.back();
					packet->data.pop_back();
					}
					}	
					if(packet->address.empty()!=true){
						pkt.address = packet->address.back();
						packet->address.pop_back();
					}				
					}
					break;
		case LLCACKDIR:{
					pkt.ack =packet->ack;
					if(packet->address.empty()!=true){
						pkt.address = packet->address.back();
						packet->address.pop_back();
					}
					}
					break;
		case LLCREQMEM: {
					pkt.rw = (packet->read_or_write == 1) ? true:false;
					if(packet->read_or_write == 1) {
					for(int i=CL_SIZE_WORDWISE;i>0;i--){
						pkt.data.data[i-1] = packet->data.back();
						packet->data.pop_back();
					}		
					}
					if(packet->address.empty()!=true){
						pkt.address = packet->address.back();
						packet->address.pop_back();
					}
					pkt.en = packet->en;
					//cout<<"MEM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<endl;
					}
					break;
		case MEMLLCACK:{		
					//cout<<"MEM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<endl;
					pkt.rw = (packet->read_or_write == 1) ? true:false;
					if(packet->read_or_write != 1) {					
					for(int i=CL_SIZE_WORDWISE;i>0;i--){
					pkt.data.data[i-1] = packet->data.back();
					packet->data.pop_back();
					}
					}
					if(packet->address.empty()!=true){
						pkt.address = packet->address.back();
						packet->address.pop_back();
					}
					pkt.ack = packet->ack;
				}break;
		default: cout<<"ERROR: ptype: "<<packet->type; 
			  cout<<"MEM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<endl;
		}
	NoCrxQ.push(pkt);

}

void MEM_LSU::lsu_rx_thread(void ) {
//cout<<"MEM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<endl;
	while(1){
		while(NoCrxQ.empty() == true) {
		////cout<<"NO PKT"<<endl;
		wait();
		}
		NoCCommPacket pkt = NoCrxQ.front();
		NoCrxQ.pop();
		switch(pkt.type) {				
		case L2CEEXEVT: {
						//cout<<"MEM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<" Dir received l2 req from "<<pkt.srcnode<<endl;
						NoCCommPacket local_memP;
						local_memP.ack = pkt.ack;
						local_memP.srcnode = pkt.srcnode;
						local_memP.dstnode = pkt.dstnode;
						local_memP.address = pkt.address;
						local_memP.req_count = pkt.req_count;
						local_memP.type = pkt.type;
						local_memP.evt = pkt.evt;
						local_memP.ex = pkt.ex;
						local_memP.rw = pkt.rw;
						local_memP.en = pkt.en ;
						local_dirQ.push(local_memP);
						}
						break;
		case L2DATA_TO_LLCMEM: {
						//cout<<"MEM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<" address:"<<hex<<pkt.address<<" ,l2:"<<pkt.srcnode<<dec<<endl;
						NoCCommPacket local_memP;
						local_memP.data = pkt.data;
						local_memP.srcnode = pkt.srcnode;
						local_memP.dstnode = pkt.dstnode;
						local_memP.address = pkt.address;
						local_memP.req_count = pkt.req_count;
						local_memP.type = pkt.type;
						local_memP.en = pkt.en ;
						local_llcQ.push(local_memP);						
						// needs llc/mem defined
						}break;
		case DIRINV: {
						cout<<"error!! ";
						cout<<"MEM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<endl;							
						wait();
					}break;
		case DIRACK: {
						cout<<"error!! ";
						cout<<"MEM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<endl;
						
						wait();
						}break;
		case DIRCE_TO_LLCMEM: {
						//cout<<"dir to llc ";
						//cout<<"MEM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<endl;	
						NoCCommPacket local_dirP;
						local_dirP.en = pkt.en;
						local_dirP.ex = pkt.ex;
						local_dirP.rw = pkt.rw;
						local_dirP.address = pkt.address;
						local_dirP.req_count = pkt.req_count;
						local_dirP.srcnode = pkt.srcnode;
						local_dirP.dstnode = pkt.dstnode;
						local_dirP.type = pkt.type;
						local_dirP.redirectionnode = pkt.redirectionnode;
						local_llcQ.push(local_dirP);
						}
						break;
		case MEMDIRACK: {	
						#if 0	
						//cout<<"MEM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<" from "<<pkt.srcnode<<endl;
						NoCCommPacket local_memP;
						local_memP.ack = pkt.ack;
						local_memP.srcnode = pkt.srcnode;
						local_memP.dstnode = pkt.dstnode;
						local_memP.address = pkt.address;
						local_memP.req_count = pkt.req_count;
						local_memP.type = pkt.type;
						local_dirQ.push(local_memP);
						#else
						cout<<"Error!! MEM_LSU::"<<__func__<<" : "<< __LINE__<<" from "<<pkt.srcnode<<endl;
						#endif
											
						}
						break;
		case MEML2ACK: {		
						#if 0
						//cout<<"@"<<inode<<" MEM_LSU:: receive_packet:: data[7]: "<< pkt.data.data[7]<<" @ pkt.req_count: "<< pkt.req_count <<endl;
						NoCCommPacket local_memP;
						local_memP.ack = pkt.ack;
						local_memP.srcnode = pkt.srcnode;
						local_memP.dstnode = pkt.dstnode;
						local_memP.address = pkt.address;
						local_memP.req_count = pkt.req_count;
						local_memP.type = pkt.type;
						if(pkt.rw == false){
						local_memP.data = pkt.data;		
						}
						local_l2Q.push(local_memP);
						#else
						cout<<"Error!! @"<<inode<<" MEM_LSU:: receive_packet:: data[7]: "<< pkt.data.data[7]<<" @ address: "<< pkt.address <<endl;
						#endif
						}break;
		case LLCACKL2: {			
						cout<<"error !!!! MEM_LSU::"<<__func__<<" : "<< __LINE__<<endl;
						wait();			
						}
						break;
		case LLCACKDIR:{			
						//cout<<"MEM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<" llcackdir - req:count: "<< pkt.req_count <<endl;
						NoCCommPacket local_memP;
						local_memP.ack = pkt.ack;
						local_memP.srcnode = pkt.srcnode;
						local_memP.dstnode = pkt.dstnode;
						local_memP.address = pkt.address;
						local_memP.req_count = pkt.req_count;
						local_memP.type = pkt.type;
						local_dirQ.push(local_memP);
						}
						break;

		case MEMLLCACK: {
						//cout<<"MEM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<endl;
						NoCCommPacket local_memP;
						local_memP.ack = pkt.ack;
						local_memP.rw = pkt.rw;
						local_memP.srcnode = pkt.srcnode;
						local_memP.dstnode = pkt.dstnode;
						local_memP.address = pkt.address;
						local_memP.req_count = pkt.req_count;
						local_memP.type = pkt.type;
						if (pkt.rw == false) {
							local_memP.data = pkt.data;
						}
						local_llcQ.push(local_memP);												
						}break;
		case LLCREQMEM: {			
						//cout<<"MEM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<endl;								
						#if 1
						NoCCommPacket local_memP;
						local_memP.en = pkt.en;
						local_memP.rw = pkt.rw;
						local_memP.srcnode = pkt.srcnode;
						local_memP.dstnode = pkt.dstnode;
						local_memP.address = pkt.address;
						local_memP.req_count = pkt.req_count;
						local_memP.type = pkt.type;
						if (pkt.rw == true) {
							local_memP.data = pkt.data;
						}
						local_memQ.push(local_memP);
						#endif
						}
						break;
				
		default: 	cout<<"ERROR: "<<pkt.type;
				cout<<"MEM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<endl;
			wait();
		}
	}

}
