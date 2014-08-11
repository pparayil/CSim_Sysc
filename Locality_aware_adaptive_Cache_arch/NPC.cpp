
#include "NPC.h"

#define LOOKUP_DELAY_NPC 1

// NPCMEM needs to be 4 ways * number of sets


void NPC::NPC_init(void){
	int i=NPC_WAYS;
	//cout<<"NPC::"<<__func__<<" : "<< __LINE__<<endl;
	for (int i = 0; i< NPC_WAYS; i++){
	NPCMem[i] =  new datatype [NPC_SET_COUNT];
	memset(NPCMem[i], 0x0,NPC_SET_COUNT);
	for(int j=0;j<NPC_SET_COUNT;j++)
		{
		//NPCtag[4][j][i] = 0x0; // timestamp
		NPCtag[3][j][i] = 0x0; // utilization counter 
		NPCtag[2][j][i] = 0x0; // LRU
		NPCtag[1][j][i] = INVALID; // state
		NPCtag[0][j][i] = 0x0; // tag
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
				if((inode == 0xc) && ((front.address >> CL_SIZE) == 0x1200))
					cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< " Order altered may b"<<endl;
			} 
			BlockedQ.pop();
			BlockedQ.push(front);
		} while(back != front);
	}
	return found;
}
bool NPC::table_lookup(unsigned int address, char * hit_way, int * state){
	 
	// 8 sets of 8 way associative 8 word cl = 5 bit offset + 10 bit set  + 3 bit in each way for lru = ttl 256kb **** as of now 32sets => 5 bit
	//NPCtag[0][set][way] = tag, NPCtag[1][set][way] = 2 bit state I 00 /V 01/M 10, NPCtag[2][set][way] = 3 bit lru */ 
	bool hit = false;
	if((hit_way == NULL) || (state == NULL)) {
		cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< " error: way not allocated "<<endl;
	}
	int set = (address >> CL_SIZE) & ((unsigned int)NPC_SET_COUNT-1); //tag/set/offset , where offset = 5 (8 words in a cl) +  (10 bits of set) 1024*8*8*4 bytes, ttl size of the NPC cache = 256kb 
	int tag = (address) >> (NPC_SET_COUNT_2 + CL_SIZE);
	for(char i=0; i<NPC_WAYS; i++)
	{
		if((NPCtag[0][set][i] == tag) && ((NPCtag[1][set][i] > INVALID))) // 3 bit state I 00 /V 01/M 10 // transition states: 100 : INV - SHARED (RD) , 101 : Shared -INV(INV simple), 110: inv to M (exRD+WR) , 111: M to INV (INV with WB), 011: Shared to M
		{
			hit = true;
			*hit_way = (char) i;
			*state = (int) NPCtag[1][set][i];			
			break;
		} else if((NPCtag[0][set][i] == tag) && ((NPCtag[1][set][i] == INVALID)))	{
			*state = (int) NPCtag[1][set][i];
		}		
	}

	return hit;
}

void NPC::LRU_uliz_update(bool down_up, unsigned int set,unsigned int way, bool util) {
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
	
	if(util == true) {
		NPCtag[3][set][way] = NPCtag[3][set][way] + 1;
		//NPCtag[4][set][way] = (unsigned int) sc_time_stamp().to_double()/(100000); // 100 ns accuracy
	}
}
/*
unsigned int NPC::min_access_time(unsigned int address) {
	int set = (address >> CL_SIZE) & ((unsigned int)NPC_SET_COUNT-1); 
	unsigned int min = 0x0;
	for(char j=0; j<NPC_WAYS; j++)
	{
		if(min > NPCtag[4][set][j])
			min = NPCtag[4][set][j];
	}
	return min;
}*/

void NPC::inv_l1(unsigned int address,unsigned int req_count){
	inv_to_l1 = true;
	address_to_l1 = address;
	req_count_to_l1 = req_count;
	wait();
	inv_to_l1 = false;
	wait();
}

void NPC::ACK_L1(unsigned int address,unsigned int req_count,bool status, bool we,datatype * data, bool rm) {
	address_to_l1 = address;
	req_count_to_l1 = req_count;
	ack_to_l1 = true;
	mode_pri_remote_to_l1 = rm;
	if(we == false) {
		data_to_l1->write_data(data);
		ack_we_to_l1 = false;
	}else {		
		ack_we_to_l1 = true;
	}
	wait();
	ack_to_l1 = false;
	wait();
}
void NPC::write_to_llc (unsigned int address, datatype * DATA, bool evict, int req_count, int way, bool req_to_directory, bool req_to_llc, unsigned int utilz){
	//cout<<"NPC::"<<inode << " : "<<__func__<<" : "<< __LINE__<<endl;	
	if(req_to_directory == true)
	{
		cl_evt_info_to_dir = evict;
		rd_wr_to_dir = true;
		ex_to_dir = false;
		ce_to_dir = true;
		address_to_dir = address;
		req_count_to_dir = req_count;
		if(evict == true)
			utiliz_to_dir = utilz;
		wait();
		cl_evt_info_to_dir = false;
		rd_wr_to_dir = false;
		ex_to_dir = false;
		ce_to_dir = false;
		wait();
	} else {
		//cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<<endl;
		generate_wr_req_npc = req_to_llc;
		address_to_llc_en = true;
		rd_wr_to_dir = evict;
		address_to_llc = address;
		req_count_to_llc = req_count;
		wait();
		generate_wr_req_npc = false;
		address_to_llc_en = false;
		rd_wr_to_dir = false;
		wait();
	}
	address_to_llc_en = true;
	address_to_llc = address;
	req_count_to_llc = req_count;
	if(DATA != NULL)
		data_to_llc->write_data(DATA);
	wait();
	address_to_llc_en = false;
	ReqFormat ReQ;
	ReQ.address = address;
	ReQ.type = ReqFormat::WR_BACK;
	ReQ.ex = false;
	ReQ.we = true;
	ReQ.way = way;
	ReQ.ex_ack_count = 0x0;
	ReQ.sc_ack_count = 0x1;
	ReQ.data = *DATA;
	ReQ.linked = true;
	ReQ.req_counter = req_count;
	BlockedQ.push(ReQ);
	wait();
	
	//cout<<"NPC::"<<inode << " : "<<__func__<<" : "<< __LINE__<<endl;
}

char NPC::find_free_way (unsigned int address,int req_count,bool *wb_enable, bool *status) {
	int set = (address >> CL_SIZE) & ((unsigned int)NPC_SET_COUNT-1); 
	int tag = (address) >> (NPC_SET_COUNT_2 + CL_SIZE);
	char j = 0;	
	unsigned int LRUvalue = 0x0;
	bool found;
	//cout<<"NPC::"<<inode << " : "<<__func__<<" : "<< __LINE__<<endl;	
	*wb_enable =  false;
	found = false;
	for(; LRUvalue<NPC_WAYS; LRUvalue++) {
		for(; j<NPC_WAYS; j++)
		{
			if (NPCtag[2][set][j] == LRUvalue)
			{
				if (NPCtag[1][set][j] == MODIFIED) // Modified
				{
					int temp_new_address = (NPCtag[0][set][j]<<(NPC_SET_COUNT_2 + CL_SIZE)) | ((set & ((unsigned int)NPC_SET_COUNT-1))<<(CL_SIZE)) | 0b00000;
					cout<<"NPC::"<<inode << " : "<<__func__<<" : "<< __LINE__<<" Modified state, tag:"<<hex<<NPCtag[0][set][j] <<" new addr:"<<temp_new_address <<" set:"<<set<<dec<<endl;
					write_to_llc(temp_new_address,&NPCMem[j][set], true,req_count,j, true, false, NPCtag[3][set][j]); // eviction WBs
					NPCtag[1][set][j] = M_I_WB; // M to I 
					*wb_enable =  true;				
					//cout<<"NPC::"<<inode << " : "<<__func__<<" : "<< __LINE__<<endl;
					found = true;
				}
				else
				if (NPCtag[1][set][j] == SHARED) //valid
				{
					//cout<<"NPC::"<<inode << " : "<<__func__<<" : "<< __LINE__<<" In shared state"<<endl;		
					int existingaddr = (NPCtag[0][set][j]<<(NPC_SET_COUNT_2 + CL_SIZE)) | ((set & ((unsigned int)NPC_SET_COUNT-1))<<(CL_SIZE)) | 0b00000;
					inv_l1(existingaddr, req_count);
					address_to_dir = existingaddr;
					utiliz_to_dir = NPCtag[3][set][j];
					req_count_to_dir = req_count;
					cl_evt_info_to_dir = true; // directory will remove the node id from the sharers list.
					ce_to_dir = false;
					wait();
					cl_evt_info_to_dir = false;
					wait();
					NPCtag[1][set][j] = INVALID;
					found = true;
					
					//cout<<"NPC::"<<inode << " : "<<__func__<<" : "<< __LINE__<<endl;
				}
				else if(NPCtag[1][set][j] > MODIFIED) // trans state dont remove this /*either wait for this to get proper state (probable deadlock) or take LRU number 1*/
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
		if(j == NPC_WAYS)  {
			//cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<<" Error No way with LRU 0!!"<<endl;
			j = 0;	
		} else 
			break;
	}
	if(found == false) {
		//cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<<" Error No way with LRU minimum and without trans!!"<<endl;
		*status = false;
	}
	return j;
}

bool NPC::rd_rdx_to_dir (unsigned int address, bool Exclusive, bool refill,int req_count){
	//cout<<"NPC::"<<inode << " : "<<__func__<<" : "<< __LINE__<<endl;
	rd_wr_to_dir = false;
	ce_to_dir = refill;
	ex_to_dir = Exclusive;
	cl_evt_info_to_dir = false;
	address_to_dir = address;
	req_count_to_dir = req_count;
	/*if(refill == true) {
		min_time_stamp_to_dir = min_access_time(address);
	} */
	wait();
	ce_to_dir = false;
	ex_to_dir = false;
	wait();
}

bool NPC::inv_ack_dir (unsigned int address, int req_count, int way){
	//cout<<"NPC::"<<inode << " : "<<__func__<<" : "<< __LINE__<<endl;
	int set = (address >> CL_SIZE) & ((unsigned int)NPC_SET_COUNT-1); 
	inv_ack_to_dir = true;
	utiliz_to_dir = NPCtag[3][set][way];
	address_to_dir = address;
	req_count_to_dir = req_count;
	wait();
	inv_ack_to_dir = false;
	wait();
}

bool NPC::invalidating_cl (unsigned int address ,int way, int req_count, unsigned int state, ReqFormat::ReQType tp)
{
	int set = (address >> CL_SIZE) & ((unsigned int)NPC_SET_COUNT-1); 
	int tag = (address) >> (NPC_SET_COUNT_2 + CL_SIZE);
	bool ret = true;
	//cout<<"NPC::"<<inode << " : "<<__func__<<" : "<< __LINE__<<endl;
	if (state == SHARED) {//valid
		//cout<<"NPC::"<<inode << " : "<<__func__<<" : "<< __LINE__<<"invalidate shared"<<endl;
		if(tp == ReqFormat::INV){ // Shared state
			NPCtag[1][set][way] = INVALID;
			LRU_uliz_update(false,set,way,false);
			inv_l1(address, req_count);
		} 
		else if (tp == ReqFormat::FLUSH) {
			NPCtag[1][set][way] = SHARED; 
		} 
		else {
			cout<<"NPC::"<<inode << " : "<<__func__<<" : "<< __LINE__<<"Error Unknown state!"<<endl;
			return true;
		}
		#ifdef INV_4PHASE		
		inv_ack_dir(address,req_count,way);
		wait();
		#endif	
		ret = true;
	} 
	else if (state ==  MODIFIED) // Modified, hence require WB 
	{
		cout<<"NPC::"<<inode << " : "<<__func__<<" : "<< __LINE__<<" : invalidate Modified"<<endl;
		
		ReqFormat ReQ;
		if(tp == ReqFormat::FLUSH) {
			write_to_llc(address,&NPCMem[way][set], true,req_count, way, false, true, NPCtag[3][set][way]); //dir already knws the status of cache
			ReQ.type = ReqFormat::FLUSH;
			NPCtag[1][set][way] = M_I_WB; // M to I + flush = M to S 
		} 
		else {
			write_to_llc(address,&NPCMem[way][set], true,req_count, way, false, true, NPCtag[3][set][way]); 
			ReQ.type = ReqFormat::INV;
			NPCtag[1][set][way] = M_I_WB; // M to I
			//inv_l1(address, req_count);
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
	else if (state > MODIFIED) //transition state
	{
		//search blockedQ
		if(BlockedQ.empty() == true) 
		{
			NPCtag[1][set][way] = INVALID;
			cout<<"NPC::"<<inode << " : "<<__func__<<" : "<< __LINE__<<" INV 1"<<endl;
			inv_ack_dir(address,req_count,way);
		} else
		{
				ReqFormat front,fronttemp;
				ReqFormat back = BlockedQ.back();
				do {
					front = BlockedQ.front();					
					if(((front.address >> CL_SIZE) == (address >> CL_SIZE))  && ((front.type != ReqFormat::WR_BACK) && (front.type != ReqFormat::INV) && (front.type != ReqFormat::FLUSH)))
					{
						inv_l1(front.address, req_count);	
						if((tp != ReqFormat::FLUSH)) {
							NPCtag[1][set][way] = INVALID;		
							cout<<"NPC::"<<inode << " : "<<__func__<<" : "<< __LINE__<<" INV 2"<<endl;
							#ifdef INV_4PHASE
							inv_ack_dir(address,req_count,way);
							#endif
							LRU_uliz_update(false,set,way,false);
							ret = true;
							if(back != front)
								BlockedQ.pop();
							else {
								BlockedQ.pop();
								break;
							}
						} 
						else if((tp == ReqFormat::FLUSH) && ((NPCtag[1][set][way] == S_M))){											
							inv_ack_dir(address,req_count,way);
							cout<<"NPC::"<<inode << " : "<<__func__<<" : "<< __LINE__<<" INV 3"<<endl;
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
					else if (((front.address >> CL_SIZE) == (address >> CL_SIZE)) && (front.linked == true) && ((front.type == ReqFormat::WR_BACK))) 
					{						
						if(back != front)
							BlockedQ.pop();
						else {
							cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< "Error!!"<<endl;
							BlockedQ.pop();
							break;
						}
						fronttemp = BlockedQ.front();
						if((fronttemp.type == ReqFormat::WR) || (fronttemp.type == ReqFormat::RD)){
							if(front.type == ReqFormat::WR_BACK) {
								generate_wr_req_npc = true;
								address_to_llc_en = true;
								address_to_llc = address;
								req_count_to_llc = front.req_counter;// need to find proper req_count 
								wait();
								generate_wr_req_npc = false;
								address_to_llc_en = false;
								wait();
							}
							inv_l1(fronttemp.address, fronttemp.req_counter);	
							cout<<"NPC::"<<inode << " : "<<__func__<<" : "<< __LINE__<<" Invalidated due INV/FLUSH command"<<endl;
							fronttemp.type = tp;
							fronttemp.address = address;
							fronttemp.req_counter = req_count;
							fronttemp.we = false;
							BlockedQ.push(front);
							BlockedQ.pop();
							BlockedQ.push(fronttemp);
							ret = true;
							break;
						}
						else if((fronttemp.type == ReqFormat::INV) || (fronttemp.type == ReqFormat::FLUSH)){
							// nothing to do as there two reqs received for the same action take the last one and delete the first
							if(tp != ReqFormat::FLUSH)
								fronttemp.type = ReqFormat::INV;
							else  {
								fronttemp.type = ReqFormat::FLUSH;
							}
							BlockedQ.push(front);
							BlockedQ.pop();
							BlockedQ.push(fronttemp);
							ret = true;
							break;
						} 
						else {
							cout<<"NPC::"<<inode << " : "<<__func__<<" : "<< __LINE__<<" Error! type :"<<(unsigned int) fronttemp.type<<endl; //wait
							BlockedQ.push(front);
							BlockedQ.pop();
							BlockedQ.push(fronttemp);
							ret = false;
						}											
					}
					else if(((front.address >> CL_SIZE) == (address >> CL_SIZE))  &&((front.type == ReqFormat::INV) || (front.type == ReqFormat::FLUSH))) {
						// nothing to do as there two reqs received for the same action take the last one and delete the first
						front.type = tp;
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
			}		
	} 
	else {
			NPCtag[1][set][way] = INVALID;	
			inv_l1(address,req_count);
			cout<<"NPC::"<<inode << " : "<<__func__<<" : "<< __LINE__<<" Error! address:"<<hex<<address<<dec<<endl; //wait
			#ifdef INV_4PHASE
			inv_ack_dir(address,req_count,way);
			#endif
			LRU_uliz_update(false,set,way,false);
	}
	return ret;	
}

void NPC::request_collector(void){
	//cout<<"NPC::"<< inode << " : "<<__func__<<" : "<< __LINE__<<dec<<endl;	
	enum state {IDLE, INV, RD_WR, ACK_ANY} st,st1;
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
					ack_noc = ack_from_dir | ack_from_llc;
					if((ce_from_l1 == true) || (inv_frm_dir == true) || (flush_frm_npc_dir == true) || ((ack_noc == true))){
						//cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<<"ack/inv with hit or ce"<< endl;
						if ((ce_from_l1 == true) && ((inv_frm_dir == true) || (flush_frm_npc_dir == true)  || (ack_noc == true)))
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
						else if(ce_from_l1 == true){
							st = RD_WR;
							st1 = IDLE;
						}
						else
						{
							cout<<inode<<" : "<<"NPC::"<<__func__<<" : "<< __LINE__<<" Error!!"<<endl;
							st = IDLE;
							st1 = IDLE;
							wait(); 
						}							
						
					} 
					else
					{
						st = IDLE;
						st1 = IDLE;
						wait(); 
					}					
					} break;
		case INV: {
					ReqFormat ReQ1;
					if(flush_frm_npc_dir == true) {
						ReQ1.address = inv_addr_from_dir;
						ReQ1.req_counter = req_count_from_dir; //((inode<<3) | 0x0);
						//ReQ.way = way;
						ReQ1.type = ReqFormat::FLUSH;
						cout<<"NPC::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<" FLUSH for address:0x"<<hex<<inv_addr_from_dir<<dec<<endl;	
					}
					else{
						ReQ1.address = inv_addr_from_dir;
						ReQ1.req_counter = req_count_from_dir; //((inode<<3) | 0x0);
						//ReQ.way = way;
						ReQ1.type = ReqFormat::INV;
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
		case ACK_ANY:{
					ReqFormat ReQ3;
					if((ack_from_dir == true)) {
						ReQ3.type = ReqFormat::ACK_SMALL;
						ReQ3.address = address_from_dir;
						ReQ3.req_counter = req_count_from_dir; //((inode<<3) | 0x0);
						ReQ3.mode_pri_remote = mode_pri_remote_from_dir;
						cout<<"NPC::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<" ACK from dir"<<endl;
					}
					else if (ack_from_llc == true) {
						ReQ3.type = ReqFormat::ACK_FULL;
						ReQ3.address = address_from_llc;
						ReQ3.req_counter = req_count_from_llc; //((inode<<3) | 0x0);
						ReQ3.mode_pri_remote = mode_pri_remote_from_llc;
						data_from_llc->read_data(&ReQ3.data);
						cout<<"NPC::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<" ACK from MEM/L3"<<endl;
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
		hit = table_lookup(progrs_ReQ.address, &way, &state);
		set = (progrs_ReQ.address >> CL_SIZE) & ((unsigned int)NPC_SET_COUNT-1);
		sc_time start = sc_time_stamp();
		switch (progrs_ReQ.type) {
			case ReqFormat::FLUSH:
			case ReqFormat::INV: {
					bool ret = false;
					//cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<<"INV/FLUSH received"<<endl;
					if (hit == true) {
						ret = invalidating_cl(progrs_ReQ.address,way,progrs_ReQ.req_counter, state,progrs_ReQ.type); // simple
					} else {	// hit false so already invalidated 
						if(ReqFormat::FLUSH == progrs_ReQ.type) {
							cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<<"Error: FLUSH invalid CL? "<<endl;							
						} 
						ret = true;
						#ifdef INV_4PHASE
						wait();
						inv_ack_dir(progrs_ReQ.address,progrs_ReQ.req_counter,way);
						#endif	
					}
					if (ret == false) {
						ReqQ.push(progrs_ReQ);
					}
					req_in_progress = false;					
				} break;
			case ReqFormat::ACK_SMALL:
			case ReqFormat::ACK_FULL:{
					cout<<flush;	
					if(hit == false) // hit miss rd/rdx/wr request 
					{
						if(BlockedQ.empty() == true) 
						{
							cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<<"ACK received for non-blocked req"<< endl;
						} else
						{
							ReqFormat front;
							bool wb_enable = true;
							bool status = true;
							ReqFormat back = BlockedQ.back();
							do {
								front = BlockedQ.front();			
								if(((front.address >> CL_SIZE) == (progrs_ReQ.address >> CL_SIZE)) && (front.req_counter == progrs_ReQ.req_counter))
								{
									if(progrs_ReQ.type == ReqFormat::ACK_SMALL)
										front.ex_ack_count = 0x0;
									else
										front.sc_ack_count = 0x0;
									
									if((progrs_ReQ.type == ReqFormat::ACK_FULL) && (front.ex_ack_count != 0x0)) {
										front.mode_pri_remote = progrs_ReQ.mode_pri_remote;
										front.data = progrs_ReQ.data;
										cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< endl;
										BlockedQ.pop();
										BlockedQ.push(front);
									} 
									else if ((progrs_ReQ.type == ReqFormat::ACK_SMALL) && (front.sc_ack_count != 0x0)){
										front.mode_pri_remote = progrs_ReQ.mode_pri_remote;
										if(front.mode_pri_remote == true) {// remote mode
											BlockedQ.pop();
											cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<<" address:"<<hex<<front.address<<dec<<endl;
											/* @ ACK small with remote mode, wr data sent to llc, wait for ack */																						
											write_to_llc(front.address,&front.data, false,front.req_counter, 0x0, false, true, 0x0);
											front.type = ReqFormat::ACKL1_WB;
											front.req_counter = front.req_counter+4;
											BlockedQ.push(front);
											break;
										} else {
											cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< endl;
											BlockedQ.pop();
											BlockedQ.push(front);
										}
									} 
									else {
										if ((((front.type == ReqFormat::WR)) && (progrs_ReQ.type == ReqFormat::ACK_SMALL)) || (((front.type == ReqFormat::RD) || (front.type == ReqFormat::WR)) && (progrs_ReQ.type == ReqFormat::ACK_FULL))){ 
											cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< "ACK typeN1 "<<endl;
											if(progrs_ReQ.mode_pri_remote == false) {
												//find_free_way 
												front.mode_pri_remote = progrs_ReQ.mode_pri_remote;
												if((progrs_ReQ.type == ReqFormat::ACK_FULL) && (front.type == ReqFormat::RD))
													front.data = progrs_ReQ.data;												
												way = find_free_way(progrs_ReQ.address,progrs_ReQ.req_counter,&wb_enable, &status);
												if(status == false) {
													ReqQ.push(progrs_ReQ); 
													BlockedQ.pop();
													BlockedQ.push(front);
												} else{
													cout<<"NPC::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<" step 1 wb:"<<wb_enable<<endl;
													if(wb_enable == true) {
														// wait for WB to complete and then proceed with RD/WR													
														front.mode_pri_remote = false;
														front.way = way;
														front.sc_ack_count = 0x0;
														front.ex_ack_count = 0x0;
														front.linked = false;
														cout<<"NPC::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<" step 2"<<endl;
														BlockedQ.pop();
														BlockedQ.push(front);			
													}
													else {
														// NPC read data acknowledge back to Proc. & save inside L1													
														NPCMem[way][set] = front.data;
														NPCtag[0][set][way] = (progrs_ReQ.address) >> (NPC_SET_COUNT_2 + CL_SIZE); 
														LRU_uliz_update(true,set,way,true);
														if(front.type == ReqFormat::WR){
															NPCtag[1][set][way] = MODIFIED;
															NPCMem[way][set] = front.data;
															ACK_L1(progrs_ReQ.address,progrs_ReQ.req_counter,true, true,NULL, false);
														} else {
															NPCtag[1][set][way] = SHARED;
															ACK_L1(progrs_ReQ.address,progrs_ReQ.req_counter,true, false,&front.data, false);
														}
														state = NPCtag[1][set][way];
														cout<<"NPC::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<" step RD done"<<endl;
														if(back != front)
															BlockedQ.pop();
														else {
															BlockedQ.pop();
															break;
														}
													}	
												}											
											} 
											else { // Remote mode
												cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< "ACK typeN1-2 "<<endl;
												front.mode_pri_remote = progrs_ReQ.mode_pri_remote;
												if (front.type == ReqFormat::RD){
													ACK_L1(front.address,front.req_counter,true, false,&progrs_ReQ.data, true);
													cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< endl;
													if(back != front)
														BlockedQ.pop();
													else {
														BlockedQ.pop();
														break;
													}
												} else if (front.type == ReqFormat::WR) {
													//ACK_L1(front.address,front.req_counter,true, true,NULL);
													if(front.mode_pri_remote == true) {
														cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< endl;
														BlockedQ.pop();
														BlockedQ.push(front);
													} else {
														ACK_L1(front.address,front.req_counter,true, true,NULL, false);
														cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< endl;
														if(back != front)
															BlockedQ.pop();
														else {
															BlockedQ.pop();
															break;
														}
													}
												} else {
													cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< "Error! "<<endl;
													if(back != front)
														BlockedQ.pop();
													else {
														BlockedQ.pop();
														break;
													}
												}
																						
											}
										}										
										else if (((front.type == ReqFormat::WR_BACK)) && (front.linked == true) && (progrs_ReQ.type == ReqFormat::ACK_FULL) && (state == INVALID))
										{
											BlockedQ.pop(); // this cant be the back in the queue
											ReqFormat frontnew = BlockedQ.front(); //actual req received RD or WR
											cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< "ACK type4"<<endl;
											if (frontnew.type == ReqFormat::ACKL1_WB) {
												ACK_L1(frontnew.address,frontnew.req_counter-4,true, true,NULL,true);
												cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<<endl;
												if(back != frontnew)
													BlockedQ.pop();
												else {
													BlockedQ.pop();
													break;
												}
											} 
											else {
												cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< "error: unknown type received @ NPC"<<endl;
												BlockedQ.push(front);
												BlockedQ.pop();
												BlockedQ.push(frontnew);
											} 
										}
										else {
											cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<<"Error!! type: "<<(unsigned int)front.type <<" ACKtype: "<<(unsigned int)progrs_ReQ.type<<" state:"<< (unsigned int)state<<endl;
											BlockedQ.pop();
											BlockedQ.push(front);
										}
										cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< "ACK last -1, Qsize:"<<(unsigned int) BlockedQ.size()<<endl;
									}
								} 
								else {
									BlockedQ.pop();
									BlockedQ.push(front);
								}
							} while(back != front);
						}
					}
					else if (((state < S_M))) {
						cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<<" State:"<<state<<", hit:"<<(unsigned int) hit <<"address:"<<hex<<progrs_ReQ.address<<dec<<endl;
					} 
					else{
						cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<<"ACK received Qsize"<<(unsigned int)BlockedQ.size()<<endl;
						if(BlockedQ.empty() == true) {
							cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<<"ACK received for non-blocked req"<< endl;
						} 
						else {
							ReqFormat front;
							ReqFormat back = BlockedQ.back();
							cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< "ACK start req_count : "<<progrs_ReQ.req_counter<<" address:"<<hex<<progrs_ReQ.address<<dec<<endl;
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
									if(progrs_ReQ.type == ReqFormat::ACK_SMALL)
										front.ex_ack_count = 0x0;
									else
										front.sc_ack_count = 0x0;
										
									if(((front.type == ReqFormat::RD) || (front.type == ReqFormat::WR)) && (progrs_ReQ.type == ReqFormat::ACK_SMALL) && ((state == S_M)) )
									{
										cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< "ACK type2 ackex:"<<(unsigned int)front.ex_ack_count <<" acksc:"<<(unsigned int)front.sc_ack_count << " state: "<< (unsigned int)state<<endl;
										if ((front.ex_ack_count == 0x0) && (front.sc_ack_count == 0x0)) {
											cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< "ACK type21"<<endl;
											if(progrs_ReQ.mode_pri_remote == false) {
												NPCtag[0][set][front.way] = (front.address) >> (NPC_SET_COUNT_2 + CL_SIZE) ; // tag											
												LRU_uliz_update(true,set,front.way,true);
												NPCtag[1][set][front.way] = MODIFIED; //modified
												NPCMem[front.way][set] = front.data;
													
												ACK_L1(progrs_ReQ.address,front.req_counter,true, true,NULL, false);
											} else {
												cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< " Error:Exclusion request resulted in CL eviction!"<<endl;	
												//LRU_uliz_update(false,set,front.way,false);
												//NPCMem[front.way][set] = front.data; /*TODO send data to LLc and wait for ack then respond pr.*/
												ACK_L1(progrs_ReQ.address,front.req_counter,true, true,NULL,true);
											}
											if(back != front)
												BlockedQ.pop();
											else {
												BlockedQ.pop();
												break;
											}
										} 	else {
											BlockedQ.pop();
											BlockedQ.push(front);
										}									
									} 
									else if ((front.type == ReqFormat::WR_BACK) && (front.linked == true) && (progrs_ReQ.type == ReqFormat::ACK_FULL) && (state == M_I_WB))
									{
										if ((front.ex_ack_count == 0x0) && (front.sc_ack_count == 0x0)) {
										BlockedQ.pop(); // this cant be the back in the queue										
										ReqFormat frontnew = BlockedQ.front(); //actual req received RD or WR
										cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< "ACK type3"<<endl;
										
										if((frontnew.type == ReqFormat::RD)) {
											inv_l1(front.address,front.req_counter);
											NPCtag[1][set][way] = SHARED; //Inv to S
											NPCtag[0][set][way] = (frontnew.address) >> (NPC_SET_COUNT_2 + CL_SIZE);
											LRU_uliz_update(true,set,way,true);
											NPCMem[way][set] = frontnew.data;
											ACK_L1(frontnew.address,frontnew.req_counter,true, false,&NPCMem[way][set], false);											
											if(back != frontnew)
												BlockedQ.pop();
											else {
												BlockedQ.pop();
												break;
											}
											cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< "ACK type3 RD"<<endl;
										} else if ((frontnew.type == ReqFormat::WR)) {	
											inv_l1(front.address,front.req_counter);
											NPCtag[1][set][way] = MODIFIED; //Inv to M
											NPCtag[0][set][way] = (frontnew.address) >> (NPC_SET_COUNT_2 + CL_SIZE);
											NPCMem[way][set] = frontnew.data;
											LRU_uliz_update(true,set,way,true);
											ACK_L1(frontnew.address,frontnew.req_counter,true, true,NULL, false);
											cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< "ACK type3 WR"<<endl;
											if(back != frontnew)
												BlockedQ.pop();
											else {
												BlockedQ.pop();
												break;
											}
										} else if((frontnew.type == ReqFormat::FLUSH)) {											
											NPCtag[1][set][frontnew.way] = SHARED; //S
											NPCtag[0][set][frontnew.way] = (frontnew.address) >> (NPC_SET_COUNT_2 + CL_SIZE);
											inv_ack_dir(frontnew.address,frontnew.req_counter,frontnew.way);	
											cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< "ACK type3 FLUSH"<<endl;
											if(back != frontnew)
												BlockedQ.pop();
											else {
												BlockedQ.pop();
												break;
											}
										} else if ((frontnew.type == ReqFormat::INV)){	
											inv_l1(frontnew.address,frontnew.req_counter);
											NPCtag[1][set][frontnew.way] = INVALID;
											NPCtag[0][set][frontnew.way] = (frontnew.address) >> (NPC_SET_COUNT_2 + CL_SIZE);
											#ifdef INV_4PHASE
											inv_ack_dir(frontnew.address,frontnew.req_counter,frontnew.way);	
											#endif	
											LRU_uliz_update(false,set,frontnew.way,false);
											cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< "ACK type3 INV"<<endl;
											if(back != frontnew)
												BlockedQ.pop();
											else {
												BlockedQ.pop();
												break;
											}
										} 
										else  {
											cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< "error: unknown type received @ NPC"<<endl;
											BlockedQ.pop();
											BlockedQ.push(front);
											BlockedQ.push(frontnew);
										}
										} else {
											cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< "error: unknown type received @ NPC"<<endl;
											BlockedQ.pop();
											BlockedQ.push(front);
										}
									}		
									else {
										cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<<"Error!! type: "<<(unsigned int)front.type <<" ACKtype: "<<(unsigned int)progrs_ReQ.type<<" state:"<< (unsigned int)state<<endl;
										if(back != front)
											BlockedQ.pop();
										else {											
											BlockedQ.pop();
											break;
										}
									}
									cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< "ACK last -1, Qsize:"<<(unsigned int) BlockedQ.size()<<endl;
								} else 
								{
									BlockedQ.pop();
									BlockedQ.push(front);
								}
							} while(back != front);
							//cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< "ACK last"<<endl;
						}	
						
					} 					
					req_in_progress = false;
					//cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< "ACK done"<<endl;
					} break;
			case ReqFormat::RD:
			case ReqFormat::WR:{
					cout<<flush;
					if (state <S_M){
						//cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<<"RD/WR req received Qsize"<<ReqQ.size()<<endl;
					}
					if ((hit == true) && (state == MODIFIED)) {
						cout<<"NPC::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<"hit M"<<endl;						
						LRU_uliz_update(true,set,way, true);
						if(progrs_ReQ.type == ReqFormat::RD) {
							ACK_L1(progrs_ReQ.address,progrs_ReQ.req_counter,true, false,&NPCMem[way][set],false);
						}else {
							ACK_L1(progrs_ReQ.address,progrs_ReQ.req_counter,true, true,NULL,false);
						}
						l2_hit_count++;	
					} else if ((hit == true) && (state == SHARED)) {
						cout<<"NPC::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<"hit S"<<endl;						
						if(progrs_ReQ.type == ReqFormat::RD) {
							LRU_uliz_update(true,set,way,true);
							ACK_L1(progrs_ReQ.address,progrs_ReQ.req_counter,true, false,&NPCMem[way][set],false);
							l2_hit_count ++;
						}else {
							// start a exclusive perm req
							// block this WR req
							if (find_in_blockedQ(progrs_ReQ.address) == true){
							ReqQ.push(progrs_ReQ);
							} 
							else{
							l2_hit_count ++;
							cout<<"NPC::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<"hit S write!"<<endl;		
							rd_rdx_to_dir(progrs_ReQ.address, true,false,progrs_ReQ.req_counter);
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
							NPCtag[1][set][way] = S_M; //S to M
							}
						}						
					}
					else if (hit == true) {
					// address in transition state, push the request to back 
						ReqQ.push(progrs_ReQ);
						//cout<<"NPC::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<"hit T"<<endl;	
					}
					else {	// miss case						
						if (find_in_blockedQ(progrs_ReQ.address) == true){
							ReqQ.push(progrs_ReQ);
						} 
						else{	
							ReqFormat ReQ;
							cout<<"NPC::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<"hit Miss"<<endl;
							l2_miss_count ++;
							if(ReqFormat::WR == progrs_ReQ.type) {
								rd_rdx_to_dir(progrs_ReQ.address, true,true,progrs_ReQ.req_counter);
								ReQ.sc_ack_count = 0x1;
								ReQ.ex_ack_count = 0x1;
								//cout<<"NPC::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<" step 4X"<<endl;
							} else {
								rd_rdx_to_dir(progrs_ReQ.address, false,true,progrs_ReQ.req_counter);
								ReQ.sc_ack_count = 0x1;
								ReQ.ex_ack_count = 0x0;
								//cout<<"NPC::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<" step 4-"<<endl;
							}	
							cout<<flush;								
							ReQ.address = progrs_ReQ.address;
							ReQ.type = progrs_ReQ.type;
							ReQ.ex = progrs_ReQ.ex;
							ReQ.we = progrs_ReQ.we;
							ReQ.data = progrs_ReQ.data;
							ReQ.linked = false;
							ReQ.req_counter = progrs_ReQ.req_counter;
							//cout<<"NPC::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<" step 5"<<endl;
							BlockedQ.push(ReQ);
						
						}
					}	
					
					//cout<<"NPC::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<"RDWR done"<<endl;	
					req_in_progress = false;
				} break;	
			default: 
					cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< ": Error: NPC::main_thread !!" <<endl; wait();
		}	
	wait();
	//cout<<inode<<" : "<<"NPC::"<<__func__<<" : -- "<< __LINE__<<endl;	
	}
}


