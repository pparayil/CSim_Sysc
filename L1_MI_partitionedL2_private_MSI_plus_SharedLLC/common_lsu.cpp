
#include "common_lsu.h"
#include <vector>


bool CM_LSU::set_packet(int src_id, LisNocPacket* packet){
 ////cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<<__LINE__<<endl;
	bool ret = false;
	if(NoCtxQ.empty() == true) {
		ret = false;
	}
	else {
	if(packet == NULL) {
		cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<<__LINE__<<" Error"<<endl; 
		ret = false;
	}//packet = new LisNocPacket;
	else {
	NoCCommPacket pkt = NoCtxQ.front();
	NoCtxQ.pop();	
	//cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<<__LINE__<<endl;
	switch(pkt.type){
		case IDLE: {
					//cout<<"NO PKT"<<endl;
					ret = false;
					}
					break;

		case L2CEEXEVT:{
					packet->read_or_write = pkt.rw;
					packet->burst = 0;
					packet->offset = 0;
					packet->ex = pkt.ex;
					packet->evt = pkt.evt;
					packet->en = pkt.en;
					packet->ack = pkt.ack;
					packet->dst_id = pkt.dstnode;
					packet->size = 2;
					}
					break;
		case L2DATA_TO_LLCMEM:{
					packet->burst = 0;
					packet->evt = false;
					packet->offset = 0;
					packet->en = pkt.en; /* if its to mem then we might need to split it up and make 8 different transactions*/
					packet->read_or_write = pkt.rw;
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
					packet->dst_id = pkt.dstnode;
					}
					break;
		case DIRINV:{
					packet->inv = pkt.inv;
					packet->evt = pkt.evt;
					packet->dst_id = pkt.dstnode;
					packet->size = 2;
					}
					break;
		case DIRACK:{
					packet->ack = pkt.ack;
					packet->evt = false;
					packet->dst_id = pkt.dstnode;
					packet->size = 2;
					}
					break;
		case DIRCE_TO_LLCMEM:{		
					packet->read_or_write = pkt.rw;
					packet->burst = 0;
					packet->evt = false;
					packet->offset = 0;
					packet->inv = false;					
					packet->ex = pkt.ex;
					packet->en = pkt.en;	
					packet->redirectionnode = pkt.redirectionnode;
					packet->dst_id = pkt.dstnode;
					packet->size = 2;
					//cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<<__LINE__<<endl;
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
					packet->dst_id = pkt.dstnode;	
					}
					break;
		case LLCACKDIR:{
					packet->ack = pkt.ack;
					packet->dst_id = pkt.dstnode;
					packet->size = 2;
					}
					break;
		case LLCREQMEM: {
					packet->read_or_write = pkt.rw;
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
		default: cout<<"ERROR: "<<pkt.type; 
				 cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<endl;
		}
	packet->flit_left = packet->size;
	packet->type = pkt.type;
	packet->src_id = pkt.srcnode;
	packet->req_count = pkt.req_count;
	packet->address.push_back(pkt.address);
	//cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" src: "<< pkt.srcnode <<" dst: "<<pkt.dstnode<< " queue size now: " << (int) NoCtxQ.size() <<" type:"<< pkt.type<<endl;
	ret = true;
	}
	}
	//cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<<__LINE__<<" return value:"<<(unsigned int) ret<<endl;
	return ret;
}

void CM_LSU::lsu_tx_method(void ) {
	////cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<<__LINE__<<endl;
	cout<<flush <<dec;
	while(1) {
	LSU_STATE st = IDLE; 
	LSU_STATE first_st =IDLE,second_st =IDLE,third_st = IDLE;
	int i=4; //
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
					else
					if((address_to_llc_en_l2 == true) && (generate_wr_req_npc_l2 == false)){
						st = L2DATA_TO_LLCMEM;
						//cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<" addr:"<<hex<< address_to_llc_l2<<dec<<endl;
                        first_st = L2DATA_TO_LLCMEM;
					} else if ((address_to_llc_en_l2 == true) && (generate_wr_req_npc_l2 == true)) {
						// direct WB from NPC to SC
						st = L2DATA_WB_TO_LLCMEM;
						first_st = L2DATA_WB_TO_LLCMEM;
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
					else if(ce_to_llc_dir == true) {
						
						//cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<endl;
						if (first_st != IDLE) 
							second_st = DIRCE_TO_LLCMEM;
						else 
							{
							first_st = DIRCE_TO_LLCMEM;
							st = DIRCE_TO_LLCMEM;
							}
					}	
					//llc side
					if (ack_to_dir_llc == true) {
						//cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<endl;
						if(first_st == IDLE){
							first_st = LLCACKDIR;
							st = LLCACKDIR;
						}
						else if(second_st == IDLE){
							second_st = LLCACKDIR;
						}
						else
						{
							third_st = LLCACKDIR;
						}
					  
					}
					else if(ack_to_l2_llc == true) {
						//cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<endl;
						if(first_st == IDLE){
							first_st = LLCACKL2;
							st = LLCACKL2;
						}
						else if(second_st == IDLE){
							second_st = LLCACKL2;
						}
						else
						{
							third_st = LLCACKL2;
						}
					}
					else if(addr_en_to_mem_llc == true) {
						//cout<<inode<<": CM_LSU::"<<__func__<<" : "<< __LINE__<<"for address: "<<hex<<addr_to_mem_llc <<dec<<endl;
						if(first_st == IDLE){
							first_st = LLCREQMEM;
							st = LLCREQMEM;
						}
						else if(second_st == IDLE){
							second_st = LLCREQMEM;
						}
						else
						{
							third_st = LLCREQMEM;
						}
					}
				
					if(first_st == IDLE && second_st == IDLE && third_st == IDLE)
					{
					st = IDLE;
					i = 1;
					//next_trigger(1, SC_NS);
					}
					} break;
					// check or find node address and see if that belongs to local node.
		case L2CEEXEVT: {
						//cout<<"CM_LSU::"<<inode<<" @"<<__func__<<" : "<<__LINE__<<endl;
						//decode address to destination node
						unsigned int cl_address = address_to_dir_l2>>(OFFSET_LENGTH);
						unsigned int dst_node = ((unsigned int)cl_address )&( (unsigned int)(NO_OF_NODES-1)); /*0x11 & address will give the dst dir bank address*/
						if(dst_node == inode)
						{
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
							local_dirQ.push(local_l2P);
							
						}
						else
						{
							//make packet and put in the queue
							NoCCommPacket pkt;
							pkt.en = ce_to_dir_l2;
							pkt.ex = ex_to_dir_l2;
							pkt.rw = rd_wr_to_dir_l2;
							pkt.evt = cl_evt_info_to_dir_l2;
							pkt.ack = inv_ack_to_dir_l2;
							pkt.address = address_to_dir_l2;
							pkt.req_count = req_count_to_dir_l2;
							pkt.srcnode = inode;
							pkt.dstnode = dst_node;
							pkt.type = st;
							NoCtxQ.push(pkt);
							
						}
						first_st = IDLE;
						st = second_st;
						if(second_st == IDLE) {//wait();
							
							st = IDLE;
							i = 1;
							//next_trigger(1, SC_NS);
						}					
						}break;
		case L2DATA_WB_TO_LLCMEM: 
		case L2DATA_TO_LLCMEM: {
						//cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<" address:0x"<< hex<<address_to_llc_l2<<dec <<endl;
						//decode address to destination node
						unsigned int cl_address = address_to_llc_l2>>(OFFSET_LENGTH);
						unsigned int dst_node , dnode = ((unsigned int)cl_address )&( (unsigned int)(NO_OF_NODES-1)); 
						#ifdef LLC_TRUE_1
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
						#ifdef LLC_TRUE_0
							dst_node  = dnode;
						#else
							dst_node  = NODE_WITH_SDRAM;
						#endif
						#endif

						//cout<<"dst_node: L2data to LLCmem: "<<dst_node<<" dnode:"<<dnode<<" address:0x"<< hex<<address_to_llc_l2<<dec <<endl;
						NoCCommPacket pkt;
						if(st == L2DATA_TO_LLCMEM) {
						(data_to_llc_l2)->read_data(&pkt.data);
						pkt.address = address_to_llc_l2;
						pkt.req_count = req_count_to_llc_l2;
						pkt.en = address_to_llc_en_l2;
						pkt.rw = true;
						pkt.srcnode = inode;
						pkt.redirectionnode = inode;
						pkt.dstnode = dst_node;
						pkt.type = st;
						} 
						else if (st == L2DATA_WB_TO_LLCMEM){
						pkt.address = address_to_llc_l2;
						pkt.req_count = req_count_to_llc_l2;
						pkt.en = address_to_llc_en_l2;
						pkt.rw = true;
						pkt.ex = true;
						pkt.srcnode = dnode;
						pkt.dstnode = dst_node;
						pkt.redirectionnode = inode;
						pkt.type = DIRCE_TO_LLCMEM;
						}
						if(dst_node == inode)
						{
							#if 1							
							local_llcQ.push(pkt); 
							#else
							cout<<"error !! as l2 and mem cant be in the same node"<<endl;
							#endif
						}						
						else
						{
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
						if(nodeaddr_to_lru_dir == inode)
						{
							NoCCommPacket local_dirP;
							local_dirP.address = inv_addr_to_node_dir;
							local_dirP.req_count = req_count_from_dir_dir;
							local_dirP.srcnode = inode;
							local_dirP.dstnode = nodeaddr_to_lru_dir;
							local_dirP.type = st;
							if((flush_frm_npc_dir == true) && (inv_to_node_dir == false)){
								local_dirP.evt = false;
								local_dirP.inv = true;
							} else if ((flush_frm_npc_dir == false) && (inv_to_node_dir == true)){
								local_dirP.evt = true;
								local_dirP.inv = true;
							}
							local_l2Q.push(local_dirP);		
						}
						else
						{
							//make packet and put in the queue
							NoCCommPacket pkt;
							pkt.address = inv_addr_to_node_dir;
							pkt.req_count = req_count_from_dir_dir;
							pkt.srcnode = inode;
							pkt.dstnode = nodeaddr_to_lru_dir;
							pkt.type = st;
							if((flush_frm_npc_dir == true) && (inv_to_node_dir == false)){
								pkt.evt = false;
								pkt.inv = true;
							} else if ((flush_frm_npc_dir == false) && (inv_to_node_dir == true)){
								pkt.evt = true;
								pkt.inv = true;
							}
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
							//next_trigger(1, SC_NS);
						}
						
					}break;
		case DIRACK: {
						//cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<endl;
						if(nodeaddr_to_lru_dir == inode)
						{
							// local transfer
							NoCCommPacket local_dirP;
							local_dirP.ack = ack_to_node_dir;
							local_dirP.address = address_from_dir_dir;
							local_dirP.req_count = req_count_from_dir_dir;
							local_dirP.srcnode = inode;
							local_dirP.dstnode = nodeaddr_to_lru_dir;
							local_dirP.type = st;
							local_l2Q.push(local_dirP);		
						}
						else
						{
							//make packet and put in the queue
							NoCCommPacket pkt;
							pkt.ack = ack_to_node_dir;
							pkt.req_count = req_count_from_dir_dir;
							pkt.address = address_from_dir_dir;
							pkt.srcnode = inode;
							pkt.dstnode = nodeaddr_to_lru_dir;
							pkt.type = st;
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
							//next_trigger(1, SC_NS);
						}
					}break;
		case DIRCE_TO_LLCMEM: {
						//cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<< "from dir to llc:" <<nodeaddr_to_lru_dir <<dec <<endl;
						NoCCommPacket local_dirP;
						local_dirP.en = true; //ce_to_llc_dir;
						local_dirP.ex = false;
						local_dirP.rw = rd_wr_to_llc_dir;
						local_dirP.req_count = req_count_to_llc_dir;
						local_dirP.address = addr_to_llc_dir;
						local_dirP.srcnode = inode;
						local_dirP.dstnode = nodeaddr_to_lru_dir;
						local_dirP.redirectionnode = dst_nodeaddr_to_llc_dir;
						local_dirP.type = st;
						if(nodeaddr_to_lru_dir == inode)
						{
							// local transfer this can be filled in only once we have LLC or mem added to the lsu							
							
							local_llcQ.push(local_dirP);					
							//cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<< "from dir to llc:" <<nodeaddr_to_lru_dir <<dec <<endl;
						}
						else
						{
							//make packet and put in the queue
							NoCtxQ.push(local_dirP);
							
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
						//cout<<inode<<": for address: "<<addr_to_llc_dir<<"next st is "<<st<<endl;
						if(st == IDLE) {
							
							i = 1;
							//next_trigger(1, SC_NS);
						}
			      
					}break;
		case LLCACKL2: {			
						//cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<" to node "<<nodeaddr_to_lru_llc<<endl;

						if(nodeaddr_to_lru_llc == inode)
						{
							
							NoCCommPacket local_llcP;
							//cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<endl;
							local_llcP.ack = ack_to_l2_llc;
							local_llcP.rw = ack_rw_to_l2_llc;
							local_llcP.req_count = req_count_to_l2_to_llc;
							local_llcP.address = addr_to_l2_to_llc;
							data_from_llc->read_data(&local_llcP.data);
							local_llcP.srcnode = inode;
							local_llcP.dstnode = nodeaddr_to_lru_llc;
							local_llcP.type = st;
							local_l2Q.push(local_llcP);		
						}
						else
						{
							//make packet and put in the queue
							NoCCommPacket pkt;
							pkt.ack = ack_to_l2_llc;
							pkt.rw = ack_rw_to_l2_llc;
							pkt.req_count = req_count_to_l2_to_llc;
							pkt.address = addr_to_l2_to_llc;
							data_from_llc->read_data(&pkt.data);
							pkt.srcnode = inode;
							pkt.dstnode = nodeaddr_to_lru_llc;
							pkt.type = st;
							NoCtxQ.push(pkt);
							//cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<endl;
							
						}
						if ( first_st == st ) {first_st = IDLE;st = second_st;}
						else if(second_st == st){ second_st = IDLE; st = third_st;}
						else { third_st = IDLE; st = IDLE; }
						
						i = 1;
						//next_trigger(1, SC_NS);
					}
					break;
		case LLCACKDIR:{			//cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<endl;
						if(nodeaddr_to_lru_llc == inode)
						{
							NoCCommPacket local_llcP;
							local_llcP.ack = ack_to_dir_llc;
							local_llcP.req_count = req_count_to_dir_to_llc;
							local_llcP.address = addr_to_dir_to_llc;
							local_llcP.srcnode = inode;
							local_llcP.dstnode = nodeaddr_to_lru_llc;
							local_llcP.type = st;
							local_dirQ.push(local_llcP);
							
							
						}
						else
						{
							//make packet and put in the queue
							NoCCommPacket pkt;
							pkt.ack = ack_to_dir_llc;
							pkt.req_count = req_count_to_dir_to_llc;
							pkt.address = addr_to_dir_to_llc;
							pkt.srcnode = inode;
							pkt.dstnode = nodeaddr_to_lru_llc;
							pkt.type = st;
							NoCtxQ.push(pkt);
							
						}
					        if ( first_st == st ) {first_st = IDLE;st = second_st;}
						else if(second_st == st){ second_st = IDLE; st = third_st;}
						else { third_st = IDLE; st = IDLE; }
						if(st == IDLE) {
						i = 1;
						//next_trigger(1, SC_NS);
						}
					}
					break;
		case LLCREQMEM: {		
						//cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<hex<<"address: " <<addr_to_mem_llc<<"from llc:"<<inode<<" to mem:"<<nodeaddr_to_lru_llc <<dec<<endl;
						if(nodeaddr_to_lru_llc == inode)
						{
							#if 0
							NoCCommPacket local_llcP;
							local_llcP.rw = r_w_to_mem_llc;
							local_llcP.req_count = req_count_to_mem_llc;
							local_llcP.address = addr_to_mem_llc;
							local_llcP.en = addr_en_to_mem_llc;
							if(r_w_to_mem_llc == true)
							data_to_mem_llc->read_data(&local_llcP.data);
							local_llcP.srcnode = inode;
							local_llcP.dstnode = nodeaddr_to_lru_llc;
							local_llcP.type = st;
							local_memQ.push(local_llcP);
							#else
							cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<" Error"<<endl;
							#endif
						}
						else
						{
							//make packet and put in the queue
							NoCCommPacket pkt;
							pkt.rw = r_w_to_mem_llc;
							pkt.req_count = req_count_to_mem_llc;
							pkt.address = addr_to_mem_llc;
							pkt.en = addr_en_to_mem_llc;
							if(r_w_to_mem_llc == true)
							data_to_mem_llc->read_data(&pkt.data);
							pkt.srcnode = inode;
							pkt.dstnode = nodeaddr_to_lru_llc;
							pkt.type = st;
							NoCtxQ.push(pkt);
							//cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<endl;
							
						}
						if ( first_st == st ) {first_st = IDLE;st = second_st;}
						else if(second_st == st){ second_st = IDLE; st = third_st;}
						else { third_st = IDLE; st = IDLE; }
						if(st == IDLE) {
						i = 1;
						//next_trigger(1, SC_NS);
						}
					}
					break;
		default: cout<<"ERROR: "<<st; 
			cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<endl;
//			wait();
		
		}
		i--;
	} wait();
	}
					
}

void CM_LSU::lsu_rx_dir(void ){
//cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<<__LINE__<<endl;
while(1){
	while (local_dirQ.empty() == true) wait();
	switch(local_dirQ.front().type) {
	case L2CEEXEVT: {
			//cout<<"CM_LSU::"<<inode<<" @"<<__func__<<" : "<<__LINE__<<endl;
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
				cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<endl;
	}
	local_dirQ.pop();
	}
}
void CM_LSU::lsu_rx_l2(void ){
//cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<<__LINE__<<endl;
while(1){
	while (local_l2Q.empty() == true) wait();
	switch(local_l2Q.front().type) {
	case DIRINV: {
			//cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<< " inv l2:" <<inode<< endl;
			if(local_l2Q.front().evt == true)
				inv_frm_dir_l2 = local_l2Q.front().inv; //inv_to_node_dir;
			else
				flush_frm_npc_dir_l2 = local_l2Q.front().inv; //flush req;
			inv_addr_from_dir_l2 = local_l2Q.front().address; //inv_addr_to_node_dir;
			req_count_from_dir_l2 = local_l2Q.front().req_count;
			//node_address_from_lru_l2 = local_l2Q.front().srcnode;
			wait();
			inv_frm_dir_l2 = false;
			flush_frm_npc_dir_l2 = false;		
			wait();
			}break;
	case DIRACK: {
			//cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<endl;
			ack_from_dir_l2 = local_l2Q.front().ack; //ack_to_node_dir;
			address_from_dir_l2 = local_l2Q.front().address; //address_to_dir_dir;
			req_count_from_dir_l2 = local_l2Q.front().req_count;
			//node_address_from_lru_l2 = local_l2Q.front().srcnode;
			wait();
			ack_from_dir_l2 = false;
			wait();
			}break;
	case LLCACKL2: 
	case MEML2ACK: {
			#if 1
			ack_from_llc_l2 = local_l2Q.front().ack;
			address_from_llc_l2 = local_l2Q.front().address;
			if(local_l2Q.front().rw == false){
			data_from_llc_l2->write_data(&local_l2Q.front().data);		
			}
			req_count_from_llc_l2 = local_l2Q.front().req_count;
			//node_address_from_lru_l2 = local_l2Q.front().srcnode;	
			wait();
			ack_from_llc_l2 = false;	
			wait();
			#else
				cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<" Error"<<endl;
			#endif
			}break;			
	default:cout<<"ERROR: "<<local_l2Q.front().type; 
		cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<endl;
	}
	local_l2Q.pop();
	}	
}
void CM_LSU::lsu_rx_llc(void ){
//cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<<__LINE__<<endl;
while(1){
	while (local_llcQ.empty() == true) wait();
	NoCCommPacket local_P = local_llcQ.front();
	switch(local_P.type) {
	case DIRCE_TO_LLCMEM: {
			rd_wr_llc = local_P.rw;
			if(local_P.ex == true) 
				without_dir_ack_llc = true;
			else 
				without_dir_ack_llc = false;
			addr_from_dir_to_llc = local_P.address;
			req_count_from_dir_to_llc = local_P.req_count;
			dst_nodeaddr_llc = local_P.redirectionnode;
			node_address_from_lru_llc = local_P.srcnode;
			ce_llc = local_P.en;
			wait();
			ce_llc = false;
			wait();
			//cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<< "from dir to llc:" <<nodeaddr_to_lru_dir <<dec <<endl;
			} break;
	case L2DATA_TO_LLCMEM: {	
			// local transfer needs llc / mem	
			req_count_from_l2_to_llc = local_P.req_count;	
			addr_from_l2_to_llc = local_P.address;	
			node_address_from_lru_llc = local_P.srcnode;				
			addr_en_from_l2_to_llc = local_P.en;
			data_to_llc->write_data(&local_P.data);			
			//cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<" for ddress"<<hex<<local_P.address<<", l2 node"<<inode<<dec<<endl;
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
			//cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<hex<<" req_count:"<<local_P.req_count<<" address:"<< local_P.address<<dec<<endl;
		} break;
	default:cout<<"ERROR: "<<local_P.type; 
		cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<endl;
	}
	local_llcQ.pop();
	}
}

void CM_LSU::receive_packet(int src_id, LisNocPacket* packet){
	////cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<<__LINE__<<endl;
	NoCCommPacket pkt;
	pkt.dstnode = packet->dst_id;
	pkt.srcnode = packet->src_id;
	pkt.type = (LSU_STATE)packet->type;
	pkt.req_count = packet->req_count;
	//cout<<"CM_LSU::"<<inode<<" @"<<__func__<<" : "<<__LINE__<<endl;
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
					//cout<<"CM_LSU::"<<inode<<" @"<<__func__<<" : "<<__LINE__<<endl;
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
					//cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<<__LINE__<<endl;
					}
					break;
		case DIRINV:{
					pkt.inv = packet->inv;
					pkt.evt = packet->evt;
					if(packet->address.empty()!=true){
					pkt.address = packet->address.back();
					packet->address.pop_back();
					}
					//cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<<__LINE__<<endl;
					}
					break;
		case DIRACK:{
					pkt.ack = packet->ack ;		
					if(packet->address.empty()!=true){
					pkt.address = packet->address.back();
					packet->address.pop_back();
					}
					//cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<<__LINE__<<endl;

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
					//cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<<__LINE__<<endl;			
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
					cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<<__LINE__<<" Error"<<endl;
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
					////cout<<"CM_LSU:: data[]: "<< pkt.data.data[i-1]<<" and from noxim side data[]: "<<packet->data.back()<<endl;
					packet->data.pop_back();					
					} 
					}
					pkt.ack = packet->ack;	
					#else
					cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<<__LINE__<<" Error"<<endl;
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
					#if 0
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
					#else
					cout<<"Error !! CM_LSU::"<<__func__<<" : "<< __LINE__<<endl;
					#endif
					}
					break;
		case MEMLLCACK:{		//cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<endl;
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
			  cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<endl;
		}
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
		switch(pkt.type) {				
		case L2CEEXEVT: {
						//cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<" Dir received l2 req from "<<pkt.srcnode<<endl;
						NoCCommPacket local_l2P;
						local_l2P.ack = pkt.ack;
						local_l2P.srcnode = pkt.srcnode;
						local_l2P.dstnode = pkt.dstnode;
						local_l2P.address = pkt.address;
						local_l2P.req_count = pkt.req_count;
						local_l2P.type = pkt.type;
						local_l2P.evt = pkt.evt;
						local_l2P.ex = pkt.ex;
						local_l2P.rw = pkt.rw;
						local_l2P.en = pkt.en ;
						local_dirQ.push(local_l2P);
						}
						break;
		case L2DATA_TO_LLCMEM: {
						//cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<" address:"<<hex<<pkt.address<<" ,l2:"<<pkt.srcnode<<dec<<endl;
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
						//cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<"INV from "<<pkt.srcnode<<endl;	
						NoCCommPacket local_dirP;
						local_dirP.inv = pkt.inv;
						local_dirP.evt = pkt.evt;
						local_dirP.address = pkt.address;
						local_dirP.req_count = pkt.req_count;
						local_dirP.srcnode = pkt.srcnode;
						local_dirP.dstnode = pkt.dstnode;
						local_dirP.type = pkt.type;
						local_l2Q.push(local_dirP);	
						}break;
		case DIRACK: {
						//cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<" from "<<pkt.srcnode<<endl;
						NoCCommPacket local_dirP;
						local_dirP.ack = pkt.ack;
						local_dirP.address = pkt.address;
						local_dirP.req_count = pkt.req_count;
						local_dirP.srcnode = pkt.srcnode;
						local_dirP.dstnode = pkt.dstnode;
						local_dirP.type = pkt.type;
						local_l2Q.push(local_dirP);							
						}break;
		case DIRCE_TO_LLCMEM: {
						//cout<<"dir to llc ";
						//cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<endl;	
						NoCCommPacket local_dirP;
						local_dirP.rw = pkt.rw;
						local_dirP.en = pkt.en;
						local_dirP.ex = pkt.ex;
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
						//cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<" from "<<pkt.srcnode<<endl;
						NoCCommPacket local_memP;
						local_memP.ack = pkt.ack;
						local_memP.srcnode = pkt.srcnode;
						local_memP.dstnode = pkt.dstnode;
						local_memP.address = pkt.address;
						local_memP.req_count = pkt.req_count;
						local_memP.type = pkt.type;
						local_dirQ.push(local_memP);
						#else
						cout<<"Error !! CM_LSU::"<<__func__<<" : "<< __LINE__<<" from "<<pkt.srcnode<<endl;
						#endif
											
						}
						break;
		case MEML2ACK: {		
						#if 0
						//cout<<"@"<<inode<<" CM_LSU:: receive_packet:: data[7]: "<< pkt.data.data[7]<<" @ pkt.req_count: "<< pkt.req_count <<endl;
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
						cout<<"Error !! @"<<inode<<" CM_LSU:: receive_packet:: data[7]: "<< pkt.data.data[7]<<" @ address: "<< pkt.address <<endl;
						#endif
						}break;
		case LLCACKL2: {			
						//cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<" llcackl2 - req:count: "<< pkt.req_count <<endl;
						NoCCommPacket local_memP;
						local_memP.ack = pkt.ack;
						local_memP.srcnode = pkt.srcnode;
						local_memP.dstnode = pkt.dstnode;
						local_memP.address = pkt.address;
						local_memP.req_count = pkt.req_count;
						local_memP.type = pkt.type;
						local_memP.data = pkt.data;
						local_l2Q.push(local_memP);
						}
						break;
		case LLCACKDIR:{			
						//cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<" llcackdir - req:count: "<< pkt.req_count <<endl;
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
						//cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<" memack - req:count: "<< pkt.req_count <<endl;
						NoCCommPacket local_memP;
						local_memP.ack = pkt.ack;
						local_memP.rw = pkt.rw;
						local_memP.srcnode = pkt.srcnode;
						local_memP.dstnode = pkt.dstnode;
						local_memP.address = pkt.address;
						local_memP.req_count = pkt.req_count;
						local_memP.type = pkt.type;
						local_memP.data = pkt.data;
						local_llcQ.push(local_memP);
						}
						break;
				
		default: 	cout<<"ERROR: "<<pkt.type;
				cout<<"CM_LSU::"<<inode<<" , "<<__func__<<" : "<< __LINE__<<endl;
			wait();
		}
	}

}
