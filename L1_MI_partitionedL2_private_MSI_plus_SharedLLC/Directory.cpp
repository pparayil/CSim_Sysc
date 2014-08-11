
#include "Directory.h"

void DIR::dir_init(void)
{
	//cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<< " dir_init" << endl;
 	DirMem = new unsigned int[TOTAL_CL_IN_DIR];
	memset(DirMem,0x0,TOTAL_CL_IN_DIR); /*[cl_status|state]*/
}

void DIR::WR_RD_CL (int address, int nodeaddress, int req_count, bool we) {
	unsigned int cl_index = (address)>>(CL_SIZE +TOTAL_NO_OF_NODES_CARRYING_DIR_2);
	rd_wr_to_llc = we;
	ce_to_llc = true;
	addr_to_llc = address;
	req_count_to_llc = req_count;
	dst_nodeaddr_to_llc = nodeaddress;
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
	wait();
}


bool DIR::refill_cl_to_npc (int address, int nodeaddress, int req_count) 
{
	unsigned int cl_index = (address)>>(CL_SIZE +TOTAL_NO_OF_NODES_CARRYING_DIR_2);
	char state;
	char holders;
	bool ret = true;
	//cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<endl;
	state = DirMem[cl_index] & 0x0f;
	if((state == 0x02) && (((DirMem[cl_index]>>(nodeaddress+4)) & 0x1) == false)) { 
	//send flush req to npc , wait for inv_ack, set state 0x1
		//then RD request
		flush_frm_npc = true; 
		req_count_from_dir = req_count;
		inv_addr_to_node = address;
		int i;
		for(i=4; i<(NO_OF_NODES+4); i++) {
		if(((DirMem[cl_index]>>i) & 0x1) == true) {
			nodeaddr_to_lru = i-4;
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
		DirMem[cl_index] = (DirMem[cl_index] & 0xfffffff0) | 0x06; // M to S	
		cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" FLUSH to mem send to "<<(i-4)<<endl;		
		ret = true;
	}
	else if (state < 0x03) {
		//send RD req. receive ack from llc/mem then set state to 0x1/0x2		
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
		if(state == 0x02) {
			DirMem[cl_index] = (DirMem[cl_index] & 0xfffffff0) | 0x04; // I to M
			cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" Error"<<endl;
		}
		else {
			Bl_req.type = RD_D;
			if(state == 0x00)
				DirMem[cl_index] = 0x0;
			DirMem[cl_index] = (DirMem[cl_index] & 0xfffffff0) | 0x0a; // I to S
		}
		BlockedQ.push(Bl_req);
		ret = true;
	}
	else if (state > 0x02){
		// wait put the req back to the queue as trans state
		ret = false;
	}
	return ret;
}

bool DIR::write_to_llc ( int address, int nodeaddress,  bool evict, int req_count)
{
	unsigned int cl_index = (address)>>(CL_SIZE +TOTAL_NO_OF_NODES_CARRYING_DIR_2);
	char state;
	char holders;
	bool ret =true;
	state = DirMem[cl_index] & 0x0f;
	
	if(state == 0x02) {
		cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<endl;
		//WR request to mem/llc 
		//ack received, if evict then evict node
		WR_RD_CL(address, nodeaddress, req_count, true);
		if(evict == true) {
			DirMem[cl_index] = (DirMem[cl_index] & 0xfffffff0) | 0x07; // state M to I
		} else {
			DirMem[cl_index] = (DirMem[cl_index] & 0xfffffff0) | 0x06;  // state M to S
		}
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
	else if (state == 0x00){
		//state 0x00, wrong error. 
		cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" Error: Wrong state WB called from nodes which are at state:0"<< endl;
		if(evict == true){
			DirMem[cl_index] = 0x0; // removing the node
		}
		ret = true;
	} else if (state == 0x01){
		//state 0x01, send invs , wait for all acks, then sent WR to mem, ack received, if evict then evict node 
		if(evict == true){
			DirMem[cl_index] = DirMem[cl_index] & ~(((unsigned int)0x1)<< (4+nodeaddress)); // removing the node
		}
		cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" Error: Wrong state WB called from nodes which are at state:1"<< endl;
		ret = true;
	}
	else if (state > 0x02){
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
	unsigned int cl_holders = ((DirMem[cl_index]) >>4);
	unsigned int inv_counter;
	ret = true;
	blockedreq = false;	
	state = DirMem[cl_index] & 0x0f;
	cout<<flush;
	//cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<hex<<" address"<<address<<" state:"<<(unsigned int)state<<dec<<endl;
	if ((state == 0x00)) {
		// nothing to be done
		inv_counter = 0x0;
		if(type == RDX_D) {
			DirMem[cl_index] = 0x0;
			DirMem[cl_index] = (DirMem[cl_index] | ((unsigned int) 0x1 << (4+nodeaddress))) | 0x04;		//I to M
			blockedreq = true;
		} else {
			cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" Error!"<<endl;
			DirMem[cl_index] = 0x0;
			blockedreq = false;
		}
		ack_to_node =  true;
		req_count_from_dir = req_count;
		address_from_dir = address;
		nodeaddr_to_lru = nodeaddress;
		wait();
		ack_to_node =  false;
		cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" 3"<<endl;
		ret = true;
	} 
	else if((state == 0x01) || (state == 0x02)) {
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
			//cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" src not holder ["<<hex<<((cl_index<<7) | (inode<<5))<<"]: "<< DirMem[cl_index]<<endl;
/*
				if(type == RDX_D) {
					if (state == 0x02)
						DirMem[cl_index] = ((DirMem[cl_index]) & 0xFFFFFFF0) | 0x09;		// M to M
					else
						DirMem[cl_index] = ((DirMem[cl_index]) & 0xFFFFFFF0) | 0x08;		// S to I and I to M	
				} else {
					DirMem[cl_index] = ((DirMem[cl_index]) & 0xFFFFFFF0) | 0x05;		//S to M
				}*/
				if(((cl_holders>>i)& 0x1) == 0x1) {
					if(type == RDX_D){
						cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" Error! RDX address:"<<hex<<address<<dec<<" nodeid:"<<nodeaddress<<endl;
					/*	if (state == 0x02)
							DirMem[cl_index] = ((DirMem[cl_index]) & 0xFFFFFFF0) | 0x09;		// M to M
						else
							DirMem[cl_index] = ((DirMem[cl_index]) & 0xFFFFFFF0) | 0x08;		// S to I and I to M	*/
					} else {
						DirMem[cl_index] = ((DirMem[cl_index]) & 0xFFFFFFF0) | 0x05;		//S to M	
					}
				} else {
					if(type == INV_D){
						cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" Error! Exclusion,  address:"<<hex<<address<<dec<<" nodeid:"<<nodeaddress<<endl;
						//DirMem[cl_index] = ((DirMem[cl_index]) & 0xFFFFFFF0) | 0x05;		//S to M	
					} else {
						if (state == 0x02) {
							DirMem[cl_index] = ((DirMem[cl_index]) & 0xFFFFFFF0) | 0x09;		// M to M
							cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" M to M address:"<<hex<<address<<dec<<endl;
							}
						else
							DirMem[cl_index] = ((DirMem[cl_index]) & 0xFFFFFFF0) | 0x08;		// S to I and I to M	
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
			ack_to_node =  true;
			req_count_from_dir = req_count;
			address_from_dir = address;
			nodeaddr_to_lru = nodeaddress;
			wait();
			ack_to_node =  false;
			if(type == RDX_D) {
				DirMem[cl_index] = 0x0;
				DirMem[cl_index] = (DirMem[cl_index] | ((unsigned int) 0x1 << (4+nodeaddress))) | (0x04) ;		//I to M
				blockedreq = true;				
			} else {
				DirMem[cl_index] = 0x0;
				DirMem[cl_index] = (DirMem[cl_index] | ((unsigned int) 0x1 << (4+nodeaddress))) | (0x02) ;		//M
				blockedreq = false;
			}
			cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" 1"<<endl;
			cout<<flush;
		} else if((inv_counter > 0x0) && ((type == RDX_D) ||(type == INV_D))) {//start a blocked req with inv count set
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
			WR_RD_CL(address, nodeaddress, req_count, false);
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
				if((inv_ack_to_dir == true)  || ( ack_from_llc == true)) {
					st1 = ACK_ANY;
				} else {
					st1 = IDLE;
				}				
				if ((ex_to_dir == true) && (ce_to_dir == false ) && (cl_evt_info_to_dir == false))
				{
					st = INV;//cache from S to M
					cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<"Invalidating other sharers!! + changing state to 0x02" << endl;
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
			if (false == write_to_llc(ReQ.address,ReQ.SrcNode,ReQ.evict,ReQ.req_count)){
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
			if((state == 0x00) || (state == 0x01)) {
				DirMem[cl_index] = DirMem[cl_index] & ~(((unsigned int)0x1)<< (4+ReQ.SrcNode)); // removing the node
				if (((DirMem[cl_index]>>(4)) == 0x0) || (state == 0x0)){
					DirMem[cl_index] = 0x0;					
				}
			} else if (state == 0x02) {
				cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" Error Invalidation before WB"<<endl;
			} else {
				ReqQ.push(ReQ);
			}
		} break;
		case ACK_NPC:
		case ACK_SC: 
			{
				if( state < 0x03) { // not in transition state
					cout << "DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" Error DIR in wrong state: "<<(unsigned int) state << "for address:"<<hex<<ReQ.address<<dec<<endl;
				} 
				else 
				{					
					switch(state) {
						case 0x08: // S to I
						case 0x05: // S to M	
						case 0x06: // M to S
						case 0x09: // M to M --> includes M to I, WB and I to M, RD from mem
						case 0x04: // I to M
						case 0x07: // M to I
						case 0x0a: // I to S --> 1 or more shared state CLs exist and a new one is going to RD data from memory and not from cache
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
											if((state == 0x08) || (state == 0x05)) {
												//case 0x08: // S to I
												//case 0x05: { // S to M
												if(ReQ.type == ACK_NPC)
												{
													if(front.inv_ack_count != 0x0)
														front.inv_ack_count--; //= 0x1;
													else 
														cout << "DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" Error: -ve inv"<<endl;
													//cout << "DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" inv ack count:"<< front.inv_ack_count <<endl;
													if(((front.inv_ack_count) == 0x0) && (front.sc_ack_count == 0x0)) {	
														if((front.type == RDX_D) /*&& (((DirMem[cl_index]) & ((unsigned int) 0x1 << (4+front.SrcNode))) == 0)*/) {
															cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" 7"<<endl;
															ack_to_node =  true;
															req_count_from_dir = front.req_count;
															address_from_dir = front.address;
															nodeaddr_to_lru = front.SrcNode;
															wait();
															ack_to_node =  false;
															WR_RD_CL(front.address, front.SrcNode, front.req_count, false);
															front.sc_ack_count = 0x1;
															front.inv_ack_count = 0x0;
															BlockedQ.pop();	
															BlockedQ.push(front);
															DirMem[cl_index] = 0x0;
															DirMem[cl_index] = (DirMem[cl_index] & 0xfffffff0) | 0x04; // set state to I to M
															DirMem[cl_index] = DirMem[cl_index] | (( (unsigned int) 0x1 << (front.SrcNode + 4))); //'b1; // set node CL status
														}												
														else if((front.type == INV_D) /*&& (((DirMem[cl_index]) & ((unsigned int) 0x1 << ((4)+front.SrcNode))) == 0x1)*/){
															#if 1
															//send ack to node as it is asking jus for the exclusive state
															ack_to_node =  true;
															req_count_from_dir = front.req_count;
															address_from_dir = front.address;
															nodeaddr_to_lru = front.SrcNode;
															wait();
															ack_to_node =  false;
															cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" 5"<<endl;
															#endif															
															DirMem[cl_index] = 0x0;
															DirMem[cl_index] = (DirMem[cl_index] & 0xfffffff0) | 0x02; // set state to M
															DirMem[cl_index] = DirMem[cl_index] | ((unsigned int) 0x1 << (front.SrcNode + 4)); //'b1; // set node CL status
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
											else if((state == 0x06) || (state == 0x09)){
											//case 0x06: // M to S
											//case 0x09: { // M to M --> includes M to I, WB and I to M, RD from mem	
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
													if (state == 0x06) {
														DirMem[cl_index] = (DirMem[cl_index] & 0xfffffff0) | 0x0a; // set state I to S
														cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" 10 1 address:"<<hex<<ReQ.address<<dec<<endl;
													} else {
														DirMem[cl_index] = 0x0; 
														DirMem[cl_index] = (DirMem[cl_index] & 0xfffffff0) | 0x04; // set state to I to M
														cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" 10 2 address:"<<hex<<ReQ.address<<dec<<endl;
														#if 1
														//send ack to node as it is asking jus for the exclusive state
														ack_to_node =  true;
														req_count_from_dir = front.req_count;
														address_from_dir = front.address;
														nodeaddr_to_lru = front.SrcNode;
														wait();
														ack_to_node =  false;
														#endif
													}									
													//start the RD req to SC, and keep the blocked req														
													WR_RD_CL(ReQ.address, (ReQ.req_count >> MAX_PROCESSOR_COUNT_WIDTH), ReQ.req_count,false);
													front.inv_ack_count = 0x0;
													front.sc_ack_count = 0x1;
													BlockedQ.pop();	
													BlockedQ.push(front);
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
											else if((state == 0x04) || (state == 0x07) || (state == 0x0a)){
											//case 0x04: // I to M
											//case 0x07: // M to I
											//case 0x0a: {// I to S --> 1 or more shared state CLs exist and a new one is going to RD data from memory and not from cache
												if(ReQ.type == ACK_SC) {
													front.sc_ack_count = 0x0;
												}
												if((ReQ.type == ACK_SC) && (front.sc_ack_count == 0x0) && (front.inv_ack_count == 0x0)) {
													if (state == 0x04) {
														DirMem[cl_index] = 0x0;
														DirMem[cl_index] = (DirMem[cl_index] & 0xfffffff0) | 0x02; // set state to M
														DirMem[cl_index] = DirMem[cl_index] | ((unsigned int) 0x1 << ((ReQ.req_count >> MAX_PROCESSOR_COUNT_WIDTH) + 4)); //[(ReQ.req_count >> MAX_PROCESSOR_COUNT_WIDTH) + 4] = 1; //'b1; // set node CL status
														cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" 6X, reqcount:"<<ReQ.req_count<<" address:"<<hex<<ReQ.address<<dec<<endl;
													} 
													else if(state == 0x07) {
														DirMem[cl_index] = 0x00; // set state to Invalid
														cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" 6In address:"<<hex<<ReQ.address<<dec<<endl;
													}
													else {
														unsigned int clholders = DirMem[cl_index]>>(4);
														DirMem[cl_index] = 0x0;
														DirMem[cl_index] = DirMem[cl_index] | (clholders << (4));
														DirMem[cl_index] = (DirMem[cl_index] & 0xfffffff0) | 0x01; // set state to S
														DirMem[cl_index] = DirMem[cl_index] | ((unsigned int) 0x1 << (((unsigned int)(ReQ.req_count >> MAX_PROCESSOR_COUNT_WIDTH)) + 4));
														cout<<"DIR::"<<inode<<", "<<__func__<<"@ "<<__LINE__<<" 6- for address:"<<hex<<ReQ.address<<" CL holders"<<(unsigned int)(DirMem[cl_index]>>4)<<dec<<endl;
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


