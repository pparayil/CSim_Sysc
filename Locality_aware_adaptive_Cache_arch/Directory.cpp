
#include "Directory.h"

void DIR::dir_init(void)
{
	//cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<< " dir_init" << endl;
 	DirMem = new unsigned int[TOTAL_CL_IN_DIR];
	memset(DirMem,0x0,TOTAL_CL_IN_DIR); 
	/*
	Dirmem [cl_status|state]
	DirACCmem [n] [RAT for core n |core n R.utilization | core n mode]
	*/
	for(char j=0; j<NO_OF_NODES;j++) {
		DirACCmem[j] = new unsigned int[TOTAL_CL_IN_DIR];
		memset(DirACCmem[j],0x0,TOTAL_CL_IN_DIR); 
	}
}

unsigned int DIR::get_rat(int address, int nodeaddress){
	/* TODO*/
	unsigned int cl_index = (address)>>(CL_SIZE +TOTAL_NO_OF_NODES_CARRYING_DIR_2);
	bool ratlevel = (DirACCmem[nodeaddress][cl_index] >> RAT_LEVEL_BASE) & 0x1;
	if(ratlevel == false)
		return PCT_VALUE;
	else
		return RATMAX;
}

void DIR::set_ratlevel(int address, int nodeaddress, bool rat){
	unsigned int cl_index = (address)>>(CL_SIZE +TOTAL_NO_OF_NODES_CARRYING_DIR_2);
	DirACCmem[nodeaddress][cl_index] = (DirACCmem[nodeaddress][cl_index] & (~(((unsigned int) 0x1)<<RAT_LEVEL_BASE))) | rat ;	
}

bool DIR::get_rm_mode(int address, int nodeaddress){
	unsigned int cl_index = (address)>>(CL_SIZE +TOTAL_NO_OF_NODES_CARRYING_DIR_2);
	return (DirACCmem[nodeaddress][cl_index] & 0x1);
}

void DIR::set_rm_mode(int address, int nodeaddress, bool mode){
	unsigned int cl_index = (address)>>(CL_SIZE +TOTAL_NO_OF_NODES_CARRYING_DIR_2);
	DirACCmem[nodeaddress][cl_index] = (DirACCmem[nodeaddress][cl_index] & (~((unsigned int)0x1))) | mode;
}



void DIR::core_rm_mode_util_update(int address, int nodeaddress){
	/* */ 
	unsigned int cl_index = (address)>>(CL_SIZE +TOTAL_NO_OF_NODES_CARRYING_DIR_2);	
	if (get_rm_mode(address, nodeaddress) == true) // if remote
	{		
		unsigned int remote_util = (DirACCmem[nodeaddress][cl_index] >> RM_UTIL_BASE)& RATMAX;
		if((remote_util <  get_rat(address, nodeaddress))) {			
			remote_util = remote_util+1;			
			DirACCmem[nodeaddress][cl_index] = (DirACCmem[nodeaddress][cl_index] & ~(((unsigned int)RATMAX)<< (RM_UTIL_BASE))) | (((unsigned int)remote_util) << RM_UTIL_BASE); // remove util and then update
		} else {
			set_rm_mode(address, nodeaddress, false); // set mode to private
		}
	}
		
}

void DIR::core_priv_rm_util_mode_update(int address, int nodeaddress, unsigned int priv_util, bool inv){
	unsigned int cl_index = (address)>>(CL_SIZE +TOTAL_NO_OF_NODES_CARRYING_DIR_2);	
	
	if (get_rm_mode(address, nodeaddress) == false) // if private
	{
		unsigned int remote_util = (DirACCmem[nodeaddress][cl_index] >> RM_UTIL_BASE)& RATMAX;
		if((priv_util+remote_util) < get_rat(address, nodeaddress)){
			set_rm_mode(address, nodeaddress, true); // set to remote
			set_ratlevel(address, nodeaddress, true);
		}else { 
			set_ratlevel(address, nodeaddress, false);
		}
	}
	 
	/*	if (get_rm_mode(address, nodeaddress) == true) // if remote
		{
			unsigned int remote_util = DirACCmem[nodeaddress][cl_index] >> RM_UTIL_BASE)& RATMAX;
			if((remote_util) >= PCT_VALUE){
				set_rm_mode(address, nodeaddress, false); // set to remote
				set_ratlevel(address, nodeaddress, false);
			}
		} */
	
}

void DIR::WR_RD_CL (int address, int nodeaddress, int req_count, bool we) {
	unsigned int cl_index = (address)>>(CL_SIZE +TOTAL_NO_OF_NODES_CARRYING_DIR_2);	
	rd_wr_to_llc = we;
	ce_to_llc = true;
	addr_to_llc = address;
	req_count_to_llc = req_count;
	dst_nodeaddr_to_llc = nodeaddress;
	mode_pri_remote_to_llc = get_rm_mode(address,nodeaddress);
	/* dir to llc mapping: cache line is shared between 2 nodes */
	#ifdef LLC_TRUE_1
	//cout<<flush;
	//cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" : addr: "<<hex<<address<<", cl_index:"<< cl_index<<dec<<endl;
	if((cl_index & 0x1) == 0x0) // 7 = 5(offset) + 2(inode)
	{
	nodeaddr_to_lru = inode;
	//cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" : llc1 nodeaddr: "<<hex<<inode<<dec<<endl;
	}
	else
	{
		if((inode & 0x1) == 0x0)
		{
			nodeaddr_to_lru = inode+1;
			//cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" : llc2 nodeaddr: "<<hex<<inode+1<<dec<<endl;
		}
		else
		{
			nodeaddr_to_lru = inode-1;
			//cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" : llc3 nodeaddr: "<<hex<<inode-1<<dec<<endl;
		}
	}
	#else
	#ifdef LLC_TRUE_0
		nodeaddr_to_lru  = inode;
	#else
		nodeaddr_to_lru = NODE_WITH_SDRAM;
	#endif
	#endif
	wait();
	//cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" : llcnodeaddr: "<<hex<<nodeaddr_to_lru<<dec<<endl;	
	rd_wr_to_llc = false;
	ce_to_llc = false;
	mode_pri_remote_to_llc = false;
	wait();		
}

void DIR::ack_node (int address, int nodeaddress, int req_count, bool mode){
	ack_to_node =  true;
	req_count_from_dir = req_count;
	address_from_dir = address;
	nodeaddr_to_lru = nodeaddress;
	mode_pri_remote_from_dir = mode;
	wait();
	ack_to_node =  false;
	mode_pri_remote_from_dir = false;
}

bool DIR::refill_cl_to_npc (int address, int nodeaddress, int req_count) 
{
	unsigned int cl_index = (address)>>(CL_SIZE +TOTAL_NO_OF_NODES_CARRYING_DIR_2);
	char state;
	char holders;
	bool ret = true;
	//cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<endl;
	state = DirMem[cl_index] & 0x0f;
	if((state == MODIFIED) && (((DirMem[cl_index]>>(nodeaddress+CL_STATUS_STORAGE_BASE)) & 0x1) == false)) { 
	//send flush req to npc , wait for inv_ack, set state 0x1
		//then RD request
		flush_frm_npc = true; 
		req_count_from_dir = req_count;
		inv_addr_to_node = address;
		int i;
		for(i=CL_STATUS_STORAGE_BASE; i<(NO_OF_NODES+CL_STATUS_STORAGE_BASE); i++) {
		if(((DirMem[cl_index]>>i) & 0x1) == true) {
			nodeaddr_to_lru = i-CL_STATUS_STORAGE_BASE;
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
		DirMem[cl_index] = (DirMem[cl_index] & 0xfffffff0) | M_S; // M to S	
		cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" FLUSH to mem send to "<<(i-CL_STATUS_STORAGE_BASE)<<endl;		
		ret = true;
	}
	else if (state < S_M) {
		//send RD req. receive ack from llc/mem then set state to 0x1/0x2
		core_rm_mode_util_update(address, nodeaddress);
		WR_RD_CL(address, nodeaddress, req_count, false);
		ReqFormat_dir Bl_req;
		Bl_req.we = false;
		Bl_req.address = address;
		Bl_req.req_count = req_count;
		Bl_req.evict = false;
		Bl_req.SrcNode = nodeaddress;
		Bl_req.DstNode = nodeaddress;
		Bl_req.inv_ack_count = 0x0; // this inv_count is not reduced 
		Bl_req.sc_ack_count = 0x1; // will be set later
		if(state == MODIFIED) {
			DirMem[cl_index] = (DirMem[cl_index] & 0xfffffff0) | I_M; // I to M
			cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" Error"<<endl;
		}
		else {
			Bl_req.type = RD_D;
			if(state == INVALID)
				DirMem[cl_index] = 0x0;
			DirMem[cl_index] = (DirMem[cl_index] & 0xfffffff0) | I_S; // I to S
		}
		BlockedQ.push(Bl_req);
		ret = true;
	}
	else if (state > MODIFIED){
		// wait put the req back to the queue as trans state
		ret = false;
	}
	return ret;
}

bool DIR::write_to_llc ( int address, int nodeaddress,  bool evict, int req_count, unsigned int priv_util)
{
	unsigned int cl_index = (address)>>(CL_SIZE +TOTAL_NO_OF_NODES_CARRYING_DIR_2);
	char state;
	char holders;
	bool ret =true;
	state = DirMem[cl_index] & 0x0f;
	
	if(state == MODIFIED) {
		cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<endl;
		//WR request to mem/llc 
		//ack received, if evict then evict node
		if(evict == true) {
			DirMem[cl_index] = (DirMem[cl_index] & 0xfffffff0) | M_I_WB; // state M to I
			core_priv_rm_util_mode_update(address,nodeaddress, priv_util, false);
		} else {
			DirMem[cl_index] = (DirMem[cl_index] & 0xfffffff0) | M_S;  // state M to S
		}		
		WR_RD_CL(address, nodeaddress, req_count, true);
		ReqFormat_dir Bl_req;
		Bl_req.sc_ack_count = 0x1;
		Bl_req.inv_ack_count = 0x0;
		Bl_req.we = true;
		Bl_req.address = address;
		Bl_req.req_count = req_count;
		Bl_req.evict = evict;
		Bl_req.type = (ReQType_Dir) WR_D;
		Bl_req.SrcNode = nodeaddress;
		Bl_req.DstNode = nodeaddress;				
		BlockedQ.push(Bl_req);
		ret = true;
	}
	else if (state == INVALID){
		//state INVALID, wrong error. 
		cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" Error: Wrong state WB called from nodes which are at state:0"<< endl;
		if(evict == true){
			DirMem[cl_index] = 0x0; // removing the node
		}
		ret = true;
	} else if (state == SHARED){
		//state SHARED, send invs , wait for all acks, then sent WR to mem, ack received, if evict then evict node 
		if(evict == true){
			DirMem[cl_index] = DirMem[cl_index] & ~(((unsigned int)0x1)<< (CL_STATUS_STORAGE_BASE+nodeaddress)); // removing the node
		}
		cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" Error: Wrong state WB called from nodes which are at state:1"<< endl;
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
	unsigned int cl_holders = ((DirMem[cl_index]) >>CL_STATUS_STORAGE_BASE);
	unsigned int inv_counter;
	ret = true;
	blockedreq = false;	
	state = DirMem[cl_index] & 0x0f;
	cout<<flush;
	//cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<hex<<" address"<<address<<" state:"<<(unsigned int)state<<dec<<endl;
	if ((state == INVALID)) {
		// nothing to be done
		inv_counter = 0x0;
		if(type == RDX_D) {
			DirMem[cl_index] = 0x0;
			core_rm_mode_util_update(address, nodeaddress);
			if(get_rm_mode(address,nodeaddress) == false) {
				DirMem[cl_index] = (DirMem[cl_index] | ((unsigned int) 0x1 << (CL_STATUS_STORAGE_BASE+nodeaddress))) ; 
				DirMem[cl_index] = (DirMem[cl_index] & 0xfffffff0) | I_M;		//I to M
				blockedreq = true;
			} else {
				DirMem[cl_index] = ((DirMem[cl_index]) & 0xFFFFFFF0) | (M_I_WB) ;	
				blockedreq = true;
			}
			
		} else {
			cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" Error!"<<endl;
			DirMem[cl_index] = 0x0;
			blockedreq = false;
		}
		ack_node(address,nodeaddress,req_count,get_rm_mode(address,nodeaddress));		
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
		} else {
		for (unsigned int i=0; i<TOTAL_NO_OF_NODES_CARRYING_DIR; i++)
		{
			if(nodeaddress == i) 
			{			
				if(((cl_holders>>i)& 0x1) == 0x1) {
					if(type == RDX_D){
						cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" Error! RDX address:"<<hex<<address<<dec<<" nodeid:"<<nodeaddress<<endl;					
					} else {
						DirMem[cl_index] = ((DirMem[cl_index]) & 0xFFFFFFF0) | S_M;		//S to M	
					}
				} else {
					if(type == INV_D){
						cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" Error! Exclusion,  address:"<<hex<<address<<dec<<" nodeid:"<<nodeaddress<<endl;
						//DirMem[cl_index] = ((DirMem[cl_index]) & 0xFFFFFFF0) | S_M;		//S to M	
					} else {
						if (state == MODIFIED) {
							DirMem[cl_index] = ((DirMem[cl_index]) & 0xFFFFFFF0) | M_M;		// M to M
							cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" M to M address:"<<hex<<address<<dec<<endl;
							}
						else
							DirMem[cl_index] = ((DirMem[cl_index]) & 0xFFFFFFF0) | S_I;		// S to I and I to M	
					}				
				}
				continue;	
			}
			else if(((cl_holders>>i)& 0x1) == 0x1) /* if(ReQ.SrcNode != NODE_WITH_SDRAM)*/
			{
			cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" invalidate node"<<i<<endl;
			inv_to_node = true;
			inv_addr_to_node = address;
			req_count_from_dir = req_count;
			nodeaddr_to_lru = i;
			inv_counter = inv_counter + 0x1;
			wait();
			inv_to_node = false;			
			remove_all_other_reqs_for_address((unsigned int)i,req_count, address);
			} else {
			continue;
			}			
		}
				
		if((inv_counter == 0x0) && ((type == RDX_D) ||(type == INV_D))) { 
		// no invalidation to complete
			core_rm_mode_util_update(address, nodeaddress);
			ack_node(address,nodeaddress,req_count,get_rm_mode(address,nodeaddress));
			if(type == RDX_D) {
				DirMem[cl_index] = 0x0;
				if(get_rm_mode(address,nodeaddress) == false) {
				DirMem[cl_index] = (DirMem[cl_index] | ((unsigned int) 0x1 << (CL_STATUS_STORAGE_BASE+nodeaddress))); 
				DirMem[cl_index] = (DirMem[cl_index] & 0xfffffff0) | (I_M) ;		//I to M				
				blockedreq = true;				
				} else {
				DirMem[cl_index] = ((DirMem[cl_index]) & 0xFFFFFFF0) | (M_I_WB) ;
				blockedreq = true;				
				}
			} else {
				DirMem[cl_index] = 0x0;
				if(get_rm_mode(address,nodeaddress) == false) {
					DirMem[cl_index] = (DirMem[cl_index] | ((unsigned int) 0x1 << (CL_STATUS_STORAGE_BASE+nodeaddress))) ;
					DirMem[cl_index] = (DirMem[cl_index] & 0xfffffff0) | MODIFIED;		//M
					blockedreq = false;
				} else {
					DirMem[cl_index] = (DirMem[cl_index] & ~(((unsigned int)0x1)<< (CL_STATUS_STORAGE_BASE+nodeaddress))) ; 
					DirMem[cl_index] = ((DirMem[cl_index]) & 0xFFFFFFF0) | (M_I_WB) ;
					blockedreq = true;					
				}
			}
			cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" 1"<<endl;
			cout<<flush;
		} else if((inv_counter > 0x0) && ((type == RDX_D) ||(type == INV_D))) {//start a blocked req with inv count set			
			core_rm_mode_util_update(address, nodeaddress);
			blockedreq = true;
			/*if(get_rm_mode(address,nodeaddress) == false) {
				blockedreq = true;
			} else {
				blockedreq = true;
				if(type == RDX_D) {
					DirMem[cl_index] = ((DirMem[cl_index]) & 0xFFFFFFF0) | (M_I_WB) ;
				} else {
					DirMem[cl_index] = (DirMem[cl_index] & ~(((unsigned int)0x1)<< (CL_STATUS_STORAGE_BASE+nodeaddress))) ;
					DirMem[cl_index] = ((DirMem[cl_index]) & 0xFFFFFFF0) | (M_I_WB) ;
				}
			} */
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
			if(get_rm_mode(address,nodeaddress) == false) {
				WR_RD_CL(address, nodeaddress, req_count, false);
			}
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
	enum state {IDLE, INV, RD_WR, RD_X, ACK_ANY} st;
	st = IDLE; 
	state st1 = IDLE;
	while (1)
	{
		switch (st) {
		case IDLE: {			
				if((inv_ack_to_dir == true)  || ( ack_from_llc == true)) {
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
					ReQ.DstNode = priv_utiliz_to_dir;
					ReQ.SrcNode = node_address_from_lru; //0; //hack as only single npc and npc lru is not implemented
					ReQ.type = EVT_D;
					cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" EVT"<<endl;
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
				ReQ.DstNode = priv_utiliz_to_dir;
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
				if ( ack_from_llc == true) {
					ReQ3.type = ACK_SC;
					ReQ3.address = addr_from_llc;
					ReQ3.req_count = req_count_from_llc;
					ReQ3.SrcNode = node_address_from_lru;
				}
				if(inv_ack_to_dir == true) {
					ReQ3.type = ACK_NPC;
					ReQ3.address = address_to_dir;
					ReQ3.req_count = req_count_to_dir;
					ReQ3.SrcNode = node_address_from_lru; 
					ReQ3.DstNode = priv_utiliz_to_dir;
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
			if (false == write_to_llc(ReQ.address,ReQ.SrcNode,ReQ.evict,ReQ.req_count, ReQ.DstNode)){
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
				core_priv_rm_util_mode_update(ReQ.address, ReQ.SrcNode, ReQ.DstNode, false);//ReQ.DstNode contains priv. utilization
				DirMem[cl_index] = DirMem[cl_index] & ~(((unsigned int)0x1)<< (CL_STATUS_STORAGE_BASE+ReQ.SrcNode)); // removing the node
				if (((DirMem[cl_index]>>(CL_STATUS_STORAGE_BASE)) == 0x0) || (state == INVALID)){
					DirMem[cl_index] = 0x0;					
				}
			} else if (state == MODIFIED) {
				cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" Error Invalidation before WB"<<endl;
			} else {
				ReqQ.push(ReQ);
			}
		} break;
		case ACK_NPC:
		case ACK_SC: 
			{
				if( state < S_M) { // not in transition state
					cout << "DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" Error DIR in wrong state: "<<(unsigned int) state << "for address:"<<hex<<ReQ.address<<dec<<endl;
				} 
				else 
				{					
					switch(state) {
						case S_I: // S to I
						case S_M: // S to M	
						case M_S: // M to S
						case M_M: // M to M --> includes M to I, WB and I to M, RD from mem
						case I_M: // I to M
						case M_I_WB: // M to I
						case I_S: // I to S --> 1 or more shared state CLs exist and a new one is going to RD data from memory and not from cache
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
											if((state == S_I) || (state == S_M)) { /*particularly for invalidation */
												//case S_I: // S to I
												//case S_M: { // S to M
												if(ReQ.type == ACK_NPC)
												{
													/* if inv ack received then,  */
													core_priv_rm_util_mode_update(ReQ.address, ReQ.SrcNode, ReQ.DstNode, true);//ReQ.DstNode contains priv. utilization
													if(front.inv_ack_count != 0x0)
														front.inv_ack_count--; //= 0x1;
													else 
														cout << "DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" Error: -ve inv"<<endl;
													//cout << "DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" inv ack count:"<< front.inv_ack_count <<endl;
													if(((front.inv_ack_count) == 0x0) && (front.sc_ack_count == 0x0)) {	
														if((front.type == RDX_D) /*&& (((DirMem[cl_index]) & ((unsigned int) 0x1 << (CL_STATUS_STORAGE_BASE+front.SrcNode))) == 0)*/) {
															cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" 7"<<endl;															
															core_rm_mode_util_update(front.address, front.SrcNode);
															ack_node(front.address, front.SrcNode,front.req_count, get_rm_mode(front.address, front.SrcNode));
															if(get_rm_mode(front.address, front.SrcNode) == false) {
																WR_RD_CL(front.address, front.SrcNode, front.req_count, false);
																front.sc_ack_count = 0x1;
																front.inv_ack_count = 0x0;
																BlockedQ.pop();	
																BlockedQ.push(front);
																DirMem[cl_index] = 0x0;
																DirMem[cl_index] = (DirMem[cl_index] & 0xfffffff0) | I_M; // set state to I to M
																DirMem[cl_index] = DirMem[cl_index] | (( (unsigned int) 0x1 << (front.SrcNode + CL_STATUS_STORAGE_BASE))); //'b1; // set node CL status
															} else {
																DirMem[cl_index] = 0x0;
																DirMem[cl_index] = (DirMem[cl_index] & 0xfffffff0) | M_I_WB; //ack from wr data updation at L3 
																front.sc_ack_count = 0x1;
																front.inv_ack_count = 0x0;
																BlockedQ.pop();	
																BlockedQ.push(front);
															}
														}												
														else if((front.type == INV_D) /*&& (((DirMem[cl_index]) & ((unsigned int) 0x1 << ((CL_STATUS_STORAGE_BASE)+front.SrcNode))) == 0x1)*/){
															#if 1
															//send ack to node as it is asking jus for the exclusive state
															ack_node(front.address, front.SrcNode,front.req_count, get_rm_mode(front.address, front.SrcNode));
															cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" 5"<<endl;
															#endif															
															DirMem[cl_index] = 0x0;
															if(get_rm_mode(front.address, front.SrcNode) == false) {
																DirMem[cl_index] = (DirMem[cl_index] & 0xfffffff0) | MODIFIED; // set state to M
																DirMem[cl_index] = DirMem[cl_index] | ((unsigned int) 0x1 << (front.SrcNode + CL_STATUS_STORAGE_BASE)); //'b1; // set node CL status
																if(back != front)
																	BlockedQ.pop();
																else {
																	BlockedQ.pop();
																	break;
																}
															} else {
																DirMem[cl_index] = DirMem[cl_index] & ~(((unsigned int)0x1)<< (CL_STATUS_STORAGE_BASE+front.SrcNode ));
																DirMem[cl_index] = (DirMem[cl_index] & 0xfffffff0) | M_I_WB; //ack from wr data updation at L3
																front.inv_ack_count = 0x0;
																front.sc_ack_count = 0x1;
																BlockedQ.pop();	
																BlockedQ.push(front);
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
											else if((state == M_S) || (state == M_M)){ /*flush and wb*/
											//case M_S: // M to S
											//case M_M: { // M to M --> includes M to I, WB and I to M, RD from mem	
												if(ReQ.type == ACK_NPC)
												{
													if(front.inv_ack_count != 0x0)
														front.inv_ack_count--; //= 0x1;
													else 
														cout << "DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" Error: -ve inv"<<endl;
												}
												if(ReQ.type == ACK_SC)
												{
													front.sc_ack_count = 0x0;
												}
												if((ReQ.type == ACK_NPC) && ((front.inv_ack_count) == 0x0)) {
													core_rm_mode_util_update(ReQ.address, (ReQ.req_count >> MAX_PROCESSOR_COUNT_WIDTH));
													if (state == M_S) {
														DirMem[cl_index] = (DirMem[cl_index] & 0xfffffff0) | I_S; // set state I to S						
														if(get_rm_mode(front.address, front.SrcNode) == true)
															DirMem[cl_index] = DirMem[cl_index] & ~(((unsigned int)0x1)<< (CL_STATUS_STORAGE_BASE+front.SrcNode ));
														cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" 10 1 address:"<<hex<<ReQ.address<<dec<<endl;
													} else {
														DirMem[cl_index] = 0x0; 
														if(get_rm_mode(front.address, front.SrcNode) == false) 
															DirMem[cl_index] = (DirMem[cl_index] & 0xfffffff0) | I_M; // set state to I to M
														else
															DirMem[cl_index] = (DirMem[cl_index] & 0xfffffff0) | M_I_WB;
														cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" 10 2 address:"<<hex<<ReQ.address<<dec<<endl;
														#if 1
														//send ack to node as it is asking jus for the exclusive state
														ack_node(front.address, front.SrcNode,front.req_count, get_rm_mode(front.address, front.SrcNode));
														#endif
													}									
													//start the RD req to SC, and keep the blocked req	
													if((get_rm_mode(front.address, front.SrcNode) == false) || (state == M_S)) {
														WR_RD_CL(ReQ.address, (ReQ.req_count >> MAX_PROCESSOR_COUNT_WIDTH), ReQ.req_count,false);
														front.inv_ack_count = 0x0;
														front.sc_ack_count = 0x1;
														BlockedQ.pop();	
														BlockedQ.push(front);
													} else {
														front.inv_ack_count = 0x0;
														front.sc_ack_count = 0x1;
														BlockedQ.pop();	
														BlockedQ.push(front);
													}
												} 
												else if((ReQ.type == ACK_SC) && ((front.sc_ack_count) == 0x0)){ // Modified to state S with 1 sharer
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
											else if((state == I_M) || (state == M_I_WB) || (state == I_S)){
											//case I_M: // I to M
											//case M_I_WB: // M to I
											//case I_S: {// I to S --> 1 or more shared state CLs exist and a new one is going to RD data from memory and not from cache
												if(ReQ.type == ACK_SC) {
													front.sc_ack_count = 0x0;
												}
												if((ReQ.type == ACK_SC) && (front.sc_ack_count == 0x0) && (front.inv_ack_count == 0x0)) {
													if (state == I_M) {
														DirMem[cl_index] = 0x0;
														DirMem[cl_index] = (DirMem[cl_index] & 0xfffffff0) | MODIFIED; // set state to M
														if(get_rm_mode(ReQ.address, ((unsigned int)(ReQ.req_count >> MAX_PROCESSOR_COUNT_WIDTH))) == false)
															DirMem[cl_index] = DirMem[cl_index] | ((unsigned int) 0x1 << ((ReQ.req_count >> MAX_PROCESSOR_COUNT_WIDTH) + CL_STATUS_STORAGE_BASE)); //[(ReQ.req_count >> MAX_PROCESSOR_COUNT_WIDTH) + CL_STATUS_STORAGE_BASE] = 1; //'b1; // set node CL status
														else 
															DirMem[cl_index] = DirMem[cl_index] & ~(((unsigned int)0x1)<< ((ReQ.req_count >> MAX_PROCESSOR_COUNT_WIDTH) + CL_STATUS_STORAGE_BASE ));
														cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" 6X, reqcount:"<<ReQ.req_count<<" address:"<<hex<<ReQ.address<<dec<<endl;
													} 
													else if(state == M_I_WB) {
														DirMem[cl_index] = 0x0; // set state to Invalid
														cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" 6In address:"<<hex<<ReQ.address<<dec<<endl;
													}
													else {
														unsigned int clholders = DirMem[cl_index]>>(CL_STATUS_STORAGE_BASE);
														DirMem[cl_index] = 0x0;
														DirMem[cl_index] = DirMem[cl_index] | (clholders << (CL_STATUS_STORAGE_BASE));
														DirMem[cl_index] = (DirMem[cl_index] & 0xfffffff0) | SHARED; // set state to S
														if(get_rm_mode(ReQ.address, ((unsigned int)(ReQ.req_count >> MAX_PROCESSOR_COUNT_WIDTH))) == false)
															DirMem[cl_index] = DirMem[cl_index] | ((unsigned int) 0x1 << (((unsigned int)(ReQ.req_count >> MAX_PROCESSOR_COUNT_WIDTH)) + CL_STATUS_STORAGE_BASE));
														else 
															DirMem[cl_index] = DirMem[cl_index] & ~(((unsigned int)0x1)<< ((ReQ.req_count >> MAX_PROCESSOR_COUNT_WIDTH) + CL_STATUS_STORAGE_BASE ));
														cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" 6- for address:"<<hex<<ReQ.address<<" CL holders"<<(unsigned int)(DirMem[cl_index]>>CL_STATUS_STORAGE_BASE)<<dec<<endl;
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


