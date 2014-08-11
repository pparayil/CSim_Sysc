
#include "Directory.h"
void DIR::dir_init(void)
{
	//cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<< " dir_init" << endl;
 	DirMem = new unsigned int[TOTAL_CL_IN_DIR];
	memset(DirMem,0x0,TOTAL_CL_IN_DIR); /*[cl_status|DCC_STR_VALID|DCC_REPLACER|FWD_VALID|FORWARDER_NODE|state]*/
}

void DIR::WR_RD_CL_FRM_MEML3 (int address, int nodeaddress, int req_count, bool we) {
	unsigned int cl_index = (address)>>(CL_SIZE +TOTAL_NO_OF_NODES_CARRYING_DIR_2);
	rd_wr_to_l3 = we;
	forward_req_from_dir = false;
	ce_to_l3 = true;
	addr_to_l3 = address;
	req_count_to_l3 = req_count;
	dst_nodeaddr_to_l3 = nodeaddress;
	/* dir to l3 mapping: cache line is shared between 2 nodes */
	#ifdef L3_TRUE_1
	//cout<<flush;
	//cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" : addr: "<<hex<<address<<", cl_index:"<< cl_index<<dec<<endl;
	if((cl_index & 0x01) == 0x0) // 7 = 5(offset) + 2(inode)
	{
	nodeaddr_to_lru = inode;
	//cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" : l31 nodeaddr: "<<hex<<inode<<dec<<endl;
	}
	else
	{
		if((inode & 0x01) == 0x0)
		{
			nodeaddr_to_lru = inode+1;
			//cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" : l32 nodeaddr: "<<hex<<inode+1<<dec<<endl;
		}
		else
		{
			nodeaddr_to_lru = inode-1;
			//cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" : l33 nodeaddr: "<<hex<<inode-1<<dec<<endl;
		}
	}
	#else
	#ifdef L3_TRUE_0
		nodeaddr_to_lru  = inode;
	#else
	nodeaddr_to_lru = NODE_WITH_SDRAM;
	#endif
	#endif
	wait();
	//cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" : l3nodeaddr: "<<hex<<nodeaddr_to_lru<<dec<<endl;	
	rd_wr_to_l3 = false;
	ce_to_l3 = false;
	wait();
}

void DIR::Invcmd_to_caches (unsigned int address,unsigned int req_count, unsigned int nodeaddress, bool spill_inv){
	inv_to_node = true;
	spill_inv_to_node = spill_inv;
	inv_addr_to_node = address;
	req_count_from_dir = req_count;
	nodeaddr_to_lru = nodeaddress;
	wait();
	inv_to_node = false;

}

void DIR::ACKcmd_to_caches (unsigned int address,unsigned int req_count, unsigned int nodeaddress){
	ack_to_node =  true;
	req_count_from_dir = req_count;
	address_from_dir = address;
	nodeaddr_to_lru = nodeaddress;
	wait();
	ack_to_node =  false;
}


void DIR::WR_RD_CL_FRM_NPC (int address, int nodeaddress, int req_count, int forwarder, unsigned int type) {
	unsigned int cl_index = (address)>>(CL_SIZE +TOTAL_NO_OF_NODES_CARRYING_DIR_2);		
	switch (type){
	case 0x0:
	case 0x2:{
			replace_req_dcc_ack_from_dir = true;
			dst_nodeaddr_to_l3 = forwarder;
			nodeaddr_to_lru = nodeaddress;
			if(type == 0x2)
				dcc_transfer_en_to_npc = false;
			else
				dcc_transfer_en_to_npc = true;
			cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" dcc ack transfer fwrdr:"<<forwarder<<endl;
		}break;
	case 0x1:{
			forward_req_from_dir = true;
			nodeaddr_to_lru = forwarder;
			dst_nodeaddr_to_l3 = nodeaddress;
			cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" fwd "<<endl;
	} break;
	case 0x3:{
			forward_spillreq_to_npc = true;
			forward_req_from_dir = true;
			nodeaddr_to_lru = forwarder;
			dst_nodeaddr_to_l3 = nodeaddress;
			cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" spill rd "<<endl;
	} break;
	default:
		cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" Error"<<endl;
	
	}
	rd_wr_to_l3 = false;
	addr_to_l3 = address;
	req_count_to_l3 = req_count;
	wait();
	//cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" : l3nodeaddr: "<<hex<<nodeaddr_to_lru<<dec<<endl;	
	rd_wr_to_l3 = false;
	forward_req_from_dir = false;
	replace_req_dcc_ack_from_dir = false;
	forward_spillreq_to_npc = false;
	wait();
}
void DIR::DCC_READ_n_INVALIDATE(unsigned int address,unsigned int req_count, unsigned int nodeaddress) {
	unsigned int cl_index = (address)>>(CL_SIZE +TOTAL_NO_OF_NODES_CARRYING_DIR_2);	
	unsigned int dcc_node = (DirMem[cl_index]>>(DCC_NODE_BASE)) & (NO_OF_NODES-1);
	unsigned int clholders_plus_dcc = DirMem[cl_index]>>(DCC_NODE_BASE);
	WR_RD_CL_FRM_NPC(address, nodeaddress,req_count,dcc_node, 0x3);	/*SPILL_RD*/
	DirMem[cl_index] = 0x0;		
	DirMem[cl_index] = DirMem[cl_index] | (clholders_plus_dcc << (DCC_NODE_BASE));			
	DirMem[cl_index] = DirMem[cl_index] | ((unsigned int) nodeaddress << FORWARDER_BASE);
	DirMem[cl_index] = DirMem[cl_index] | ((unsigned int) 0x1 <<(CL_STATUS_STORAGE_BASE+nodeaddress));	
	DirMem[cl_index] = (DirMem[cl_index] & 0xfffffff0) | I_to_M_DCC;
	DirMem[cl_index] = DirMem[cl_index] & ~(((unsigned int)0x1)<< ((DCC_NODE_EN_BASE))); // deactivate dcc mode /* since there is another reader for dcc, disable dcc here */
}

void DIR::DCC_INVALIDATE(unsigned int address,unsigned int req_count) {
	unsigned int cl_index = (address)>>(CL_SIZE +TOTAL_NO_OF_NODES_CARRYING_DIR_2);	
	unsigned int dcc_node = (DirMem[cl_index]>>(DCC_NODE_BASE)) & (NO_OF_NODES-1);
	unsigned int clholders = DirMem[cl_index]>>(CL_STATUS_STORAGE_BASE);
	Invcmd_to_caches(address, req_count, dcc_node, true);											
	DirMem[cl_index] = 0x0;		
	DirMem[cl_index] = DirMem[cl_index] | (clholders << (DCC_NODE_BASE));
	DirMem[cl_index] = (DirMem[cl_index] & 0xfffffff0) | M_to_M_DCC;
	DirMem[cl_index] = DirMem[cl_index] & ~(((unsigned int)0x1)<< ((DCC_NODE_EN_BASE))); // deactivate dcc mode /* since there is another reader for dcc, disable dcc here */
}
bool DIR::refill_cl_to_npc (int address, int nodeaddress, int req_count) 
{
	unsigned int cl_index = (address)>>(CL_SIZE +TOTAL_NO_OF_NODES_CARRYING_DIR_2);
	char state;
	char holders;
	bool ret = true;
	//cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<endl;
	state = DirMem[cl_index] & 0x0f;
	if((state == MODIFIED) && (((DirMem[cl_index]>>(nodeaddress+CL_STATUS_STORAGE_BASE)) & 0x1) == false) && (((DirMem[cl_index]>>(DCC_NODE_EN_BASE)) & 0x1) == false))
	{
		//send flush req to npc , wait for inv_ack, set state SHARED
		//then RD request
		flush_frm_npc = true; 
		req_count_from_dir = req_count;
		inv_addr_to_node = address;
		dst_nodeaddr_to_l3 = nodeaddress;
		int i;
		for(i=(CL_STATUS_STORAGE_BASE); i<(NO_OF_NODES+CL_STATUS_STORAGE_BASE); i++) {
		if(((DirMem[cl_index]>>i) & 0x1) == true) {
			nodeaddr_to_lru = i-(CL_STATUS_STORAGE_BASE);
			break;
		}
		}
		wait();
		flush_frm_npc = false; 
		ReqFormat_dir Bl_req;
		Bl_req.we = false;
		Bl_req.address = address;
		Bl_req.req_count = req_count;
		Bl_req.evict = false;
		Bl_req.type = RD_D;
		Bl_req.SrcNode = nodeaddress;
		Bl_req.DstNode = nodeaddress;
		Bl_req.inv_ack_count = 0x1; // this inv_count is not reduced 
		Bl_req.sc_ack_count = 0x0; // will be set later
		BlockedQ.push(Bl_req);
		DirMem[cl_index] = (DirMem[cl_index] & 0xfffffff0) | M_to_S; // M to S	
		cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" FLUSH to mem send to "<<(i-(CL_STATUS_STORAGE_BASE))<<endl;		
		ret = true;
	}	
	else if (state < MODIFIED) {
		//send RD req. receive ack from l3/mem then set state to SHARED/MODIFIED		
		if((state == SHARED) && ((DirMem[cl_index]>>(FORWARDER_EN_BASE) & 0x1) == true)){
			unsigned int forwarder = (DirMem[cl_index]>>FORWARDER_BASE) & (NO_OF_NODES-1);
			unsigned int clholders_plus_dcc = DirMem[cl_index]>>(DCC_NODE_BASE);
			if(nodeaddress != forwarder) {
			WR_RD_CL_FRM_NPC(address, nodeaddress,req_count,forwarder, 0x1); /*FWD_RD*/
			DirMem[cl_index] = 0x0;
			DirMem[cl_index] = DirMem[cl_index] | (clholders_plus_dcc << (DCC_NODE_BASE));
			DirMem[cl_index] = DirMem[cl_index] | ((unsigned int) nodeaddress << FORWARDER_BASE);
			DirMem[cl_index] = DirMem[cl_index] | ((unsigned int) 0x1 <<(CL_STATUS_STORAGE_BASE+nodeaddress));	
			DirMem[cl_index] = (DirMem[cl_index] & 0xfffffff0) | S_to_S; // S to S jus to differentiate between rd from mem and read from npc
			} else {
			cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" Error!! FWDer == requester"<<endl;
			}
		} else if((DirMem[cl_index]>>(DCC_NODE_EN_BASE) & 0x1) == true){
			// DCC read 
			unsigned int dcc_node = (DirMem[cl_index]>>(DCC_NODE_BASE)) & (NO_OF_NODES-1);
			unsigned int clholders_plus_dcc = DirMem[cl_index]>>(DCC_NODE_BASE);
			WR_RD_CL_FRM_NPC(address, nodeaddress,req_count,dcc_node, 0x3);	/*SPILL_RD*/
			DirMem[cl_index] = 0x0;		
			DirMem[cl_index] = DirMem[cl_index] | (clholders_plus_dcc << (DCC_NODE_BASE));			
			DirMem[cl_index] = DirMem[cl_index] | ((unsigned int) nodeaddress << FORWARDER_BASE);
			DirMem[cl_index] = DirMem[cl_index] | ((unsigned int) 0x1 <<(CL_STATUS_STORAGE_BASE+nodeaddress));	
			DirMem[cl_index] = DirMem[cl_index] & ~(((unsigned int)0x1)<< ((DCC_NODE_EN_BASE))); // deactivate dcc mode /* since there is another reader for dcc, disable dcc here */
			DirMem[cl_index] = (DirMem[cl_index] & 0xfffffff0) | I_to_S_DCC; 
		}
		else {
			WR_RD_CL_FRM_MEML3(address, nodeaddress, req_count, false);
			ReqFormat_dir Bl_req;
			Bl_req.we = false;
			Bl_req.address = address;
			Bl_req.req_count = req_count;
			Bl_req.evict = false;
			Bl_req.SrcNode = nodeaddress;
			Bl_req.DstNode = nodeaddress;
			Bl_req.inv_ack_count = 0x0; // this inv_count is not reduced 
			Bl_req.sc_ack_count = 0x1; // will be set later
			Bl_req.type = RD_D;
			if(state == INVALID)
				DirMem[cl_index] = 0x0;
			DirMem[cl_index] = (DirMem[cl_index] & 0xfffffff0) | I_to_S; // I to S
			BlockedQ.push(Bl_req);
		}			
		ret = true;
	}
	else if (state > MODIFIED){
		// wait put the req back to the queue as trans state
		ret = false;
	} else {
		cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" Error!!"<<endl;
		ret = true;
	}
	return ret;
}
unsigned int DIR::count_one(unsigned int x) {
	// for 32 bit unsigned int size
	unsigned int count = 0x0;
	for(unsigned int i =0; i< 32; i++) {
		if(((x>>i) & 0x1) == true)
			count= count+0x1;
	}
	return count;
}

bool DIR::write_to_l3 ( int address, int nodeaddress,  bool evict, int req_count)
{
	unsigned int cl_index = (address)>>(CL_SIZE +TOTAL_NO_OF_NODES_CARRYING_DIR_2);
	char state;
	char holders;
	bool ret =true;
	state = DirMem[cl_index] & 0x0f;
	
	if(state == MODIFIED) {
		cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<endl;
		//WR request to mem/l3 
		//ack received, if evict then evict node
		if(evict == true) {			
			WR_RD_CL_FRM_MEML3(address, nodeaddress, req_count, true);
			DirMem[cl_index] = (DirMem[cl_index] & 0xfffffff0) | (M_I_WB);  // state M to I			
		} else {
			if((nodeaddress != inode) && (inode != NODE_WITH_SDRAM)) {
				WR_RD_CL_FRM_NPC(address, nodeaddress,req_count,((nodeaddress+1)%(NO_OF_NODES-1)),0x0);
				cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<endl;
			} else{
				WR_RD_CL_FRM_NPC(address, nodeaddress,req_count,((nodeaddress+1)%(NO_OF_NODES-1)),0x0); 				
				cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<endl;
			}				
			DirMem[cl_index] = (DirMem[cl_index] & 0xfffffff0) | M_I_DCC; // state M to I
		}	
		ret = true;
	}
	else if (state == INVALID){
		//state 0x00, wrong error. 		
		if(evict == true){
			if((DirMem[cl_index]>>(DCC_NODE_EN_BASE) & 0x1) == true) {// dcc enabled ? 
				WR_RD_CL_FRM_MEML3(address, nodeaddress, req_count, true);
				DirMem[cl_index] = (DirMem[cl_index] & 0xfffffff0) | (M_I_WB);  // state M to I
			} else 
				DirMem[cl_index] = 0x0; // removing the node
		} else {
			cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" Error: Wrong state WB called from nodes which are at state:0"<< endl;
		}
		ret = true;
	} 
	else if (state == SHARED){
		//state 0x01, send invs , wait for all acks, then sent WR to mem, ack received, if evict then evict node 
		if(evict == true){
			DirMem[cl_index] = DirMem[cl_index] & ~(((unsigned int)0x1)<< (CL_STATUS_STORAGE_BASE+nodeaddress)); // removing the node
			unsigned int forwarder = (DirMem[cl_index]>>FORWARDER_BASE) & (NO_OF_NODES-1);
			if((forwarder == nodeaddress) && (((DirMem[cl_index]>>(FORWARDER_EN_BASE)) & 0x1) == true))
				DirMem[cl_index] = DirMem[cl_index] & ~(((unsigned int)0x1)<< ((FORWARDER_EN_BASE))); // deactivate forwarder mode
			cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" Error: Wrong state WB called from nodes which are at state:1"<< endl;
		} else {
			/*if multiple sharers ack with no dcc node value and some indication, like dcc-en + dcc node */
			unsigned int clholders = DirMem[cl_index]>>(CL_STATUS_STORAGE_BASE);
			/*See if more than one, holders are thr*/
			if(count_one(clholders) == 0x1) {
				if((nodeaddress != inode) && (inode != NODE_WITH_SDRAM)) {
					WR_RD_CL_FRM_NPC(address, nodeaddress,req_count,((nodeaddress+1)%(NO_OF_NODES-1)),0x0);
				} else{
					WR_RD_CL_FRM_NPC(address, nodeaddress,req_count,((nodeaddress+1)%(NO_OF_NODES-1)),0x0); 					
				}				
				DirMem[cl_index] = (DirMem[cl_index] & 0xfffffff0) | S_I_DCC; // 
			} else {
				WR_RD_CL_FRM_NPC(address, nodeaddress,req_count,0x0,0x2); // not singlet
				unsigned int forwarder = (DirMem[cl_index]>>FORWARDER_BASE) & (NO_OF_NODES-1);
				if((forwarder == nodeaddress) && (((DirMem[cl_index]>>(FORWARDER_EN_BASE)) & 0x1) == true))
					DirMem[cl_index] = DirMem[cl_index] & ~(((unsigned int)0x1)<< ((FORWARDER_EN_BASE))); // deactivate forwarder mode
				DirMem[cl_index] = DirMem[cl_index] & ~(((unsigned int)0x1)<< (CL_STATUS_STORAGE_BASE+nodeaddress)); // removing the node
			}
		}
		ret = true;
	}
	else if (state > MODIFIED){
		// wait put the req back to the queue as trans state
		ret = false; 
	}
	return ret;
}

bool DIR::remove_all_other_reqs_for_address(unsigned int nodeid, unsigned int req_count,unsigned int address){
	if(ReqQ.empty() == true) 
		return true;
	else {
		ReqFormat_dir front;
		ReqFormat_dir back = ReqQ.back();
		do{
			front = ReqQ.front();
			if(((address >> CL_SIZE) == (front.address >> CL_SIZE)) && (((front.req_count >> (MAX_PROCESSOR_COUNT_WIDTH)) == nodeid) && ((front.req_count >> (MAX_PROCESSOR_COUNT_WIDTH)) != (req_count >> (MAX_PROCESSOR_COUNT_WIDTH))))) {
				ReqQ.pop(); // request will be resent from the nodes anyways
				cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" removing req : "<<front.req_count<<endl;
			} else {
				ReqQ.pop();
				ReqQ.push(front);
			}
		} while(front != back);
		
	}
	return true;
}



bool DIR::Invalidation_fn(int address, int nodeaddress, int req_count, bool we, bool evict, unsigned int type){
	unsigned int cl_index = (address)>>(CL_SIZE +TOTAL_NO_OF_NODES_CARRYING_DIR_2);
	unsigned int state;
	bool ret = true;
	bool blockedreq = false;	
	unsigned int cl_holders = ((DirMem[cl_index]) >>(CL_STATUS_STORAGE_BASE));
	unsigned int inv_counter;
	ret = true;
	blockedreq = false;	
	state = DirMem[cl_index] & 0x0f;
	cout<<flush;
	//cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<hex<<" address"<<address<<" state:"<<(unsigned int)state<<dec<<endl;
	if ((state == INVALID)) {
		// nothing to be done
		inv_counter = 0x0;
		if((DirMem[cl_index]>>(DCC_NODE_EN_BASE) & 0x1) == true){
			if(type == RDX_D) {
				DCC_READ_n_INVALIDATE(address, req_count, nodeaddress);
			} else {
				DCC_INVALIDATE(address, req_count);
			}
			blockedreq = false;
		} else {
			if(type == RDX_D) {			
				DirMem[cl_index] = 0x0;			
				DirMem[cl_index] = (DirMem[cl_index] | ((unsigned int) 0x1 << ((CL_STATUS_STORAGE_BASE)+nodeaddress))) | I_to_M;		//I to M
				blockedreq = true;
			} else {
				cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" Error!"<<endl;
				DirMem[cl_index] = 0x0;
				//DirMem[cl_index] = (DirMem[cl_index] | ((unsigned int) 0x1 << ((CL_STATUS_STORAGE_BASE)+nodeaddress))) | (MODIFIED) ;		//M
				blockedreq = false;
			}
		}
		ACKcmd_to_caches(address,req_count,nodeaddress);
		cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" 3"<<endl;
		ret = true;
	} 
	else if((state == SHARED) || (state == MODIFIED)) {
		inv_counter = 0x0;
		
		if((((cl_holders>>nodeaddress)& 0x1) == 0x1) && ((type == RDX_D))){
			cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" Error! RDX address:"<<hex<<address<<dec<<" nodeid:"<<nodeaddress<<endl;
		}
		else if((((cl_holders>>nodeaddress)& 0x1) == 0x0) && (type == INV_D)){
			cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" Error! Exclusion,  address:"<<hex<<address<<dec<<" nodeid:"<<nodeaddress<<endl;
		} 
		else {
		for (unsigned int i=0; i<TOTAL_NO_OF_NODES_CARRYING_DIR; i++)
		{
			if(nodeaddress == i) 
			{
			//cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" src not holder ["<<hex<<((cl_index<<7) | (inode<<5))<<"]: "<< DirMem[cl_index]<<endl;
				if(((cl_holders>>i)& 0x1) == 0x1) {
					if(type == RDX_D){
						cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" Error! RDX address:"<<hex<<address<<dec<<" nodeid:"<<nodeaddress<<endl;
					/*	if (state == MODIFIED)
							DirMem[cl_index] = ((DirMem[cl_index]) & 0xFFFFFFF0) | M_to_M;		// M to M
						else
							DirMem[cl_index] = ((DirMem[cl_index]) & 0xFFFFFFF0) | S_to_I;		// S to I and I to M	*/
					} else {
						DirMem[cl_index] = ((DirMem[cl_index]) & 0xFFFFFFF0) | S_to_M;		//S to M	
					}
				} else {
					if(type == INV_D){
						cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" Error! Exclusion,  address:"<<hex<<address<<dec<<" nodeid:"<<nodeaddress<<endl;
						//DirMem[cl_index] = ((DirMem[cl_index]) & 0xFFFFFFF0) | S_to_M;		//S to M	
					} else {
						if (state == MODIFIED) {
							DirMem[cl_index] = ((DirMem[cl_index]) & 0xFFFFFFF0) | M_to_M;		// M to M
							cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" M to M address:"<<hex<<address<<dec<<endl;
							}
						else
							DirMem[cl_index] = ((DirMem[cl_index]) & 0xFFFFFFF0) | S_to_I;		// S to I and I to M	
					}				
				}
				continue;	
			}
			else if(((cl_holders>>i)& 0x1) == 0x1) /* if(ReQ.SrcNode != NODE_WITH_SDRAM)*/
			{
			cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" invalidate node"<<i<<endl;			
			inv_counter = inv_counter + 0x1;
			Invcmd_to_caches(address, req_count, i, false);			
			remove_all_other_reqs_for_address((unsigned int)i,req_count, address);
			} else {
			continue;
			}			
		}
		 		
		if((inv_counter == 0x0) && ((type == RDX_D) ||(type == INV_D))) { 
		// no invalidation to complete
			ACKcmd_to_caches(address,req_count,nodeaddress);
			if((DirMem[cl_index]>>(DCC_NODE_EN_BASE) & 0x1) == true){
				if(type == RDX_D) {
					DCC_READ_n_INVALIDATE(address, req_count, nodeaddress);
				} else {
					DCC_INVALIDATE(address, req_count);
				}
				blockedreq = false;
			} else {
				if(type == RDX_D) {				 
					DirMem[cl_index] = 0x0;
					DirMem[cl_index] = (DirMem[cl_index] | ((unsigned int) 0x1 << ((CL_STATUS_STORAGE_BASE)+nodeaddress))) | (I_to_M) ;		//I to M
					blockedreq = true;
									
				} else {
					DirMem[cl_index] = 0x0;
					DirMem[cl_index] = (DirMem[cl_index] | ((unsigned int) 0x1 << ((CL_STATUS_STORAGE_BASE)+nodeaddress))) | (MODIFIED) ;		//M
					blockedreq = false;
				}
			}
			cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" 1"<<endl;
			cout<<flush;
		} else if((inv_counter > 0x0) && ((type == RDX_D) ||(type == INV_D))) {
			if((DirMem[cl_index]>>(DCC_NODE_EN_BASE) & 0x1) == true){
				DCC_INVALIDATE(address, req_count);
				cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" Error"<<endl;
				blockedreq = true;
			} else //start a blocked req with inv count set
				blockedreq = true;
			cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" 2"<<endl;
		} else {
			cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" Error"<<endl;
		}
		}		 		
		ret = true; 
	}	
	else  { 
	// trans state let the state come back to a defined state and then proceed with the INV 
		ret = false;
		blockedreq = false;
		//cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" T state:address 0x"<<hex<<address<<dec<<endl;
	} 
	if(blockedreq == true) {
		ReqFormat_dir Bl_req;
		Bl_req.inv_ack_count = inv_counter;
		if((inv_counter == 0x0) && (type == RDX_D)){
			WR_RD_CL_FRM_MEML3(address, nodeaddress, req_count, false);
			Bl_req.sc_ack_count = 0x1;
		} else {
			Bl_req.sc_ack_count = 0x0;
		}
		Bl_req.we = we;
		Bl_req.address = address;
		Bl_req.req_count = req_count;
		Bl_req.evict = evict;
		Bl_req.type = (ReQType_Dir) type;
		Bl_req.SrcNode = nodeaddress;
		Bl_req.DstNode = nodeaddress;		
		BlockedQ.push(Bl_req);
	}
	return ret;
}

void DIR::request_collector(void)
{
//cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<endl;
/* TODO: check if rdx or wrx is causing any req miss */
	enum state {IDLE, INV, RD_WR, RD_X, ACK_ANY} st;
	st = IDLE; 
	state st1 = IDLE;
	while (1)
	{
		switch (st) {
		case IDLE: {			
				if((inv_ack_to_dir == true)  || ( ack_from_l3 == true)) {
					st1 = ACK_ANY;
				} else {
					st1 = IDLE;
				}				
				if ((ex_to_dir == true) && (ce_to_dir == false ) && (cl_evt_info_to_dir == false))
				{
					st = INV;//cache from S to M
					cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<"Invalidating other sharers!! + changing state to MODIFIED" << endl;
				}
				else if(((ex_to_dir == false) && (ce_to_dir == true)))
				{
					//cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<"rw/ wevict, but no exclusive!!"<<endl;
					st =  RD_WR; 
				} 				
				else if ((ex_to_dir == true) && (ce_to_dir == true))
				{
					if(rd_wr_to_dir == false) {
						st = RD_X;
					} else
						cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" Error!"<<endl;
					
					cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" RDX: from "<<node_address_from_lru <<endl;
				}
				else if ((cl_evt_info_to_dir == true) && (ce_to_dir == false))
				{
					ReqFormat_dir ReQ;
					ReQ.evict = true;
					ReQ.we = false;
					ReQ.address = address_to_dir;
					ReQ.req_count = req_count_to_dir;
					ReQ.SrcNode = node_address_from_lru; //0; //hack as only single npc and npc lru is not implemented
					ReQ.type = EVT_D;
					cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" EVT address:"<<address_to_dir<<endl;
					ReqQ.push(ReQ);					
					st = st1;
					if(st1 == IDLE)
						wait();
				}
				else
				{
					st = st1;
					if(st1 == IDLE)
						wait(); 
				}
				
			} break;
		case INV: {
				ReqFormat_dir ReQ2;
				ReQ2.address = address_to_dir;
				ReQ2.req_count = req_count_to_dir;
				ReQ2.SrcNode = node_address_from_lru; //0; //hack as only single npc and npc lru is not implemented
				ReQ2.type = INV_D;
				ReQ2.evict = false;
				ReqQ.push(ReQ2);
				cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" INV from "<<node_address_from_lru <<endl;
				st = st1;
				if(st1 == IDLE)
					wait();				
				} break;
		case RD_WR: {
				ReqFormat_dir ReQ;
				ReQ.address = address_to_dir;
				ReQ.req_count = req_count_to_dir;
				ReQ.we = rd_wr_to_dir;
				if(rd_wr_to_dir == true)
				{
					ReQ.type = WR_D;
				}
				else
					ReQ.type = RD_D;
				ReQ.evict = cl_evt_info_to_dir;
				ReQ.SrcNode = node_address_from_lru; //0; //hack as only single npc and npc lru is not implemented
				ReqQ.push(ReQ);
				cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" RDWR from "<<node_address_from_lru <<endl;
				st = st1;
				if(st1 == IDLE){
					wait();	
				}
			} break;
		case RD_X:{ // Dir from I to M or S to M 
				ReqFormat_dir ReQ1;
				ReQ1.address = address_to_dir;
				ReQ1.req_count = req_count_to_dir;
				ReQ1.SrcNode = node_address_from_lru; //0; //hack as only single npc and npc lru is not implemented
				ReQ1.type = RDX_D;
				ReQ1.evict = false;
				ReQ1.we = false ; //r_w_temp;
				ReqQ.push(ReQ1);				
				cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" INVRDWR from "<<ReQ1.SrcNode <<endl;
				st = st1;
				if(st1 == IDLE)
					wait();	
				} break;
		case ACK_ANY: {
				ReqFormat_dir ReQ3;
				if ( ack_from_l3 == true) {
					ReQ3.type = ACK_SC;
					ReQ3.address = addr_from_l3;
					ReQ3.req_count = req_count_from_l3;
					ReQ3.SrcNode = node_address_from_lru;
				}
				if(inv_ack_to_dir == true) {
					ReQ3.type = ACK_NPC;
					ReQ3.address = address_to_dir;
					ReQ3.req_count = req_count_to_dir;
					ReQ3.SrcNode = node_address_from_lru;
				}
				ReQ3.we = false;
				ReqQ.push(ReQ3);
				//cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" INV from "<<node_address_from_lru <<endl;
				st = IDLE;	
				st1 = IDLE;
				wait();
				}break;
		default: cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__ << "Error!!" <<endl;
			wait();
		}
	}
}

void DIR::dir_main_thread(void)
{	
//cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<endl;
bool req_completed;
int state;
while (1) {
	ReqFormat_dir ReQ;	
	while (ReqQ.empty() == true) wait(); 
	//cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<endl;
	ReQ = ReqQ.front();
	//remove request
	ReqQ.pop();
	unsigned int cl_index = (ReQ.address)>>(CL_SIZE +TOTAL_NO_OF_NODES_CARRYING_DIR_2); /* 32 bit    0xffff 0b 1111 1111 1xx0 0000,   xx = inode , for 2 node bit case, */
	state = DirMem[cl_index] & 0x0f;
	switch (ReQ.type) {
		case RD_D:{
			//cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" RD_D"<<endl;
			if (false == refill_cl_to_npc(ReQ.address,ReQ.SrcNode,ReQ.req_count)) {
				ReqQ.push(ReQ);
				//cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<"RD_D in trans state"<<endl;
			}
			} break;
		case WR_D:{					
			//cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" WR_D"<<endl;
			if (false == write_to_l3(ReQ.address,ReQ.SrcNode,ReQ.evict,ReQ.req_count)){
				ReqQ.push(ReQ);
				//cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" WR_D in trans state"<<endl;
			}
			} break;		
		case RDX_D: 
		case INV_D: {			
			//cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" INV_D"<<endl;	
			if(false == (Invalidation_fn(ReQ.address,ReQ.SrcNode,ReQ.req_count,ReQ.we,ReQ.evict, ReQ.type))){
				ReqQ.push(ReQ);
				//cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" INV_D in trans state"<<endl;	
			}				
			} break;
		case EVT_D: {
			if((state == INVALID) || (state == SHARED)) {
				DirMem[cl_index] = DirMem[cl_index] & ~(((unsigned int)0x1)<< ((CL_STATUS_STORAGE_BASE)+ReQ.SrcNode)); // removing the node
				unsigned int forwarder = (DirMem[cl_index]>>FORWARDER_BASE) & (NO_OF_NODES-1);
				unsigned int dcc_sp = (DirMem[cl_index]>>DCC_NODE_BASE) & (NO_OF_NODES-1);
				if((forwarder == ReQ.SrcNode) && (((DirMem[cl_index]>>(FORWARDER_EN_BASE)) & 0x1) == true)){
					DirMem[cl_index] = DirMem[cl_index] & ~(((unsigned int)0x1)<< ((FORWARDER_EN_BASE))); // deactivate forwarder mode
					ACKcmd_to_caches(ReQ.address,ReQ.req_count,ReQ.SrcNode);									
				} else if ((((DirMem[cl_index]>>(DCC_NODE_EN_BASE)) & 0x1) == true) && (dcc_sp = ReQ.SrcNode)) {
					DirMem[cl_index] = DirMem[cl_index] & ~(((unsigned int)0x1)<< ((DCC_NODE_EN_BASE))); // deactivate dcc mode
					ACKcmd_to_caches(ReQ.address,ReQ.req_count,ReQ.SrcNode);
					cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<endl;
				} else if (((DirMem[cl_index]>>((CL_STATUS_STORAGE_BASE))) == 0x0) || (state == INVALID)){
					DirMem[cl_index] = 0x0;					
				}
			} else if (state == MODIFIED) {
				DirMem[cl_index] = DirMem[cl_index] & ~(((unsigned int)0x1)<< ((CL_STATUS_STORAGE_BASE)+ReQ.SrcNode)); // removing the node
				cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" Error Invalidation before WB"<<endl;
			} else {
				ReqQ.push(ReQ);
			}
		} break;
		case ACK_NPC:
		case ACK_SC: 
			{
				if( state < S_to_S) { // not in transition state
					cout << "DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" Error DIR in wrong state: "<<(unsigned int) state << "for address:"<<hex<<ReQ.address<<dec<<endl;
				} 
				else 
				{					
					switch(state) {
						case I_to_S_DCC: //SPILL_RD
						case I_to_M_DCC:
						case S_to_S: { // FWD_RD								
									if((ReQ.type == ACK_NPC)) {									
										DirMem[cl_index] = DirMem[cl_index] | ((unsigned int) 0x1 << ((FORWARDER_EN_BASE))); // activate forward
										if(state == I_to_M_DCC) {
											/*send SPL inv to DCC node */
											//Invcmd_to_caches(ReQ.address, ReQ.req_count, ReQ.SrcNode, true);	
											//DirMem[cl_index] = (DirMem[cl_index] & 0xfffffff0) | M_to_M_DCC;
											DirMem[cl_index] = (DirMem[cl_index] & 0xfffffff0) | MODIFIED;
											cout << "DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" DCC read n inv"<<endl;
										} else {
											DirMem[cl_index] = (DirMem[cl_index] & 0xfffffff0) | SHARED; // set state to S
											cout << "DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" DCC read"<<endl;
										}
									} else {
										cout << "DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" Error!!  wrong ack"<<endl;
									}
							} break;
						case M_to_M_DCC: {
							if(ReQ.type == ACK_NPC) {
								DirMem[cl_index] = (DirMem[cl_index] & 0xfffffff0) | MODIFIED;
								cout << "DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" DCC inv"<<endl;
							}
						} break;
						case M_I_WB: { // M to I
									if(ReQ.type == ACK_SC) 
									{
										DirMem[cl_index] = 0x0;
									} else {
										cout << "DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" Error!!  wrong ack"<<endl;
									}
							} break;
						case S_I_DCC:
						case M_I_DCC: { // M to dcc replacement M/I
									if(ReQ.type == ACK_NPC) {
										//TODO enable dcc with dccnode  ReQ.srcnode value
										DirMem[cl_index] = 0x0;
										DirMem[cl_index] = (DirMem[cl_index] & 0xfffffff0) | INVALID; 
										/* Now its DCC node's responsibility to track if the stored value is dirty or clean, and if that needs to evicted how to act on etc. */
										DirMem[cl_index] = DirMem[cl_index] | ((unsigned int)ReQ.SrcNode << (DCC_NODE_BASE));
										DirMem[cl_index] = DirMem[cl_index] | ((unsigned int) 0x1 << (DCC_NODE_EN_BASE));
									} else {
										// evicted down to memory. // like WB 
										if(state == M_I_DCC)
											DirMem[cl_index] = 0x0;
										else {
											DirMem[cl_index] = 0x0;
											cout << "DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" "<<endl;
										}
									}
							} break;
						case S_to_I: // S to I
						case S_to_M: // S to M	
						case M_to_S: // M to S
						case M_to_M: // M to M --> includes M to I, WB and I to M, RD from mem
						case I_to_M: // I to M
						case I_to_S: // I to S --> 1 or more shared state CLs exist and a new one is going to RD data from memory and not from cache
							{
								if(BlockedQ.empty() == true) 
								{
									cout << "DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" Error ACK received for non-blocked req"<< endl;
								} else
								{
									ReqFormat_dir front;
									ReqFormat_dir back = BlockedQ.back();
									do {
										front = BlockedQ.front();			
										if(((front.address >> CL_SIZE) == (ReQ.address >> CL_SIZE)) && (front.req_count == ReQ.req_count))
										{																				
											if((state == S_to_I) || (state == S_to_M)) {
												//case S_to_I: // S to I
												//case S_to_M: { // S to M
												if(ReQ.type == ACK_NPC)
												{
													if(front.inv_ack_count != 0x0)
														front.inv_ack_count--; //= 0x01;
													else 
														cout << "DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" Error: -ve inv"<<endl;
													//cout << "DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" inv ack count:"<< front.inv_ack_count <<endl;
													if(((front.inv_ack_count) == 0x0) && (front.sc_ack_count == 0x0)) {	
														if((front.type == RDX_D) /*&& (((DirMem[cl_index]) & ((unsigned int) 0x1 << ((CL_STATUS_STORAGE_BASE)+front.SrcNode))) == 0x0)*/) {
															cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" 8"<<endl;
															ACKcmd_to_caches(front.address,front.req_count,front.SrcNode);	
															WR_RD_CL_FRM_MEML3(front.address, front.SrcNode, front.req_count, false);
															front.sc_ack_count = 0x1;
															front.inv_ack_count = 0x0;
															BlockedQ.pop();	
															BlockedQ.push(front);
															DirMem[cl_index] = 0x0;
															DirMem[cl_index] = (DirMem[cl_index] & 0xfffffff0) | I_to_M; // set state to I to M
															DirMem[cl_index] = DirMem[cl_index] | (( (unsigned int) 0x1 << (front.SrcNode + (CL_STATUS_STORAGE_BASE)))); //'b1; // set node CL status
														}												
														else if((front.type == INV_D) /*&& (((DirMem[cl_index]) & ((unsigned int) 0x1 << ((CL_STATUS_STORAGE_BASE)+front.SrcNode))) == 0x1)*/){
															#if 1
															//send ack to node as it is asking jus for the exclusive state
															ACKcmd_to_caches(front.address,front.req_count,front.SrcNode);	
															cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" 5"<<endl;
															#endif															
															DirMem[cl_index] = 0x0;
															DirMem[cl_index] = (DirMem[cl_index] & 0xfffffff0) | MODIFIED; // set state to M
															DirMem[cl_index] = DirMem[cl_index] | ((unsigned int) 0x1 << (front.SrcNode + (CL_STATUS_STORAGE_BASE))); //'b1; // set node CL status
															if(back != front)
																BlockedQ.pop();
															else {
																BlockedQ.pop();
																break;
															}
														} 
														else {
															cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" error type:"<<(unsigned int) front.type<<endl;
															BlockedQ.pop();
															BlockedQ.push(front);	
														}
													} 
													else {
														BlockedQ.pop();
														BlockedQ.push(front);																
													}
												} 
												else {
													BlockedQ.pop();
													BlockedQ.push(front);	
												}
											} 
											else if((state == M_to_S) || (state == M_to_M)){
											//case M_to_S: // M to S
											//case M_to_M: { // M to M --> includes M to I, WB and I to M, RD from mem	
												if(ReQ.type == ACK_NPC)
												{
													if(front.inv_ack_count != 0x0)
														front.inv_ack_count--; //= 0x01;
													else 
														cout << "DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" Error: -ve inv"<<endl;
												}
												if(ReQ.type == ACK_SC)
												{
													front.sc_ack_count = 0x0;
												}
												if((ReQ.type == ACK_NPC) && ((front.inv_ack_count) == 0x0)) {
													if (state == M_to_S) {//Forward the flushed data to the requesting node														
														DirMem[cl_index] = 0x0;														
														DirMem[cl_index] = DirMem[cl_index] | ((unsigned int) 0x1 <<(CL_STATUS_STORAGE_BASE+(ReQ.SrcNode)));
														DirMem[cl_index] = DirMem[cl_index] | ((unsigned int) (front.req_count >> MAX_PROCESSOR_COUNT_WIDTH) << FORWARDER_BASE);
														DirMem[cl_index] = DirMem[cl_index] | ((unsigned int) 0x1 <<(CL_STATUS_STORAGE_BASE+(ReQ.req_count >> MAX_PROCESSOR_COUNT_WIDTH)));													
														DirMem[cl_index] = (DirMem[cl_index] & 0xfffffff0) | SHARED; // set state to S
														DirMem[cl_index] = DirMem[cl_index] | ((unsigned int) 0x1 << ((FORWARDER_EN_BASE)));// activate forward
														cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" 10 1 address:"<<hex<<ReQ.address<<" dirindex:"<<(unsigned int)(DirMem[cl_index])<<dec<<endl;
														if(back != front)
															BlockedQ.pop();
														else {
															BlockedQ.pop();
															break;
														}
													} else {
														DirMem[cl_index] = 0x0; 
														DirMem[cl_index] = (DirMem[cl_index] & 0xfffffff0) | I_to_M; // set state to I to M
														cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" 10 2 address:"<<hex<<ReQ.address<<dec<<endl;
														#if 1
														//send ack to node as it is asking jus for the exclusive state
														ACKcmd_to_caches(front.address,front.req_count,front.SrcNode);	
														#endif
														//start the RD req to SC, and keep the blocked req														
														if((front.type == RD_D) || (front.type == RDX_D))
															WR_RD_CL_FRM_MEML3(ReQ.address, (ReQ.req_count >> MAX_PROCESSOR_COUNT_WIDTH), ReQ.req_count,false);
														front.inv_ack_count = 0x0;
														front.sc_ack_count = 0x1;
														BlockedQ.pop();	
														BlockedQ.push(front);
													}
												} 
												else if((ReQ.type == ACK_SC) && ((front.sc_ack_count) == 0x0)){ 
													cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" Error"<<endl;
													if(back != front)
														BlockedQ.pop();
													else {
														BlockedQ.pop();
														break;
													}
												} 
												else {
													BlockedQ.pop();
													BlockedQ.push(front);
												}
											}
											else if((state == I_to_M) || (state == I_to_S)){
											//case I_to_M: // I to M
											//case I_to_S: {// I to S --> 1 or more shared state CLs exist and a new one is going to RD data from memory and not from cache
												if(ReQ.type == ACK_SC) {
													front.sc_ack_count = 0x0;
												}
												if((ReQ.type == ACK_SC) && (front.sc_ack_count == 0x0) && (front.inv_ack_count == 0x0)) {
													if (state == I_to_M) {
														unsigned int clholders_plus_dcc = DirMem[cl_index]>>(DCC_NODE_BASE);
														DirMem[cl_index] = 0x0;
														DirMem[cl_index] = DirMem[cl_index] | (clholders_plus_dcc << (DCC_NODE_BASE));
														DirMem[cl_index] = (DirMem[cl_index] & 0xfffffff0) | MODIFIED; // set state to M
														DirMem[cl_index] = DirMem[cl_index] | ((unsigned int) 0x1 << ((ReQ.req_count >> MAX_PROCESSOR_COUNT_WIDTH) + (CL_STATUS_STORAGE_BASE)));
														cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" 6X, reqcount:"<<ReQ.req_count<<" address:"<<hex<<ReQ.address<<dec<<endl;
													} 
													else {
														unsigned int clholders_plus_dcc = DirMem[cl_index]>>(DCC_NODE_BASE);
														DirMem[cl_index] = 0x0;
														DirMem[cl_index] = DirMem[cl_index] | (clholders_plus_dcc << (DCC_NODE_BASE));
														DirMem[cl_index] = DirMem[cl_index] | ((unsigned int) (ReQ.req_count >> MAX_PROCESSOR_COUNT_WIDTH) << FORWARDER_BASE);
														DirMem[cl_index] = DirMem[cl_index] | ((unsigned int) 0x1 <<(CL_STATUS_STORAGE_BASE+(ReQ.req_count >> MAX_PROCESSOR_COUNT_WIDTH)));	
														DirMem[cl_index] = (DirMem[cl_index] & 0xfffffff0) | SHARED; // set state to S											
														DirMem[cl_index] = DirMem[cl_index] | ((unsigned int) 0x1 << ((FORWARDER_EN_BASE)));// activate forward
														cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" 6- for address:"<<hex<<ReQ.address<<dec<<endl;
													}
													if(back != front)
														BlockedQ.pop();
													else {
														BlockedQ.pop();
														break;
													}														
												} 
												else {
													BlockedQ.pop();
													BlockedQ.push(front);
													cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" 7"<<endl;
												}
											} 
											else {
												cout << "DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" Error!! wrong tran state:"<<(unsigned int)(DirMem[cl_index] & 0x0f)<<endl;
												BlockedQ.pop();
												BlockedQ.push(front);
											}
										} 
										else {
											BlockedQ.pop();
											BlockedQ.push(front);
										}											
									} while(front != back);		
								}
							} break;
						default: {
							cout << "DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" Error!! wrong tran state:"<<(unsigned int)(DirMem[cl_index] & 0x0f)<<endl;
							}
					}
				}		
			} break;
		default: cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<< "Error: DIR::dir_main_thread!!" <<endl; 
	}	
	wait();
	}
}


