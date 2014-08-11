
#include "NPC.h"

#define LOOKUP_DELAY_NPC 1

// NPCMEM needs to be 4 ways * number of sets
// Lets try the initial static model
// fixate 0 n 1 ways for private 
// 2 n 3 for spill store

void NPC::NPC_init(void){
	int i=NPC_WAYS;
	//cout<<"NPC::"<<__func__<<" : "<< __LINE__<<endl;
	for (int i = 0; i< NPC_WAYS; i++){
	NPCMem[i] =  new datatype [NPC_SET_COUNT];
	memset(NPCMem[i], 0x0,NPC_SET_COUNT);
	for(int j=0;j<NPC_SET_COUNT;j++)
		{
		NPCtag[2][j][i] = 0x0;
		NPCtag[1][j][i] = 0x0;
		NPCtag[0][j][i] = 0x0;
		}
	}
	l2_miss_count = 0x0;
	l2_hit_count = 0x0;
}
bool NPC::find_in_blockedQ(unsigned int address)
{
	bool found;
	found = false;
	if(BlockedQ.empty() == false) {
		ReqFormat front;
		ReqFormat back = BlockedQ.back();
		
		do {
			front = BlockedQ.front();			
			if(((front.address >> CL_SIZE) == (address >> CL_SIZE)))
			{
				found = true;
			} 
			BlockedQ.pop();
			BlockedQ.push(front);
		} while(back != front);
	}
	return found;
}
bool NPC::table_lookup(int address, char * hit_way, int * state, bool only_sp){
	 
	// 8 sets of 8 way associative 8 word cl = 5 bit offset + 10 bit set  + 3 bit in each way for lru = ttl 256kb **** as of now 32sets => 5 bit
	//NPCtag[0][set][way] = tag, NPCtag[1][set][way] = 4 bit state I 0 /V 1/M 2 ..., NPCtag[2][set][way] = 3 bit lru */ 
	bool hit = false;
	if((hit_way == NULL) || (state == NULL)) {
		cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< " error: way not allocated "<<endl;
	}
	int set = (address >> CL_SIZE) & ((unsigned int)NPC_SET_COUNT-1); //tag/set/offset , where offset = 5 (8 words in a cl) +  (10 bits of set) 1024*8*8*4 bytes, ttl size of the NPC cache = 256kb 
	int tag = (address) >> (NPC_SET_COUNT_2 + CL_SIZE);
	for(char i=0; i<NPC_WAYS; i++)
	{
		/*Invalid or splilled hit are considered to be miss
		Spilled hits required to be coherency managed by directory 
		and retraced back to the l2, 
		then read or convert to a private accessible state*/
		if((only_sp == false)){
			if((NPCtag[0][set][i] == tag) && (NPCtag[1][set][i] > INVALID) && ((NPCtag[1][set][i] != S_SP) && (NPCtag[1][set][i] != M_SP))) 
			{
				hit = true;
				*hit_way = (char) i;
				*state = (int) NPCtag[1][set][i];			
				break;
			} else if((NPCtag[0][set][i] == tag) && ((NPCtag[1][set][i] == INVALID) || (NPCtag[1][set][i] == S_SP) || (NPCtag[1][set][i] == M_SP) )){
				*state = (int) NPCtag[1][set][i];
			}	
		} else {
			if((NPCtag[0][set][i] == tag) && ((NPCtag[1][set][i] == S_SP) || (NPCtag[1][set][i] == M_SP))) 
			{
				hit = true;
				*hit_way = (char) i;
				*state = (int) NPCtag[1][set][i];			
				break;
			} else if((NPCtag[0][set][i] == tag) && ((NPCtag[1][set][i] != S_SP) && (NPCtag[1][set][i] != M_SP))){
				*state = (int) NPCtag[1][set][i];
			}	
		} /*else {
			if((NPCtag[0][set][i] == tag) && (NPCtag[1][set][i] > INVALID)){
			hit = true;
			*hit_way = (char) i;
			*state = (int) NPCtag[1][set][i];			
			break;
			} else if if((NPCtag[0][set][i] == tag) && ((NPCtag[1][set][i] == INVALID))){
			*state = (int) NPCtag[1][set][i];
			}
		}*/
	}

	return hit;
}

void NPC::req_to_DIR (bool evict,bool rw,bool ex,bool ce, unsigned int address,unsigned int req_count, bool ecc_en, bool inv_ack) {
	cl_evt_info_to_dir = evict; 
	rd_wr_to_dir = rw;
	ex_to_dir = ex;
	ce_to_dir = ce;
	address_to_dir = address;
	req_count_to_dir = req_count;
	inv_ack_to_dir = inv_ack;
	/*if(ecc_en == true) {
		dcc_redirection_node_to_dir = true; 
		nodeid_to_lsu = ecc_node; 
	} */ /*TODO*/
	wait();
	cl_evt_info_to_dir = false;
	rd_wr_to_dir = false;
	ex_to_dir = false;
	ce_to_dir = false;
	inv_ack_to_dir = false;
	wait();
}
void NPC::write_to_l3 ( int address, datatype * DATA, bool evict, int req_count, int way, bool EN_req_to_directory, unsigned int ack_receiver, bool ecc, bool dirty){
	//cout<<"NPC::"<<__func__<<" : "<< __LINE__<<" evict:"<<(unsigned int)evict<<endl;		
	ReqFormat ReQ;
	if(EN_req_to_directory == true)
	{
		if(ecc == true){
			ReQ.type = ReqFormat::ECC_REPLACEMENT;
			unsigned int ecc_node = rand () % (TTL_NO_NPC); 
			/*if directory is not aware of the data passing from npc-ecc, there cud b coherency issues*/
			req_to_DIR(false, true,false,true,/*bool evict,bool rw,bool ex,bool ce*/address,req_count,false, false);
			ReQ.ex_ack_count = 0x1;
			ReQ.sc_ack_count = 0x0;			
		} else {
			req_to_DIR(true, true,false,true,/*bool evict,bool rw,bool ex,bool ce*/address,req_count,false, false);			
			address_to_l3_en = true;
			nodeid_to_lsu = ack_receiver;
			address_to_l3 = address;
			req_count_to_l3 = req_count;
			if(DATA != NULL)
				data_to_l3->write_data(DATA);
			wait();
			address_to_l3_en = false;
			wait();
			ReQ.type = ReqFormat::WR_BACK;
			ReQ.ex_ack_count = 0x0;
			ReQ.sc_ack_count = 0x1;
		}
	} 
	else {
		cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<<endl;
		generate_wr_req_npc = true;
		if(ack_receiver == inode)
		rd_wr_to_dir = true;
		else 
		rd_wr_to_dir = false;
		address_to_l3_en = true;
		address_to_l3 = address;
		req_count_to_l3 = req_count;
		nodeid_to_lsu = ack_receiver;
		wait();
		generate_wr_req_npc = false;
		rd_wr_to_dir = false;
		address_to_l3_en = false;
		wait();
		address_to_l3_en = true;
		address_to_l3 = address;
		req_count_to_l3 = req_count;
		if(DATA != NULL)
			data_to_l3->write_data(DATA);
		wait();
		address_to_l3_en = false;
		wait();
		ReQ.type = ReqFormat::WR_BACK;
		ReQ.ex_ack_count = 0x0;
		ReQ.sc_ack_count = 0x1;
	}
	if(ack_receiver == inode) { // no ack receiver
		ReQ.address = address;
		if((dirty == true) && (ecc == false)){
		ReQ.linked = false;
		} else {		
		ReQ.linked = true;
		}
		ReQ.ex = dirty;
		ReQ.we = true;
		ReQ.way = way;
		ReQ.data = *DATA;
		ReQ.req_counter = req_count;
		BlockedQ.push(ReQ);	
	} 
	//cout<<"NPC::"<<__func__<<" : "<< __LINE__<<endl;
}

void NPC::INV_L1(unsigned int address,unsigned int req_count){
	inv_to_l1 = true;
	address_to_l1 = address;
	req_count_to_l1 = req_count;
	wait();
	inv_to_l1 = false;
	wait();
}

void NPC::ack_to_L1(bool we,unsigned int address,unsigned int req_count,datatype* data){
	ack_we_to_l1 = we;
	address_to_l1 = address;
	req_count_to_l1 = req_count;
	ack_to_l1 = true;	
	if(we == false) {
		data_to_l1->write_data(data);
	}
	wait();
	ack_to_l1 = false;
	wait();
}

char NPC::find_free_way (int address,int req_count,int *prev_state, bool *status, bool ecc_reception) {
	int set = (address >> CL_SIZE) & ((unsigned int)NPC_SET_COUNT-1); 
	int tag = (address) >> (NPC_SET_COUNT_2 + CL_SIZE);
	char j = 0;	
	unsigned int LRUvalue = 0x0;
	bool found;
	//cout<<"NPC::"<<inode << " : "<<__func__<<" : "<< __LINE__<<endl;	
	*prev_state =  0x0;
	found = false;
	char sng_way_list [NPC_WAYS] = {0,0,0,0};
	unsigned int sng_way_list_counter = 0x0;

	if(ecc_reception) {
		j = (NPC_WAYS-1);
	} else {
		j = 0;
	}
	
	for(; LRUvalue<NPC_WAYS; LRUvalue++) {
		for(; j<NPC_WAYS; j++)
		{
			if ((ecc_reception == false) && (j == (NPC_WAYS-1))) break;
			if (NPCtag[2][set][j] == LRUvalue)
			{
				int existingaddr = (NPCtag[0][set][j]<<(NPC_SET_COUNT_2 + CL_SIZE)) | ((set & ((unsigned int)NPC_SET_COUNT-1))<<(CL_SIZE)) | 0b00000;
				*prev_state = NPCtag[1][set][j];
						
				if ((NPCtag[1][set][j] == SHARED) ) //shared or shared spilled
				{
					if(ecc_reception) 
						cout<<"NPC::"<<inode << " : "<<__func__<<" : "<< __LINE__<<" In shared state:"<<NPCtag[1][set][j]<<endl;					
					req_to_DIR(true, false,false,false,/*bool evict,bool rw,bool ex,bool ce*/existingaddr,req_count,false, false);
					if(NPCtag[1][set][j] == SHARED) INV_L1(existingaddr,req_count); // wait is thr inside this function					
					NPCtag[1][set][j] = INVALID;
					found = true;
					
				} else if ((NPCtag[1][set][j] == FORWARD) || (NPCtag[1][set][j] == S_SP)) 
				{ // not singlet
					if(ecc_reception)  
						cout<<"NPC::"<<inode << " : "<<__func__<<" : "<< __LINE__<<" In FWD state"<<endl;		
					//INV_L1(existingaddr,req_count); // wait is thr inside this function
					req_to_DIR(true, false,false,false,/*bool evict,bool rw,bool ex,bool ce*/existingaddr,req_count,false, false);
					NPCtag[1][set][j] = F_I; //F to I
					ReqFormat ReQ;
					ReQ.address = existingaddr;
					ReQ.type = ReqFormat::FWD_RD;
					ReQ.ex = false;
					ReQ.we = false;
					ReQ.way = j;
					ReQ.ex_ack_count = 0x1;
					ReQ.sc_ack_count = 0x0;
					ReQ.linked = true;
					ReQ.req_counter = req_count;
					BlockedQ.push(ReQ);
					found = true;					
				} else if ((NPCtag[1][set][j] == M_SP))
				{
					if(ecc_reception) cout<<"NPC::"<<inode << " : "<<__func__<<" : "<< __LINE__<<" In Modified Spilled state"<<endl;
					write_to_l3(existingaddr,&NPCMem[j][set], true,req_count,j, true, inode, false, false); // eviction WBs
					//INV_L1(existingaddr,req_count); // wait is thr inside this function					
					req_to_DIR(true, false,false,false,/*bool evict,bool rw,bool ex,bool ce*/existingaddr,req_count,false, false);
					if(existingaddr == 0x7b20) cout<<"NPC::"<<inode << " : "<<__func__<<" : "<< __LINE__<<" M_I_WB set way:"<<(unsigned int)j<<endl;
					NPCtag[1][set][j] = M_I_WB; // M to I 					
					found = true;
				}

				else if ((NPCtag[1][set][j] == MODIFIED) || (NPCtag[1][set][j] == FORWARD_SNG)) // Modified / fwd singlet
				{
					//cout<<"NPC::"<<inode << " : "<<__func__<<" : "<< __LINE__<<" Modified state, tag:"<<hex<<NPCtag[0][set][j] <<" new addr:"<<temp_new_address <<" set:"<<set<<dec<<endl;
					if(ecc_reception == false){
						sng_way_list_counter ++;
						sng_way_list[(sng_way_list_counter- 1)] = j;
						/*if(NPCtag[1][set][j] == MODIFIED)
							write_to_l3(existingaddr,&NPCMem[j][set], true,req_count,j, true, inode, true, true); // eviction ECC replacement
						else 
							write_to_l3(existingaddr,&NPCMem[j][set], true,req_count,j, true, inode, true, false); 
						NPCtag[1][set][j] = SNG_ECC_TRANS; // M/F_SNG to I as ecc replacement
						//INV_L1(existingaddr,req_count); 
						cout<<"NPC::"<<inode << " : "<<__func__<<" : "<< __LINE__<<" SNG ECC on move, address:"<<hex<<existingaddr<<dec<<endl;
						found = true;*/
					} else {
						continue;
					}
				}
				else if((NPCtag[1][set][j] > M_SP)) // trans state dont remove this /*either wait for this to get proper state (probable deadlock) or take LRU number 1*/
				{
					continue;
				}
				else
				{
					//cout<<"NPC::"<<inode << " : "<<__func__<<" : "<< __LINE__<<endl;
					found = true;
				}
				break;
			}
			
		}
		if((ecc_reception == true) && (j == NPC_WAYS))  {
			j = (NPC_WAYS-1);
		} else if ((ecc_reception == false) && (j == (NPC_WAYS-1))) {//cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<<" Error No way with LRU 0!!"<<endl;
			j = 0;
		} else 
			break;
	}
	if((found == false) && (sng_way_list_counter == 0x0)) {
		//cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<<" Error No way with LRU minimum and without trans!!"<<endl;		
		*status = false;
	}  else if ((found == false) && (sng_way_list_counter != 0x0)) {
		j = sng_way_list[0];
		int existingaddr = (NPCtag[0][set][j]<<(NPC_SET_COUNT_2 + CL_SIZE)) | ((set & ((unsigned int)NPC_SET_COUNT-1))<<(CL_SIZE)) | 0b00000;
		if(NPCtag[1][set][j] == MODIFIED)
			write_to_l3(existingaddr,&NPCMem[j][set], true,req_count,j, true, inode, true, true); // eviction ECC replacement
		else 
			write_to_l3(existingaddr,&NPCMem[j][set], true,req_count,j, true, inode, true, false); 
		NPCtag[1][set][j] = SNG_ECC_TRANS; // M/F_SNG to I as ecc replacement
		//INV_L1(existingaddr,req_count); 
		cout<<"NPC::"<<inode << " : "<<__func__<<" : "<< __LINE__<<" SNG ECC on move, address:"<<hex<<existingaddr<<dec<<endl;
		*status = true;		
	}
	return j;
}

void NPC::LRU_update(bool down_up, unsigned int set,unsigned int way ) {
	if(down_up == false) {
		for(int j = 0;j<NPC_WAYS; j++)
		{
			//cout<<"NPCtag[2][set]: "<<NPCtag[2][set][j]<<endl;
			if (j== way) continue;
			if(NPCtag[2][set][j] < NPCtag[2][set][way]) // invalidated line is least preferred as cud be 
			{
				NPCtag[2][set][j] = NPCtag[2][set][j]+1;
			}		
			
		}
		NPCtag[2][set][way] = 0x0;
	} 
	else {
		for(char j=0; j<NPC_WAYS; j++)
		{
			if (j == way) continue;
			else if (NPCtag[2][set][j] != 0)
			{
			NPCtag[2][set][j] = NPCtag[2][set][j] - 1;
			}
		}
		NPCtag[2][set][way] = NPC_WAYS-1;
	}
}
bool NPC::rd_rdx_to_dir (int address, bool Exclusive, bool refill,datatype* data,int req_count){
	//cout<<"NPC::"<<inode << " : "<<__func__<<" : "<< __LINE__<<endl;
	req_to_DIR(false, false,Exclusive,refill,/*bool evict,bool rw,bool ex,bool ce*/address,req_count,false, false);
}

void NPC::fwd_rd_to_npc(unsigned int nodeid, unsigned int address,unsigned int req_count,datatype* data) {
	forward_data_to_npc = true;
	nodeid_to_lsu = nodeid; 
	address_to_l3 = address;
	req_count_to_l3 = req_count;
	data_to_l3->write_data(data);
	wait();
	forward_data_to_npc = false;
	wait();
}

bool NPC::invalidating_cl (int address ,int way, int req_count, unsigned int state, ReqFormat::ReQType tp, unsigned int redirect_node)
{
	int set = (address >> CL_SIZE) & ((unsigned int)NPC_SET_COUNT-1); 
	int tag = (address) >> (NPC_SET_COUNT_2 + CL_SIZE);
	bool ret = true;
	//cout<<"NPC::"<<inode << " : "<<__func__<<" : "<< __LINE__<<endl;
	if ((state == SHARED) || (state == FORWARD) || (state == FORWARD_SNG)) { //valid or fwd state
		//cout<<"NPC::"<<inode << " : "<<__func__<<" : "<< __LINE__<<"invalidate shared"<<endl;
		if(tp == ReqFormat::INV){ // Shared state or FWD state
			NPCtag[1][set][way] = INVALID;
			LRU_update(false, set, way);
			if(!(state == S_SP))
				INV_L1(address, req_count);
		} 
		else if ((tp == ReqFormat::FLUSH) &&((state == SHARED) ||(state == FORWARD) || (state == FORWARD_SNG))) {
			NPCtag[1][set][way] = SHARED; 			
			fwd_rd_to_npc(redirect_node, address,req_count,&NPCMem[way][set]);			
			cout<<"NPC::"<<inode << " : "<<__func__<<" : "<< __LINE__<<"Error Test!"<<endl;
		} 
		else {
			cout<<"NPC::"<<inode << " : "<<__func__<<" : "<< __LINE__<<"Error Unknown state!"<<endl;
			return true;
		}
		#ifdef INV_4PHASE		
		req_to_DIR(false, false,false,false,/*bool evict,bool rw,bool ex,bool ce*/address,req_count,false, true);
		#endif	
		ret = true;
	} 
	else if ((state == MODIFIED) ) {// Modified, hence require WB 
		cout<<"NPC::"<<inode << " : "<<__func__<<" : "<< __LINE__<<" : invalidate Modified"<<endl;
		
		ReqFormat ReQ;
		if(tp == ReqFormat::FLUSH) { /*check if in M_SP and flush requests are received? */
			write_to_l3(address,&NPCMem[way][set], false,req_count, way, false, inode, false, false); //dir already knws the status of cache
			ReQ.type = ReqFormat::FLUSH;
			ReQ.redirect_nodeid = redirect_node;
			if(address == 0x7b20) cout<<"NPC::"<<inode << " : "<<__func__<<" : "<< __LINE__<<" M_I_WB set way:"<<(unsigned int)way<<endl;
			NPCtag[1][set][way] = M_I_WB; // M to I + flush = M to S 
			if (state == M_SP)
			{
				cout<<"NPC::"<<inode << " : "<<__func__<<" : "<< __LINE__<<"Error Test!"<<endl;
			}
		} 
		else {
			write_to_l3(address,&NPCMem[way][set], false,req_count, way, false, inode, false, false); 
			ReQ.type = ReqFormat::INV;
			ReQ.redirect_nodeid = redirect_node;
			if(address == 0x7b20) cout<<"NPC::"<<inode << " : "<<__func__<<" : "<< __LINE__<<" M_I_WB set way:"<<(unsigned int)way<<endl;
			NPCtag[1][set][way] = M_I_WB; // M to I
			INV_L1(address, req_count);
		}
		/* do invalidation when ack for the blocked req is received */		
		ReQ.address = address;
		ReQ.ex = false;
		ReQ.we = false;
		ReQ.way = way;
		ReQ.ex_ack_count = 0x0;
		ReQ.sc_ack_count = 0x0;
		ReQ.req_counter = req_count;
		ReQ.linked = false;
		BlockedQ.push(ReQ);
		ret = true;
	}
	else if (state > M_SP) //transition state
	{
		//search blockedQ		
		if(BlockedQ.empty() == true) 
		{
			if(tp != ReqFormat::FLUSH) { 
				NPCtag[1][set][way] = INVALID;
				INV_L1(address, req_count);
			}
			#ifdef INV_4PHASE
			req_to_DIR(false, false,false,false,/*bool evict,bool rw,bool ex,bool ce*/address,req_count,false, true);
			#endif
			cout<<"NPC::"<<inode << " : "<<__func__<<" : "<< __LINE__<<" Error INV 1"<<endl;
		} else
		{
			ReqFormat front,fronttemp;
			ReqFormat back = BlockedQ.back();
			do {
				front = BlockedQ.front();			
				if(((front.address >> CL_SIZE) == (address >> CL_SIZE))  && ((front.type != ReqFormat::WR_BACK) && (front.type != ReqFormat::ECC_REPLACEMENT) && (front.type != ReqFormat::INV) && (front.type != ReqFormat::FLUSH)))
				{
					INV_L1(front.address, req_count);
					if(tp != ReqFormat::FLUSH) {
						NPCtag[1][set][way] = INVALID;		
						cout<<"NPC::"<<inode << " : "<<__func__<<" : "<< __LINE__<<" INV 2"<<endl;
						#ifdef INV_4PHASE
						req_to_DIR(false, false,false,false,/*bool evict,bool rw,bool ex,bool ce*/address,req_count,false, true);
						#endif
						LRU_update(false, set, way);
						ret = true;
						if(back != front)
							BlockedQ.pop();
						else {
							BlockedQ.pop();
							break;
						}
					} 
					else if((tp == ReqFormat::FLUSH) && ((NPCtag[1][set][way] == S_M) || (NPCtag[1][set][way] == F_M))) {
						fwd_rd_to_npc(redirect_node, address,req_count,&NPCMem[way][set]);
						#ifdef INV_4PHASE
						req_to_DIR(false, false,false,false,/*bool evict,bool rw,bool ex,bool ce*/address,req_count,false, true);
						#endif
						NPCtag[1][set][way] = SHARED;
						ret = true;
						if(back != front)
							BlockedQ.pop();
						else {
							BlockedQ.pop();
							break;
						}
					} 
					else {
						ret = false;
						BlockedQ.pop();
						BlockedQ.push(front);
						cout<<"NPC::"<<inode << " : "<<__func__<<" : "<< __LINE__<<" Error!!! state:"<<NPCtag[1][set][way] << " type:"<<front.type<<endl;
					}
				} 
				else if (((front.address >> CL_SIZE) == (address >> CL_SIZE))  && ((front.type == ReqFormat::WR_BACK) || (front.type == ReqFormat::ECC_REPLACEMENT) || (front.type == ReqFormat::FWD_RD)) && (front.linked == true)) 
				{
					if(back != front)
						BlockedQ.pop();
					else {
						BlockedQ.pop();
						cout<<"NPC::"<<inode << " : "<<__func__<<" : "<< __LINE__<<" Error"<<endl;
						break;
					}		
					fronttemp = BlockedQ.front();
					if((fronttemp.type == ReqFormat::WR) || (fronttemp.type == ReqFormat::RD)){
						if((front.type == ReqFormat::WR_BACK)) {
							generate_wr_req_npc = true;
							address_to_l3_en = true;
							nodeid_to_lsu = inode;
							address_to_l3 = front.address;
							req_count_to_l3 = front.req_counter;
							wait();
							generate_wr_req_npc = false;
							address_to_l3_en = false;
							wait();						
							BlockedQ.push(front);
							INV_L1(fronttemp.address, fronttemp.req_counter);
		                                        fronttemp.type = tp;
		                                        fronttemp.address = address;
		                                        fronttemp.req_counter = req_count;
		                                        fronttemp.redirect_nodeid = redirect_node;
		                                        fronttemp.we = false;
		                                        BlockedQ.pop();
		                                        BlockedQ.push(fronttemp);
						} else if (front.type == ReqFormat::ECC_REPLACEMENT) {
							write_to_l3(front.address,&front.data, false,front.req_counter, front.way, false, inode, false, false);
							if(front.address == 0x7b20) cout<<"NPC::"<<inode << " : "<<__func__<<" : "<< __LINE__<<" M_I_WB set way:"<<(unsigned int)front.way<<endl;
							NPCtag[1][set][front.way] = M_I_WB;
							INV_L1(fronttemp.address, fronttemp.req_counter);
		                                        fronttemp.type = tp;
		                                        fronttemp.address = address;
		                                        fronttemp.req_counter = req_count;
		                                        fronttemp.redirect_nodeid = redirect_node;
		                                        fronttemp.we = false;
		                                        BlockedQ.pop();
		                                        BlockedQ.push(fronttemp);
						}else if((front.type == ReqFormat::FWD_RD)){
		                                        INV_L1(fronttemp.address, fronttemp.req_counter);
							if(back != fronttemp)
		                                        	BlockedQ.pop();
		                               		else {
		                                        	BlockedQ.pop();
		                                        	break;
		                                	}						
						}
						ret = true;						
						cout<<"NPC::"<<inode << " : "<<__func__<<" : "<< __LINE__<<" Invalidated due INV command"<<endl;
						break;
					} 
					else if((fronttemp.type == ReqFormat::INV) || (fronttemp.type == ReqFormat::FLUSH)){
						// nothing to do as there two reqs received for the same action take the last one and delete the first
						fronttemp.type = tp;
						fronttemp.redirect_nodeid = redirect_node;
						BlockedQ.push(front);
						BlockedQ.pop();
						BlockedQ.push(fronttemp);
						ret = true;
						break;
					} 
					else {
						cout<<"NPC::"<<inode << " : "<<__func__<<" : "<< __LINE__<<" Error! type :"<<(unsigned int) fronttemp.type<<endl; //wait
						BlockedQ.pop();
						BlockedQ.push(fronttemp);
						ret = false;
					}										
				}else if(((front.address >> CL_SIZE) == (address >> CL_SIZE))  &&((front.type == ReqFormat::INV) || (front.type == ReqFormat::FLUSH))) {
					// nothing to do as there two reqs received for the same action take the last one and delete the first
					front.type = tp;
					front.redirect_nodeid = redirect_node;
					BlockedQ.pop();
					BlockedQ.push(front);	
					ret = true;
					break;
				}
				else 
				{
					BlockedQ.pop();
					BlockedQ.push(front);
				}
			} while(back != front);
			cout<<"NPC::"<<inode << " : "<<__func__<<" : "<< __LINE__<<endl;
		}		
	} 	 
	else {
		if((state != S_SP) && (state != M_SP))
			NPCtag[1][set][way] = INVALID;
		cout<<"NPC::"<<inode << " : "<<__func__<<" : "<< __LINE__<<" Error! address:"<<hex<<address<<dec<<endl; //wait
		#ifdef INV_4PHASE
		req_to_DIR(false, false,false,false,/*bool evict,bool rw,bool ex,bool ce*/address,req_count,false, true);
		#endif
		LRU_update(false, set, way);
	}
	return ret;	
}

void NPC::request_collector(void){
	//cout<<"NPC::"<< inode << " : "<<__func__<<" : "<< __LINE__<<dec<<endl;	
	enum state {IDLE, INV, RD_WR, ACK_ANY,FW} st,st1;
	char way;
	int state;
	bool hit, hit1, hit2;
	bool ack_noc;
	st1 = IDLE;
	st = IDLE;
	while (1)
	{
		switch (st) {
		case IDLE: {					
					ack_noc = ack_from_dir | ack_from_l3;
					if((ce_from_l1 == true) || (forward_req_from_dir == true) || (inv_frm_dir == true) || (flush_frm_npc_dir == true) || ((ack_noc == true))){
						//cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<<"ack/inv with hit or ce"<< endl;
						if ((ce_from_l1 == true) && ((inv_frm_dir == true) || (flush_frm_npc_dir == true) || (forward_req_from_dir == true) || (ack_noc == true)))
						{
							if ((inv_frm_dir == true) || (flush_frm_npc_dir == true))
							{	
								//cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<<endl;	
								st = INV;
								st1 = RD_WR;
							} else if (ack_noc == true)
							{	
								//cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<<endl;	
								st = ACK_ANY;								
								st1 = RD_WR;
							} else if (forward_req_from_dir == true)
							{	
								//cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<<endl;	
								st = FW;
								st1 = RD_WR;
							} 
							else 
							{	
								//cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<<endl;	
								st = RD_WR;
								st1 = IDLE;
							}
						}
						else if ((inv_frm_dir == true) || (flush_frm_npc_dir == true))
						{
							//cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<<" @time: "<<sc_time_stamp()<<"@npc CE is "<<CE_DB<<endl;
							st = INV;
							st1 = IDLE;	
						}
						else if (ack_noc == true) 
						{
							st = ACK_ANY;
							st1 = IDLE;
						}						
						else if(forward_req_from_dir == true)
						{
							st = FW;
							st1 = IDLE;
						} 
						else if(ce_from_l1 == true){
							st = RD_WR;
							st1 = IDLE;
						}
						else
						{
							cout<<inode<<" : "<<"NPC::"<<__func__<<" : "<< __LINE__<<" Error!!"<<endl;
							wait(); 
							st = IDLE;
							st1 = IDLE;
						}							
						
					} 
					else
					{
						wait(); 
						st = IDLE;
						st1 = IDLE;
					}					
					} break;
		case INV: {
					ReqFormat ReQ1;
					if(flush_frm_npc_dir == true) {
						ReQ1.address = inv_addr_from_dir;
						ReQ1.req_counter = req_count_from_dir; //((inode<<3) | 0x0);
						//ReQ.way = way;
						ReQ1.type = ReqFormat::FLUSH;
						ReQ1.redirect_nodeid = nodeid_from_lsu;
						cout<<"NPC::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<" FLUSH for address:0x"<<hex<<inv_addr_from_dir<<dec<<endl;	
					}
					else{
						ReQ1.address = inv_addr_from_dir;
						ReQ1.req_counter = req_count_from_dir; //((inode<<3) | 0x0);
						//ReQ.way = way;
						ReQ1.type = ReqFormat::INV;
						ReQ1.ex = spill_inv_frm_dir;
						ReQ1.redirect_nodeid = nodeid_from_lsu;
						cout<<"NPC::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<" INV for address:0x"<<hex<<inv_addr_from_dir<<dec<<endl;	
					}	
					ReqQ.push(ReQ1);
					st = st1;
					if(st1 == IDLE) 
						wait();			
				} break;
		case RD_WR: {
					ReqFormat ReQ2;
					ReQ2.address = address_from_l1;
					ReQ2.req_counter = req_count_from_l1;
					//ReQ.way = way;
					if(rd_wr_from_l1 == true)
					{ 						
						ReQ2.we = true;
						ReQ2.ex = true;
						ReQ2.type = ReqFormat::WR;
						data_from_l1->read_data(&ReQ2.data);
						cout<<"NPC::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<" WR"<<endl;	
					}
					else
					{ // read case
						ReQ2.we = false;
						ReQ2.ex = false;
						ReQ2.type = ReqFormat::RD;
						cout<<"NPC::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<" RD"<<endl;				
					}
					ReqQ.push(ReQ2); 
					st = IDLE;
					st1 = IDLE;
					wait();
				} break;
		case FW:{
					ReqFormat ReQ;
					ReQ.fwd = true;
					ReQ.we = false;
					ReQ.address = address_from_dir;
					ReQ.req_counter = req_count_from_dir;
					if(forward_spillreq_from_dir == true) {
						ReQ.type = ReqFormat::SPILL_RD;
					} else{
						ReQ.type = ReqFormat::FWD_RD;
						ReQ.ex = false;
					}
					ReQ.redirect_nodeid = nodeid_from_lsu;
					ReqQ.push(ReQ); 
					st = st1;
					cout<<"NPC::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<" FWD"<<endl;
					if(st1 == IDLE) 
						wait();
				} break;				
		case ACK_ANY:{
					ReqFormat ReQ3;
					if((ack_from_dir == true)) {
						if (replace_req_dcc_ack_from_dir == true) {
							ReQ3.type = ReqFormat::REPL_REQ_ECC_ACK;
							ReQ3.redirect_nodeid = nodeid_from_lsu;
							ReQ3.ex = dcc_transfer_en_from_dir;
						} else {							
							ReQ3.type = ReqFormat::ACK_SMALL;
						}
						ReQ3.address = address_from_dir;
						ReQ3.req_counter = req_count_from_dir; //((inode<<3) | 0x0);
						cout<<"NPC::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<" ACK from dir"<<endl;
					}
					else if (ack_from_l3 == true) {
						if(ack_from_npc == true){
							ReQ3.type = ReqFormat::FWD_RD_ACK;							
							data_from_l3->read_data(&ReQ3.data);
							ReQ3.ex = forward_spilldata_ack_from_npc;						
						} else if (replace_data_dcc_from_npc == true) {
							ReQ3.type = ReqFormat::SPILL_DATA_ECC;
							data_from_l3->read_data(&ReQ3.data);
							ReQ3.redirect_nodeid = nodeid_from_lsu;							
							ReQ3.ex = spill_data_dirty_from_npc;
						} else if (replace_data_dcc_ack_from_npc == true) {
							ReQ3.type = ReqFormat::SPILL_DATA_ECC_ACK;
						} else {
							ReQ3.type = ReqFormat::ACK_FULL;
							data_from_l3->read_data(&ReQ3.data);
						}					
						ReQ3.address = address_from_l3;
						ReQ3.req_counter = req_count_from_l3; //((inode<<3) | 0x0);	
						cout<<"NPC::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<" ACK from FWDack/MEM/L3"<<endl;
					}
					//ReQ.way = way;	
					ReqQ.push(ReQ3);
					st = st1;
					if(st1 == IDLE) 
						wait();					
					} break;			
		default: { 
				cout<<"NPC::"<<inode<<" : "<<__func__<<" : "<< __LINE__<< "Error!!, state:" <<st<<endl; 
				st = st1;
				if(st1 == IDLE) 
					wait(); 
			}
		}
	}
}

void NPC::npc_main_thread(void){

	//cout<<"NPC::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<endl;
	char way;
	int state;
	bool hit;
	int set;
	ReqFormat progrs_ReQ;
	bool req_in_progress;
	while (1) {	
		while(ReqQ.empty() == true) wait();
		state = INVALID;
		hit = false;
		set = 0;
		way = 0;
		//cout<<"NPC::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<endl;	
		progrs_ReQ = ReqQ.front();
		ReqQ.pop();
		req_in_progress = true;
		hit = table_lookup(progrs_ReQ.address, &way, &state, false);
		set = (progrs_ReQ.address >> CL_SIZE) & ((unsigned int)NPC_SET_COUNT-1);
		sc_time start = sc_time_stamp();
		switch (progrs_ReQ.type) {
			case ReqFormat::FLUSH:
			case ReqFormat::INV: {
					bool ret = false;
					if(progrs_ReQ.ex == true)
						hit = table_lookup(progrs_ReQ.address, &way, &state, true);						
					//cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<<"INV/FLUSH received"<<endl;
					if (hit == true) 
						ret = invalidating_cl(progrs_ReQ.address,way,progrs_ReQ.req_counter,state,progrs_ReQ.type,progrs_ReQ.redirect_nodeid); 
					else {	// hit false so already invalidated 
						if(ReqFormat::FLUSH == progrs_ReQ.type) {
							cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<<"Error: FLUSH invalid CL? "<<endl;							
						} 
						ret = true;
						#ifdef INV_4PHASE						
						req_to_DIR(false, false,false,false,/*bool evict,bool rw,bool ex,bool ce*/progrs_ReQ.address,progrs_ReQ.req_counter,false, true);						
						#endif	
					}
					if (ret == false) {
						ReqQ.push(progrs_ReQ);
					}
					req_in_progress = false;					
				} break;
			case ReqFormat::SPILL_DATA_ECC_ACK: // npc or memory
			case ReqFormat::REPL_REQ_ECC_ACK: // Dir
			case ReqFormat::FWD_RD_ACK:
			case ReqFormat::ACK_SMALL:
			case ReqFormat::ACK_FULL:{
					cout<<flush;
					bool done = false;
					while(done == false) {
					done = true;
					if ((hit == false) || ((state < F_M))) {
						if(state == INVALID)
						cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<<" State:0 hit:"<<(unsigned int) hit <<"address:"<<hex<<progrs_ReQ.address<<dec<<endl;
						//if(((progrs_ReQ.address == 0x7b20) || (progrs_ReQ.address == 0x1cc340)) && (inode == 0xc)) 
						{						
							int set = (progrs_ReQ.address >> CL_SIZE) & ((unsigned int)NPC_SET_COUNT-1); //tag/set/offset , where offset = 5 (8 words in a cl) +  (10 bits of set) 1024*8*8*4 bytes, ttl size of the NPC cache = 256kb 
							int tag = (progrs_ReQ.address) >> (NPC_SET_COUNT_2 + CL_SIZE);
							char i ;
							for(i = way+1;i<NPC_WAYS;i++)
							if((NPCtag[0][set][i] == tag) && (NPCtag[1][set][i] > INVALID) && ((NPCtag[1][set][i] != S_SP) && (NPCtag[1][set][i] != M_SP))) 
							{
								if(i == way) continue;
								hit = true;
								way = (char) i;
								state = (int) NPCtag[1][set][i];
								done = false;
								if((progrs_ReQ.address == 0x7b20) || (progrs_ReQ.address == 0x1cc340)) cout<<"NPC::"<<inode << " : "<<__func__<<" : "<< __LINE__<<" M_I_WB way:"<<(unsigned int)way<<endl;
								break;
							}							
							if(i == NPC_WAYS) cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<<" Couldn't find"<<endl;
							
						}
					}else{
						cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<<"ACK received Qsize"<<(unsigned int)BlockedQ.size()<<" address:"<<hex<<progrs_ReQ.address<<dec<<" req_count:"<<progrs_ReQ.req_counter<<endl;
						if(BlockedQ.empty() == true) 
						{
							cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<<"ACK received for non-blocked req"<< endl;
						} else
						{
							ReqFormat front;
							ReqFormat back = BlockedQ.back();
							bool find ;
							find = false;
							do {
								front = BlockedQ.front();			
								if(((front.address >> CL_SIZE) == (progrs_ReQ.address >> CL_SIZE)) && (front.req_counter == progrs_ReQ.req_counter))
								{
									//read the blocked req
									// if the type == RD and ex_ack_count == 1, send resp and delete req
									// if type == WR and sc_ack ==1 ex_ack_count == 1, reduce ack count by 1 
									// if type == WR and ex_ack_count == 1, reduce ack count by 1, send resp and delete req
									// if type == WB with linked req, delete the req and read the next linked req, 
									// if rd/wr, send req and set ex_ack_count
									find = true;
									if(progrs_ReQ.type == ReqFormat::ACK_SMALL)
										front.ex_ack_count = 0x0;
									else
										front.sc_ack_count = 0x0;									
									
									if (((front.type == ReqFormat::ECC_REPLACEMENT) && (front.linked == true)) && ((progrs_ReQ.type == ReqFormat::REPL_REQ_ECC_ACK)) && ((state == SNG_ECC_TRANS))){
										// do a ECC write to the got nodeaddress
										if(progrs_ReQ.ex == true){ // ecc transfer enabled 
											replace_data_dcc_to_npc = true;
											spill_data_dirty_to_npc = front.ex;
											nodeid_to_lsu = progrs_ReQ.redirect_nodeid; 
											address_to_l3 = progrs_ReQ.address;
											req_count_to_l3 = progrs_ReQ.req_counter;
											data_to_l3->write_data(&NPCMem[front.way][set]);
											wait();
											replace_data_dcc_to_npc = false;
											wait();
											cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< "ACK type6"<<endl;
											front.sc_ack_count = 0x1;											
											BlockedQ.pop();
											BlockedQ.push(front);
										} else {
											//cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< "Error: No need of DCC"<<endl; /*implement*/
											BlockedQ.pop(); // this cant be the back in the queue
										front = BlockedQ.front(); //actual req received RD or WR
										cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< "ACK type5"<<endl;
										if((front.type == ReqFormat::RD)) {
											rd_rdx_to_dir(front.address, false,true,NULL,front.req_counter);
											NPCtag[1][set][way] = I_S; //Inv to S
											NPCtag[0][set][way] = (front.address) >> (NPC_SET_COUNT_2 + CL_SIZE);
											front.ex_ack_count = 0x0;
											front.sc_ack_count = 0x1;
											BlockedQ.pop();
											BlockedQ.push(front);
											cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< "ACK type5 RD"<<endl;
										} else if ((front.type == ReqFormat::WR)) {
											rd_rdx_to_dir(front.address, true,true,NULL,front.req_counter);	
											NPCtag[1][set][way] = I_M; //Inv to M
											NPCtag[0][set][way] = (front.address) >> (NPC_SET_COUNT_2 + CL_SIZE);
											front.ex_ack_count = 0x1;
											front.sc_ack_count = 0x1;
											cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< "ACK type5 WR"<<endl;
											BlockedQ.pop();
											BlockedQ.push(front);
										} else {
											cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< " Error!"<<endl;
											BlockedQ.pop();
											BlockedQ.push(front);
										}
										}
									} 
									else if (((front.type == ReqFormat::ECC_REPLACEMENT)) && ((progrs_ReQ.type == ReqFormat::SPILL_DATA_ECC_ACK) || (progrs_ReQ.type == ReqFormat::ACK_FULL)) && ((state == SNG_ECC_TRANS))){
										BlockedQ.pop(); // this cant be the back in the queue
										front = BlockedQ.front(); //actual req received RD or WR
										cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< "ACK type5"<<endl;
										if((front.type == ReqFormat::RD)) {
											rd_rdx_to_dir(front.address, false,true,NULL,front.req_counter);
											NPCtag[1][set][front.way] = I_S; //Inv to S
											NPCtag[0][set][front.way] = (front.address) >> (NPC_SET_COUNT_2 + CL_SIZE);
											front.ex_ack_count = 0x0;
											front.sc_ack_count = 0x1;
											BlockedQ.pop();
											BlockedQ.push(front);
											cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< "ACK type5 RD"<<endl;
										} else if ((front.type == ReqFormat::WR)) {
											rd_rdx_to_dir(front.address, true,true,NULL,front.req_counter);	
											NPCtag[1][set][front.way] = I_M; //Inv to M
											NPCtag[0][set][front.way] = (front.address) >> (NPC_SET_COUNT_2 + CL_SIZE);
											front.ex_ack_count = 0x1;
											front.sc_ack_count = 0x1;
											cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< "ACK type5 WR"<<endl;
											BlockedQ.pop();
											BlockedQ.push(front);
										} else if((front.type == ReqFormat::FLUSH)) {											
											NPCtag[1][set][way] = SHARED; 
											fwd_rd_to_npc(front.redirect_nodeid, front.address,front.req_counter,&NPCMem[way][set]);
											wait();
											req_to_DIR(false, false,false,false,/*bool evict,bool rw,bool ex,bool ce*/front.address,front.req_counter,false, true);	
											cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< "ACK type3 FLUSH"<<endl;
											if(back != front)
												BlockedQ.pop();
											else {
												BlockedQ.pop();
												break;
											}
										} else if ((front.type == ReqFormat::INV)){										
											NPCtag[1][set][way] = INVALID;
											#ifdef INV_4PHASE
											req_to_DIR(false, false,false,false,/*bool evict,bool rw,bool ex,bool ce*/front.address,front.req_counter,false,true);											
											#endif	
											//INV_L1(front.address, front.req_counter);
											LRU_update(false,set,way );
											cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< "ACK type3 INV"<<endl;
											if(back != front)
												BlockedQ.pop();
											else {
												BlockedQ.pop();
												break;
											}
										} else {
											cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< " Error!"<<endl;
											BlockedQ.pop();
											BlockedQ.push(front);
										}
									}
									else if (((front.type == ReqFormat::RD) || (front.type == ReqFormat::WR)) && ((progrs_ReQ.type == ReqFormat::FWD_RD_ACK)||(progrs_ReQ.type == ReqFormat::ACK_FULL)) && ((state == I_S)  || (state == I_M))){ 
										if((front.type == ReqFormat::RD)){
											NPCMem[front.way][set] = progrs_ReQ.data;											
										}
										if(progrs_ReQ.type == ReqFormat::FWD_RD_ACK){
											cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< "ACK type1 FWD data received ackex:"<<(unsigned int)front.ex_ack_count <<" acksc:"<<(unsigned int)front.sc_ack_count << " state: "<< (unsigned int)state<<endl;
										} else { 
											cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< "ACK type1 ackex:"<<(unsigned int)front.ex_ack_count <<" acksc:"<<(unsigned int)front.sc_ack_count << " state: "<< (unsigned int)state<<endl;
										}
										if ((front.ex_ack_count == 0x0) && (front.sc_ack_count == 0x0)) {
											cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< "ACK type11"<<endl;
											NPCtag[0][set][front.way] = (front.address) >> (NPC_SET_COUNT_2 + CL_SIZE); // tag
											LRU_update(true,set,front.way);
											if (((front.type == ReqFormat::RD))&& (front.linked == false) && (state == I_S)) {
												if(( progrs_ReQ.type == ReqFormat::FWD_RD_ACK) && (progrs_ReQ.ex == false)) /*if fwd from NPC*/
													NPCtag[1][set][front.way] = FORWARD;
												else /*: FWD from spilled or memory/l3   */
													NPCtag[1][set][front.way] = FORWARD_SNG;												
												ack_to_L1(false,progrs_ReQ.address, progrs_ReQ.req_counter,&NPCMem[front.way][set]);
												cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< "ACK type12"<<endl;
											}
											else if (((front.type == ReqFormat::RD) ||(front.type == ReqFormat::WR))&& (front.linked == false)&&((state == I_M))) {
												NPCtag[1][set][front.way] = MODIFIED; //modified
												ack_to_L1(true,progrs_ReQ.address, progrs_ReQ.req_counter,NULL);
												NPCMem[front.way][set] = front.data; // new data to be written
												cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< "ACK type13"<<endl;
											} 
											else{
												cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< "Error!! type:"<<(unsigned int)front.type<<" linked:"<<(unsigned int)front.linked <<" state:"<<(unsigned int)state <<endl;
											}	
											if(back != front)
												BlockedQ.pop();
											else {
												BlockedQ.pop();
												break;
											}
										} else {
											BlockedQ.pop();
											BlockedQ.push(front);
										}
										
									} 
									else if(((front.type == ReqFormat::FWD_RD) ||(front.type == ReqFormat::RD) || (front.type == ReqFormat::WR)) && (progrs_ReQ.type == ReqFormat::ACK_SMALL) && ((state == F_I) || (state == I_M) || (state == S_M) || (state == F_M)))
									{ 
										cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< "ACK type2 ackex:"<<(unsigned int)front.ex_ack_count <<" acksc:"<<(unsigned int)front.sc_ack_count << " state: "<< (unsigned int)state<<endl;
										if ((front.ex_ack_count == 0x0) && (front.sc_ack_count == 0x0)) {
											cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< "ACK type21"<<endl;
											if((state == F_I) ){
												if((front.linked == true) && (front.type == ReqFormat::FWD_RD)){
													BlockedQ.pop(); // F to I conversion completed. 
													front = BlockedQ.front();
													if((front.type == ReqFormat::RD)) {
														rd_rdx_to_dir(front.address, false,true,NULL,front.req_counter);
														NPCtag[1][set][front.way] = I_S; //Inv to S
														NPCtag[0][set][front.way] = (front.address) >> (NPC_SET_COUNT_2 + CL_SIZE);
														front.ex_ack_count = 0x0;
														front.sc_ack_count = 0x1;
														BlockedQ.pop();
														BlockedQ.push(front);
													} else if ((front.type == ReqFormat::WR)) {
														rd_rdx_to_dir(front.address, true,true,NULL,front.req_counter);	
														NPCtag[1][set][front.way] = I_M; //Inv to M
														NPCtag[0][set][front.way] = (front.address) >> (NPC_SET_COUNT_2 + CL_SIZE);
														front.ex_ack_count = 0x1;
														front.sc_ack_count = 0x1;
														BlockedQ.pop();
														BlockedQ.push(front);
													} else if (front.type == ReqFormat::SPILL_DATA_ECC) {
														NPCMem[front.way][set] = front.data;
														if(front.ex == true) 
															NPCtag[1][set][front.way] = M_SP; //M_replaced
														else 
															NPCtag[1][set][front.way] = S_SP; //F_sng_replaced
														NPCtag[0][set][front.way] = (front.address) >> (NPC_SET_COUNT_2 + CL_SIZE);	// tag set		
														LRU_update(false,set,front.way); /*CHECK the effect : The spilled data if not reread then thr is no use, so shudnt have v high LRU set*/
														cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<<" for address:"<<hex<< front.address<<dec<<" @ state:" << NPCtag[1][set][front.way]<<endl;
														replace_data_dcc_ack_to_npc = true;
														nodeid_to_lsu = front.redirect_nodeid; 
														address_to_l3 = front.address;
														req_count_to_l3 = front.req_counter;
														wait();
														replace_data_dcc_ack_to_npc = false;
														wait();
														req_to_DIR(false, false,false,false,/*bool evict,bool rw,bool ex,bool ce*/front.address,front.req_counter,false,true);
														wait();
														if(back != front)
															BlockedQ.pop();
														else {
															BlockedQ.pop();
															break;
														}
													} else {
														cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< "Error: unknown type"<<endl;
														BlockedQ.pop();
														BlockedQ.push(front);
													}
												}else {
													cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< "Error: unknown type"<<endl;
													BlockedQ.pop();
													BlockedQ.push(front);													
												}												
											} 
											else {
												NPCtag[0][set][front.way] = (front.address) >> (NPC_SET_COUNT_2 + CL_SIZE) ; // tag
												LRU_update(true,set,front.way );
												NPCtag[1][set][front.way] = MODIFIED; //modified
												NPCMem[front.way][set] = front.data;
												ack_to_L1(true,progrs_ReQ.address, progrs_ReQ.req_counter,NULL);	
												if(back != front)
													BlockedQ.pop();
												else {
													BlockedQ.pop();
													break;
												}
											}
										} 	else {
											BlockedQ.pop();
											BlockedQ.push(front);
										}									
									} 
									else if ((front.type == ReqFormat::WR_BACK) && ((progrs_ReQ.type == ReqFormat::FWD_RD_ACK)||(progrs_ReQ.type == ReqFormat::ACK_FULL)) && ((state == M_I_WB)))
									{ 
										if(front.linked == true){
										BlockedQ.pop(); // this cant be the back in the queue
										if(front.address == 0x7b20) cout<<"NPC::"<<inode << " : "<<__func__<<" : "<< __LINE__<<" M_I_WB reset way:"<<(unsigned int)way<<endl;
										front = BlockedQ.front(); //actual req received RD or WR
										cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< "ACK type3"<<endl;											
										if((front.type == ReqFormat::RD)) {
											rd_rdx_to_dir(front.address, false,true,NULL,front.req_counter);
											NPCtag[1][set][front.way] = I_S; //Inv to S
											NPCtag[0][set][front.way] = (front.address) >> (NPC_SET_COUNT_2 + CL_SIZE);
											front.ex_ack_count = 0x0;
											front.sc_ack_count = 0x1;
											BlockedQ.pop();
											BlockedQ.push(front);
											cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< "ACK type3 RD"<<endl;
										} else if ((front.type == ReqFormat::WR)) {
											rd_rdx_to_dir(front.address, true,true,NULL,front.req_counter);	
											NPCtag[1][set][front.way] = I_M; //Inv to M
											NPCtag[0][set][front.way] = (front.address) >> (NPC_SET_COUNT_2 + CL_SIZE);
											front.ex_ack_count = 0x1;
											front.sc_ack_count = 0x1;
											cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< "ACK type3 WR"<<endl;
											BlockedQ.pop();
											BlockedQ.push(front);
										} else if((front.type == ReqFormat::FLUSH)) {											
											NPCtag[1][set][way] = SHARED; 
											fwd_rd_to_npc(front.redirect_nodeid, front.address,front.req_counter,&NPCMem[way][set]);
											wait();
											req_to_DIR(false, false,false,false,/*bool evict,bool rw,bool ex,bool ce*/front.address,front.req_counter,false, true);	
											cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< "ACK type3 FLUSH"<<endl;
											if(back != front)
												BlockedQ.pop();
											else {
												BlockedQ.pop();
												break;
											}
										} else if ((front.type == ReqFormat::INV)){										
											NPCtag[1][set][way] = INVALID;
											#ifdef INV_4PHASE
											req_to_DIR(false, false,false,false,/*bool evict,bool rw,bool ex,bool ce*/front.address,front.req_counter,false,true);											
											#endif	
											//INV_L1(front.address, front.req_counter);
											LRU_update(false,set,way );
											cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< "ACK type3 INV"<<endl;
											if(back != front)
												BlockedQ.pop();
											else {
												BlockedQ.pop();
												break;
											}
										} 
										else if (front.type == ReqFormat::SPILL_DATA_ECC) {
											NPCMem[front.way][set] = front.data;
											if(front.ex == true) 
												NPCtag[1][set][front.way] = M_SP; //M_replaced
											else 
												NPCtag[1][set][front.way] = S_SP; //F_sng_replaced
											NPCtag[0][set][front.way] = (front.address) >> (NPC_SET_COUNT_2 + CL_SIZE);	// tag set		
											LRU_update(false,set,front.way); /*CHECK the effect : The spilled data if not reread then thr is no use, so shudnt have v high LRU set*/
											replace_data_dcc_ack_to_npc = true;
											nodeid_to_lsu = front.redirect_nodeid; 
											address_to_l3 = front.address;
											req_count_to_l3 = front.req_counter;
											wait();
											replace_data_dcc_ack_to_npc = false;
											wait();
											cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<<" for address:"<<hex<< front.address<<dec<<" @ state:" << NPCtag[1][set][front.way]<<endl;
											req_to_DIR(false, false,false,false,/*bool evict,bool rw,bool ex,bool ce*/front.address,front.req_counter,false,true);
											wait();
											if(back != front)
												BlockedQ.pop();
											else {
												BlockedQ.pop();
												break;
											}
										} 
										else {
											cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< "error: unknown type received @ NPC"<<endl;
											BlockedQ.pop();
											BlockedQ.push(front);
										}
										} 
										else {
											if(front.address == 0x7b20) cout<<"NPC::"<<inode << " : "<<__func__<<" : "<< __LINE__<<" M_I_WB reset way:"<<(unsigned int)way<<endl;
											NPCtag[1][set][way] = INVALID;
											LRU_update(false,set,way );
											if(back != front)
												BlockedQ.pop();
											else {
												BlockedQ.pop();
												break;
											}
										}
									}		
									else {
										//if(((progrs_ReQ.address == 0x7b20) || (progrs_ReQ.address == 0x1cc340)) && (inode == 0xc)) 
										{						
											int set = (progrs_ReQ.address >> CL_SIZE) & ((unsigned int)NPC_SET_COUNT-1); //tag/set/offset , where offset = 5 (8 words in a cl) +  (10 bits of set) 1024*8*8*4 bytes, ttl size of the NPC cache = 256kb 
											int tag = (progrs_ReQ.address) >> (NPC_SET_COUNT_2 + CL_SIZE);
											char i ;
											for(i = way+1;i<NPC_WAYS;i++)
											if((NPCtag[0][set][i] == tag) && (NPCtag[1][set][i] > INVALID) && ((NPCtag[1][set][i] != S_SP) && (NPCtag[1][set][i] != M_SP))) 
											{
												if(i == way) continue;
												hit = true;
												way = (char) i;
												state = (int) NPCtag[1][set][i];
												done = false;
												if((progrs_ReQ.address == 0x7b20) || (progrs_ReQ.address == 0x1cc340)) cout<<"NPC::"<<inode << " : "<<__func__<<" : "<< __LINE__<<" M_I_WB way:"<<(unsigned int)way<<endl;
												break;
											}							
											if(i == NPC_WAYS) cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<<" Couldn't find"<<endl;
											
										} 
										cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<<"Error!! type: "<<(unsigned int)front.type <<" ACKtype: "<<(unsigned int)progrs_ReQ.type<<" state:"<< (unsigned int)state<<endl;
										BlockedQ.pop();
										BlockedQ.push(front);
									}
									cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< "ACK last -1, Qsize:"<<(unsigned int) BlockedQ.size()<<endl;
								} else 
								{
									BlockedQ.pop();
									BlockedQ.push(front);
								}
							} while(back != front);
							cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< "ACK :find : "<< (unsigned int)find<<endl;
						}	
						
					} 					
					req_in_progress = false;
					cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< "ACK done"<<endl;
					}
					}break;
			case ReqFormat::RD:
			case ReqFormat::WR:{
					cout<<flush;
					//if ((state < F_M))
					//	cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<<"RD/WR req received Qsize"<<ReqQ.size()<<endl;
					if ((hit == true) && (state == MODIFIED)) {
						cout<<"NPC::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<"hit M"<<endl;	
						LRU_update(true,set,way );
						if(progrs_ReQ.type == ReqFormat::RD) {
							ack_to_L1(false,progrs_ReQ.address, progrs_ReQ.req_counter,&NPCMem[way][set]);
						}else {
							ack_to_L1(true,progrs_ReQ.address, progrs_ReQ.req_counter,NULL);
						}
						l2_hit_count++;						
					} 
					else if ((hit == true) && ((state == SHARED) || (state == FORWARD) || (state == FORWARD_SNG))) {
						cout<<"NPC::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<"hit S"<<endl;						
						if(progrs_ReQ.type == ReqFormat::RD) {
							LRU_update(true,set,way );
							ack_to_L1(false,progrs_ReQ.address, progrs_ReQ.req_counter,&NPCMem[way][set]);
						}else {
							// start a exclusive perm req
							// block this WR req 
							if (find_in_blockedQ(progrs_ReQ.address) == true){
								ReqQ.push(progrs_ReQ);
							} else {
							cout<<"NPC::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<"hit S write! address:"<<hex<<progrs_ReQ.address <<dec<<" Req_count:" <<progrs_ReQ.req_counter<<endl;	
							l2_hit_count ++;
							rd_rdx_to_dir(progrs_ReQ.address, true,false,NULL,progrs_ReQ.req_counter);
							ReqFormat ReQ;
							ReQ.address = progrs_ReQ.address;
							ReQ.type = ReqFormat::WR;
							ReQ.ex = true;
							ReQ.we = true;
							ReQ.way = way;
							ReQ.ex_ack_count = 0x1;
							ReQ.sc_ack_count = 0x0;
							ReQ.data = progrs_ReQ.data;
							ReQ.linked = false;
							ReQ.req_counter = progrs_ReQ.req_counter;
							BlockedQ.push(ReQ);	
							if(state == SHARED){
								NPCtag[1][set][way] = S_M; //S to M
							} else {
								NPCtag[1][set][way] = F_M; //F to M
							}
							}
						}						
					}
					else if (hit == true) {
					// address in transition state, push the request to back 
						ReqQ.push(progrs_ReQ);
						//cout<<"NPC::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<"hit T"<<endl;	
					}
					else {	// miss case
						int prev_state = 0x0;
						bool status = true;						
						if (find_in_blockedQ(progrs_ReQ.address) == true){
							ReqQ.push(progrs_ReQ);
						} else{
						way = find_free_way(progrs_ReQ.address,progrs_ReQ.req_counter,&prev_state, &status, false );
						if(status == false)
							ReqQ.push(progrs_ReQ);
						else{
							cout<<"NPC::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<"hit Miss"<<endl;
							//cout<<"NPC::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<" step 1"<<endl;
							ReqFormat ReQ;	
							l2_miss_count ++;							
							if((prev_state == MODIFIED) || (prev_state == FORWARD_SNG) || (prev_state == S_SP) || (prev_state == M_SP) || (prev_state == FORWARD)){
								if((prev_state == MODIFIED) || (prev_state == FORWARD_SNG)) { // require ECC spill
									//wait for ack from ECC or L3/MEM then proceed with request
									cout<<"NPC::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<" step 5ECC"<<endl;
								} 
								else if(prev_state == M_SP) {// 
									// wait for WB to complete and then proceed with RD/WR									
									cout<<"NPC::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<" step 2"<<endl;
								} 
								else {//if(prev_state == FORWARD) { // F to I wait 
									//  and then proceed with RD/WR
									cout<<"NPC::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<" step 6"<<endl;			
								}
								ReQ.ex = true;								
								if(progrs_ReQ.type == ReqFormat::WR) {
									ReQ.sc_ack_count = 0x1;
									ReQ.ex_ack_count = 0x1;
								} else {
									ReQ.sc_ack_count = 0x1;
									ReQ.ex_ack_count = 0x0;
								}								
							}							
							else {// SHARED or S_SP or INVALID
							
								//start a read/exclusive read req to dir and then block the request.
								cout<<"NPC::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<" step 3"<<endl;								
								if(ReqFormat::WR == progrs_ReQ.type) {								
									NPCtag[1][set][way] = I_M; // I to M
									rd_rdx_to_dir(progrs_ReQ.address, true,true,NULL,progrs_ReQ.req_counter);
									ReQ.sc_ack_count = 0x1;
									ReQ.ex_ack_count = 0x1;
									cout<<"NPC::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<" step 4X"<<endl;
								} else {
									NPCtag[1][set][way] = I_S; // I to S
									rd_rdx_to_dir(progrs_ReQ.address, false,true,NULL,progrs_ReQ.req_counter);
									ReQ.sc_ack_count = 0x1;
									ReQ.ex_ack_count = 0x0;
									cout<<"NPC::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<" step 4-"<<endl;
								}
								NPCtag[0][set][way] = (progrs_ReQ.address) >> (NPC_SET_COUNT_2 + CL_SIZE);	
								cout<<flush;
								ReQ.ex = progrs_ReQ.ex;
								ReQ.we = progrs_ReQ.we;
							}
							ReQ.address = progrs_ReQ.address;
							ReQ.type = progrs_ReQ.type;
							ReQ.we = progrs_ReQ.we;
							ReQ.way = way;
							ReQ.data = progrs_ReQ.data;
							ReQ.linked = false;
							ReQ.req_counter = progrs_ReQ.req_counter;
							BlockedQ.push(ReQ);
						}							
					}
					}
					//cout<<"NPC::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<"RDWR done"<<endl;	
					req_in_progress = false;
				} break;	
			case ReqFormat::SPILL_RD: {
					hit = table_lookup(progrs_ReQ.address, &way, &state, true);
					if (((hit == true) && ((state == S_SP) || (state == M_SP))) || ((hit == false) && ((state == M_I_WB) || (state == F_I)))){						
						if(progrs_ReQ.redirect_nodeid != inode) {
							/*needs to invalidate this*/
							if(state == S_SP) {
								NPCtag[1][set][way] = INVALID;
								LRU_update(false,set,way);
							} else if (state == M_SP) {
								/* WB to memory and do nothing*/
								write_to_l3(progrs_ReQ.address,&NPCMem[way][set], true,progrs_ReQ.req_counter+1,way, false, inode, false, true);
								if((progrs_ReQ.address == 0x7b20) || (progrs_ReQ.address == 0x1cc340)) cout<<"NPC::"<<inode << " : "<<__func__<<" : "<< __LINE__<<" M_I_WB set way:"<<(unsigned int)way<<endl;
								NPCtag[1][set][way] = M_I_WB;
							}
							cout<<"NPC::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<endl;
							wait();
							forward_spill_data_to_npc = true;
							fwd_rd_to_npc(progrs_ReQ.redirect_nodeid, progrs_ReQ.address,progrs_ReQ.req_counter,&NPCMem[way][set]);
							forward_spill_data_to_npc = false;						
							wait();
							req_to_DIR(false, false,false,false,/*bool evict,bool rw,bool ex,bool ce*/progrs_ReQ.address,progrs_ReQ.req_counter,false,true);
							wait();		
					    } else {
							req_to_DIR(false, false,false,false,/*bool evict,bool rw,bool ex,bool ce*/progrs_ReQ.address,progrs_ReQ.req_counter,false,true);
							wait();
							/*needs to invalidate this*/
							if(state == S_SP) {
								NPCtag[1][set][way] = INVALID;
								LRU_update(false,set,way);
							} else if (state == M_SP) {
								/* WB to memory and do nothing*/
								write_to_l3(progrs_ReQ.address,&NPCMem[way][set], true,progrs_ReQ.req_counter+1,way, false, inode, false, true);
								if((progrs_ReQ.address == 0x7b20) || (progrs_ReQ.address == 0x1cc340)) cout<<"NPC::"<<inode << " : "<<__func__<<" : "<< __LINE__<<" M_I_WB set way:"<<(unsigned int)way<<endl;
								NPCtag[1][set][way] = M_I_WB;
								NPCtag[0][set][way] = (progrs_ReQ.address) >> (NPC_SET_COUNT_2 + CL_SIZE); // tag
							}
							char way_prev = way;
							int state_prev = state;
							hit = table_lookup(progrs_ReQ.address, &way, &state, false);
							if(BlockedQ.empty() == true) 
							{
								cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<<" Error! ACK received for non-blocked req"<< endl;
							} else
							{
								ReqFormat front;
								ReqFormat back = BlockedQ.back();
								bool find ;
								find = false;
								do {
									front = BlockedQ.front();			
									if(((front.address >> CL_SIZE) == (progrs_ReQ.address >> CL_SIZE)) && (front.req_counter == progrs_ReQ.req_counter) && (((front.type == ReqFormat::RD) || (front.type == ReqFormat::WR)) && ((state == I_S)  || (state == I_M))))
									{										
										front.sc_ack_count = 0x0;
										if((progrs_ReQ.address == 0x7b20) || (progrs_ReQ.address == 0x1cc340)) cout<<"NPC::"<<inode << " : "<<__func__<<" : "<< __LINE__<<" M_I_WB set way:"<<(unsigned int)way<<endl;
										if((front.type == ReqFormat::RD)){
											NPCMem[way][set] = progrs_ReQ.data;											
										}
										if ((front.ex_ack_count == 0x0) && (front.sc_ack_count == 0x0)) {
											cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< "ACK type11"<<endl;
											NPCtag[0][set][way] = (front.address) >> (NPC_SET_COUNT_2 + CL_SIZE); // tag
											LRU_update(true,set,way);
											if (((front.type == ReqFormat::RD))&& (front.linked == false) && (state == I_S)) {
												if(( progrs_ReQ.type == ReqFormat::FWD_RD_ACK) && (progrs_ReQ.ex == false)) /*if fwd from NPC*/
													NPCtag[1][set][way] = FORWARD;
												else /*: FWD from spilled or memory/l3   */
													NPCtag[1][set][way] = FORWARD_SNG;												
												ack_to_L1(false,progrs_ReQ.address, progrs_ReQ.req_counter,&NPCMem[way][set]);
												cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< "ACK type12"<<endl;
											}
											else if (((front.type == ReqFormat::RD) ||(front.type == ReqFormat::WR))&& (front.linked == false)&&((state == I_M))) {
												NPCtag[1][set][way] = MODIFIED; //modified
												ack_to_L1(true,progrs_ReQ.address, progrs_ReQ.req_counter,NULL);
												NPCMem[way][set] = front.data; // new data to be written
												cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< "ACK type13"<<endl;
											} 
											else{
												cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< "Error!! type:"<<(unsigned int)front.type<<" linked:"<<(unsigned int)front.linked <<" state:"<<(unsigned int)state <<endl;
											}
											find = true;
											if(back != front)
												BlockedQ.pop();
											else {
												BlockedQ.pop();
												break;
											}
										} else {
											find = true;
											BlockedQ.pop();
											BlockedQ.push(front);
										}
									} else {
										BlockedQ.pop();
										BlockedQ.push(front);
									}
								} while (front != back);
								if(find == false) {
									cout<<"NPC::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<" Error! state:"<<(unsigned int)state<< endl;
								}
							}
						}
					} else {
						cout<<"NPC::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<" Error! state:"<<(unsigned int)state<< endl;
					}
					} break;
			case ReqFormat::FWD_RD: {
					if ((hit == true) && ((state == FORWARD) || (state == FORWARD_SNG) || (state == F_M)||(state == F_I_SNG) || (state == F_I))) { // in forward state
						cout<<"NPC::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<" FWD state:"<< (unsigned int)state<<endl;						
						fwd_rd_to_npc(progrs_ReQ.redirect_nodeid, progrs_ReQ.address,progrs_ReQ.req_counter,&NPCMem[way][set]);
						req_to_DIR(false, false,false,false,/*bool evict,bool rw,bool ex,bool ce*/progrs_ReQ.address,progrs_ReQ.req_counter,false,true);
						wait();
						if((state == FORWARD) || (state == FORWARD_SNG)){
							NPCtag[1][set][way] = SHARED; // read npc will be having the state F and sent cl will change to state S
						} else if ((state == F_I) || (state == F_I_SNG)) { /* F to I , as data is forwarded and dir stored forwarder is changed, now no ack is gonna be received at npc assume as if F to I ack is received. */
							if(BlockedQ.empty() == false){
								ReqFormat front;
								ReqFormat back = BlockedQ.back();
								do {
									if((front.linked == true) && (front.type == ReqFormat::FWD_RD) && (progrs_ReQ.address == front.address)){
										BlockedQ.pop(); // F to I conversion completed. 
										front = BlockedQ.front();
										if((front.type == ReqFormat::RD)) {
											rd_rdx_to_dir(front.address, false,true,NULL,front.req_counter);
											NPCtag[1][set][front.way] = I_S; //Inv to S
											NPCtag[0][set][front.way] = (front.address) >> (NPC_SET_COUNT_2 + CL_SIZE);
											front.ex_ack_count = 0x0;
											front.sc_ack_count = 0x1;
											BlockedQ.pop();
											BlockedQ.push(front);
										} else if ((front.type == ReqFormat::WR)) {
											rd_rdx_to_dir(front.address, true,true,NULL,front.req_counter);	
											NPCtag[1][set][front.way] = I_M; //Inv to M
											NPCtag[0][set][front.way] = (front.address) >> (NPC_SET_COUNT_2 + CL_SIZE);
											front.ex_ack_count = 0x1;
											front.sc_ack_count = 0x1;
											BlockedQ.pop();
											BlockedQ.push(front);
										} else {
											cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< "Error: unknown type"<<endl;
											BlockedQ.pop();
											BlockedQ.push(front);
										}
									}else {
										cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< "Error: unknown type"<<endl;
										BlockedQ.pop();
										BlockedQ.push(front);													
									}
								} while (back != front);
							} else {
								cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< "Error: unknown type"<<endl;
							}
						} else {
							//NPCtag[1][set][way] = state; // F to M , acks expectation is same for NPC
						}
					} else if ((state > M_SP)) { // trans
						ReqQ.push(progrs_ReQ);
					} else  { //M S or I cant forward data
						cout<<"NPC::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<" FWD error! state:"<<state<<endl;	
					}
					req_in_progress = false;
					//cout<<"NPC::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<" FWD state:"<< (unsigned int)state<<endl;	
				} break;
			case ReqFormat::SPILL_DATA_ECC: {
					bool status = true;	
					int prev_state = 0x0;					
					way = find_free_way(progrs_ReQ.address,progrs_ReQ.req_counter,&prev_state, &status, true);
					if(status == false) {
						//spill over spill protection 
						//send to memory and nothing more to be done!
						write_to_l3(progrs_ReQ.address,&progrs_ReQ.data, false,progrs_ReQ.req_counter, 0x0, false, progrs_ReQ.redirect_nodeid, false, false);
					} 
					else {
						if((prev_state == M_SP) || (prev_state == FORWARD) || (prev_state == S_SP)) {
							//wait for WR_ACK or FWD_evict_ack from dir then process the spill req							
							ReqFormat ReQ;							
							cout<<"NPC::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<" wait for wb/Fevt_ack"<<endl;
							if(prev_state == M_SP) {
								ReQ.sc_ack_count = 0x1;
								ReQ.ex_ack_count = 0x0;
							} else {
								ReQ.sc_ack_count = 0x0;
								ReQ.ex_ack_count = 0x1;
							}
							ReQ.ex = progrs_ReQ.ex;			
							ReQ.address = progrs_ReQ.address;
							ReQ.type = ReqFormat::SPILL_DATA_ECC;
							ReQ.we = progrs_ReQ.we;
							ReQ.way = way;
							ReQ.data = progrs_ReQ.data;
							ReQ.linked = false;
							ReQ.redirect_nodeid = progrs_ReQ.redirect_nodeid;
							ReQ.req_counter = progrs_ReQ.req_counter;
							BlockedQ.push(ReQ);
						}
						else {						
						// replace + send ack to both npc and dir
						NPCMem[way][set] = progrs_ReQ.data;
						if(progrs_ReQ.ex == true) 
							NPCtag[1][set][way] = M_SP; //M_replaced
						else 
							NPCtag[1][set][way] = S_SP; //F_sng_replaced
						NPCtag[0][set][way] = (progrs_ReQ.address) >> (NPC_SET_COUNT_2 + CL_SIZE);	// tag set		
						LRU_update(false,set,way); /*TODO CHECK the effect : The spilled data if not reread then thr is no use, so shudnt have v high LRU set*/						
						cout<<"NPC::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<" send reply to "<<progrs_ReQ.redirect_nodeid<< " for address:"<<hex<< progrs_ReQ.address<<dec<<" @ state:" << NPCtag[1][set][way]<<endl;
						replace_data_dcc_ack_to_npc = true;
						nodeid_to_lsu = progrs_ReQ.redirect_nodeid; 
						address_to_l3 = progrs_ReQ.address;
						req_count_to_l3 = progrs_ReQ.req_counter;
						wait();
						replace_data_dcc_ack_to_npc = false;
						wait();
						req_to_DIR(false, false,false,false,/*bool evict,bool rw,bool ex,bool ce*/progrs_ReQ.address,progrs_ReQ.req_counter,false,true);
						wait();
						}
					}
				} break;
			default: 
					cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< ": Error: NPC::main_thread !!" <<endl; wait();
		}	
	wait();
	//cout<<inode<<" : "<<"NPC::"<<__func__<<" : -- "<< __LINE__<<endl;	
	}
}


