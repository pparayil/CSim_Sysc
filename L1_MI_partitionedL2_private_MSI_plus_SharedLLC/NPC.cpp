
#include "NPC.h"

#define LOOKUP_DELAY_NPC 1

// NPCMEM needs to be 4 ways * number of sets

/**
TODO: Flush request handling , DIR--> flush to NPC, NPC write data to llc/mem maintaning a copy with itself + state change from M to S  -- done testing is pending
TODO: add a provision for NPC-mem/llc direct WR interaction without directory. -- done testing is pending
*/

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
bool NPC::table_lookup(int address, char * hit_way, int * state){
	 
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
		if((NPCtag[0][set][i] == tag) && ((NPCtag[1][set][i] > 0x0))) // 3 bit state I 00 /V 01/M 10 // transition states: 100 : INV - SHARED (RD) , 101 : Shared -INV(INV simple), 110: inv to M (exRD+WR) , 111: M to INV (INV with WB), 011: Shared to M
		{
			hit = true;
			*hit_way = (char) i;
			*state = (int) NPCtag[1][set][i];			
			break;
		} else if((NPCtag[0][set][i] == tag) && ((NPCtag[1][set][i] == 0x0)))	{
			*state = (int) NPCtag[1][set][i];
		}		
	}

	return hit;
}

void NPC::write_to_llc ( int address, datatype * DATA, bool evict, int req_count, int way, bool EN_req_to_directory){
	//cout<<"NPC::"<<__func__<<" : "<< __LINE__<<" evict:"<<(unsigned int)evict<<endl;	
	/*TODO: remove directory involvement in WBs if required*/ //-- done testing pending
	if(EN_req_to_directory == true)
	{
		cl_evt_info_to_dir = evict;
		rd_wr_to_dir = true;
		ex_to_dir = false;
		ce_to_dir = true;
		address_to_dir = address;
		req_count_to_dir = req_count;
		wait();
		cl_evt_info_to_dir = false;
		rd_wr_to_dir = false;
		ex_to_dir = false;
		ce_to_dir = false;
		wait();
	} else {
		//cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<<endl;
		generate_wr_req_npc = true;
		address_to_llc_en = true;
		address_to_llc = address;
		req_count_to_llc = req_count;
		wait();
		generate_wr_req_npc = false;
		address_to_llc_en = false;
		wait();
	}
	address_to_llc_en = true;
	address_to_llc = address;
	req_count_to_llc = req_count;
	if(DATA != NULL)
		data_to_llc->write_data(DATA);
	wait();
	address_to_llc_en = false;
	wait();
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
	
	//cout<<"NPC::"<<__func__<<" : "<< __LINE__<<endl;
}

char NPC::find_free_way (int address,int req_count,bool *wb_enable, bool *status) {
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
				if (NPCtag[1][set][j] == 0x02) // Modified
				{
					int temp_new_address = (NPCtag[0][set][j]<<(NPC_SET_COUNT_2 + CL_SIZE)) | ((set & ((unsigned int)NPC_SET_COUNT-1))<<(CL_SIZE)) | 0b00000;
					cout<<"NPC::"<<inode << " : "<<__func__<<" : "<< __LINE__<<" Modified state, tag:"<<hex<<NPCtag[0][set][j] <<" new addr:"<<temp_new_address <<" set:"<<set<<dec<<endl;
					write_to_llc(temp_new_address,&NPCMem[j][set], true,req_count,j, true); // eviction WBs
					NPCtag[1][set][j] = 0x7; // M to I 
					*wb_enable =  true;				
					//cout<<"NPC::"<<inode << " : "<<__func__<<" : "<< __LINE__<<endl;
					found = true;
				}
				else
				if (NPCtag[1][set][j] == 0x01) //valid
				{
					//cout<<"NPC::"<<inode << " : "<<__func__<<" : "<< __LINE__<<" In shared state"<<endl;		
					int temp_new_address = (NPCtag[0][set][j]<<(NPC_SET_COUNT_2 + CL_SIZE)) | ((set & ((unsigned int)NPC_SET_COUNT-1))<<(CL_SIZE)) | 0b00000;
					address_to_dir = temp_new_address;
					req_count_to_dir = req_count;
					cl_evt_info_to_dir = true; // directory will remove the node id from the sharers list.
					ce_to_dir = false;
					wait();
					cl_evt_info_to_dir = false;
					wait();
					NPCtag[1][set][j] = 0x0;
					found = true;
					
					//cout<<"NPC::"<<inode << " : "<<__func__<<" : "<< __LINE__<<endl;
				}
				else if(NPCtag[1][set][j] > 0x02) // trans state dont remove this /*either wait for this to get proper state (probable deadlock) or take LRU number 1*/
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

bool NPC::rd_rdx_to_dir (int address, bool Exclusive, bool refill,int req_count){
	//cout<<"NPC::"<<inode << " : "<<__func__<<" : "<< __LINE__<<endl;
	rd_wr_to_dir = false;
	ce_to_dir = refill;
	ex_to_dir = Exclusive;
	cl_evt_info_to_dir = false;
	address_to_dir = address;
	req_count_to_dir = req_count;
	wait();
	ce_to_dir = false;
	ex_to_dir = false;
	wait();
}

bool NPC::invalidating_cl (int address ,int way, int req_count, unsigned int type, ReqFormat::ReQType tp)
{
	int set = (address >> CL_SIZE) & ((unsigned int)NPC_SET_COUNT-1); 
	int tag = (address) >> (NPC_SET_COUNT_2 + CL_SIZE);
	bool ret = true;
	//cout<<"NPC::"<<inode << " : "<<__func__<<" : "<< __LINE__<<endl;
	if (type == 0x0) {//valid
		//cout<<"NPC::"<<inode << " : "<<__func__<<" : "<< __LINE__<<"invalidate shared"<<endl;
		if(tp == ReqFormat::INV){ // Shared state
			NPCtag[1][set][way] = 0x0;
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
			inv_to_l1 = true;
			address_to_l1 = address;
			req_count_to_l1 = req_count;
			wait();
			inv_to_l1 = false;
			wait();
		} 
		else if (tp == ReqFormat::FLUSH) {
			NPCtag[1][set][way] = 0x1; 
		} 
		else {
			cout<<"NPC::"<<inode << " : "<<__func__<<" : "<< __LINE__<<"Error Unknown state!"<<endl;
			return true;
		}
		#ifdef INV_4PHASE
		wait();
		inv_ack_to_dir = true;
		address_to_dir = address;
		req_count_to_dir = req_count;
		wait();
		inv_ack_to_dir = false;
		wait();
		#endif	
		ret = true;
	} 
	else if (type == 0x1) // Modified, hence require WB 
	{
		cout<<"NPC::"<<inode << " : "<<__func__<<" : "<< __LINE__<<" : invalidate Modified"<<endl;
		
		ReqFormat ReQ;
		if(tp == ReqFormat::FLUSH) {
			write_to_llc(address,&NPCMem[way][set], false,req_count, way, false); //dir already knws the status of cache
			ReQ.type = ReqFormat::FLUSH;
			NPCtag[1][set][way] = 0x07; // M to I + flush = M to S 
		} 
		else {
			write_to_llc(address,&NPCMem[way][set], true,req_count, way, false); 
			ReQ.type = ReqFormat::INV;
			NPCtag[1][set][way] = 0x07; // M to I
			inv_to_l1 = true;
			address_to_l1 = address;
			req_count_to_l1 = req_count;
			wait();
			inv_to_l1 = false;
			wait();
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
	else if (type == 0x2) //transition state
	{
		//search blockedQ
		if(BlockedQ.empty() == true) 
		{
			NPCtag[1][set][way] = 0x00;
			cout<<"NPC::"<<inode << " : "<<__func__<<" : "<< __LINE__<<" INV 1"<<endl;
			#ifdef INV_4PHASE
			inv_ack_to_dir = true;
			address_to_dir = address;
			req_count_to_dir = req_count;
			wait();
			inv_ack_to_dir = false;
			wait();
			#endif
		} else
		{
				ReqFormat front,fronttemp;
				ReqFormat back = BlockedQ.back();
				do {
					front = BlockedQ.front();					
					if(((front.address >> CL_SIZE) == (address >> CL_SIZE))  && ((front.type != ReqFormat::WR_BACK) && (front.type != ReqFormat::INV) && (front.type != ReqFormat::FLUSH)))
					{
						inv_to_l1 = true;
						address_to_l1 = front.address;
						req_count_to_l1 = req_count;
						wait();
						inv_to_l1 = false;
						wait();	
						if((tp != ReqFormat::FLUSH)) {
							NPCtag[1][set][way] = 0x00;		
							cout<<"NPC::"<<inode << " : "<<__func__<<" : "<< __LINE__<<" INV 2"<<endl;
							#ifdef INV_4PHASE
							inv_ack_to_dir = true;
							address_to_dir = address;
							req_count_to_dir = req_count;
							wait();
							inv_ack_to_dir = false;
							wait();
							#endif
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
							ret = true;
							if(back != front)
								BlockedQ.pop();
							else {
								BlockedQ.pop();
								break;
							}
						} 
						else if((tp == ReqFormat::FLUSH) && ((NPCtag[1][set][way] == 0x03) )){											
							#ifdef INV_4PHASE
							inv_ack_to_dir = true;
							address_to_dir = address;
							req_count_to_dir = req_count;
							wait();
							inv_ack_to_dir = false;
							wait();
							#endif
							cout<<"NPC::"<<inode << " : "<<__func__<<" : "<< __LINE__<<" INV 3"<<endl;
							NPCtag[1][set][way] = 0x01;	
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
							inv_to_l1 = true;
							address_to_l1 = fronttemp.address;
							req_count_to_l1 = fronttemp.req_counter;
							wait();
							inv_to_l1 = false;
							wait();	
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
							cout<<"NPC::"<<inode << " : "<<__func__<<" : "<< __LINE__<<" Error! type ="<<(unsigned int) fronttemp.type<<endl; //wait
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
			NPCtag[1][set][way] = 0x0;	
			cout<<"NPC::"<<inode << " : "<<__func__<<" : "<< __LINE__<<" Error! address:"<<hex<<address<<dec<<endl; //wait
			#ifdef INV_4PHASE
			inv_ack_to_dir = true;
			address_to_dir = address;
			req_count_to_dir = req_count;
			wait();
			inv_ack_to_dir = false;
			wait();
			#endif
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
							if((((flush_frm_npc_dir == true) || (ack_noc == true)) && (inv_frm_dir == true)) ||(((inv_frm_dir == true) || (ack_noc == true)) && (flush_frm_npc_dir == true)) || (((inv_frm_dir == true) || (flush_frm_npc_dir == true)) && (ack_noc == true)))
							{
								//impossible! as all comes thru noc and wud require queueing at lsu		
								cout<<inode<<" : "<<"NPC::"<<__func__<<" : "<< __LINE__<<" Error!!"<<endl;
								st = RD_WR;								
								st1 = IDLE;
							} else if ((inv_frm_dir == true) || (flush_frm_npc_dir == true))
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
						else if((((flush_frm_npc_dir == true) || (ack_noc == true)) && (inv_frm_dir == true)) ||(((inv_frm_dir == true) || (ack_noc == true)) && (flush_frm_npc_dir == true)) || (((inv_frm_dir == true) || (flush_frm_npc_dir == true)) && (ack_noc == true)))
						{
							//impossible! as all comes thru noc and wud require queueing at lsu		
							cout<<inode<<" : "<<"NPC::"<<__func__<<" : "<< __LINE__<<" Error!!"<<endl;
							wait(); 
							st = IDLE;
							st1 = IDLE;
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
						cout<<"NPC::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<" ACK from dir"<<endl;
					}
					else if (ack_from_llc == true) {
						ReQ3.type = ReqFormat::ACK_FULL;
						ReQ3.address = address_from_llc;
						ReQ3.req_counter = req_count_from_llc; //((inode<<3) | 0x0);				
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
		state = 0x0;
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
					if ((hit == true) && (state == 0x01)) {
						ret = invalidating_cl(progrs_ReQ.address,way,progrs_ReQ.req_counter, 0x0,progrs_ReQ.type); // simple
					}
					else if ((hit == true) && (state == 0x02)) {
					    ret = invalidating_cl(progrs_ReQ.address,way,progrs_ReQ.req_counter,0x1,progrs_ReQ.type); // with WB
					}
					else if (hit == true) 
						ret = invalidating_cl(progrs_ReQ.address,way,progrs_ReQ.req_counter,0x2,progrs_ReQ.type); // for TRANS state inv
					else {	// hit false so already invalidated 
						if(ReqFormat::FLUSH == progrs_ReQ.type) {
							cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<<"Error: FLUSH invalid CL? "<<endl;							
						} 
						ret = true;
						#ifdef INV_4PHASE
						inv_ack_to_dir = true;
						address_to_dir = progrs_ReQ.address;
						req_count_to_dir = progrs_ReQ.req_counter;
						wait();
						inv_ack_to_dir = false;
						wait();
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
					if ((hit == false) || ((state < 0x03))) {
						if(state == 0x0)
						cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<<" State:0 hit:"<<(unsigned int) hit <<"address:"<<hex<<progrs_ReQ.address<<dec<<endl;
					}else{
						cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<<"ACK received Qsize"<<(unsigned int)BlockedQ.size()<<endl;
						if(BlockedQ.empty() == true) 
						{
							cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<<"ACK received for non-blocked req"<< endl;
						} else
						{
							ReqFormat front;
							ReqFormat back = BlockedQ.back();
							cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< "ACK start req_count = "<<progrs_ReQ.req_counter<<" address:"<<hex<<progrs_ReQ.address<<dec<<endl;
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
									if (((front.type == ReqFormat::RD) || (front.type == ReqFormat::WR)) && (progrs_ReQ.type == ReqFormat::ACK_FULL) && ((state == 0x04) || (state == 0x06))){ 
										cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< "ACK type1 ackex:"<<(unsigned int)front.ex_ack_count <<" acksc:"<<(unsigned int)front.sc_ack_count << " state: "<< (unsigned int)state<<endl;
										if((front.type == ReqFormat::RD)){
											NPCMem[front.way][set] = progrs_ReQ.data;											
										}
										if ((front.ex_ack_count == 0x0) && (front.sc_ack_count == 0x0)) {
											//cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< "ACK type11"<<endl;
											NPCtag[0][set][front.way] = (front.address) >> (NPC_SET_COUNT_2 + CL_SIZE); // tag
											for(char j=0; j<NPC_WAYS; j++)
											{
												if (j == front.way) continue;
												else if (NPCtag[2][set][j] != 0)
												{
												NPCtag[2][set][j] = NPCtag[2][set][j] - 1;
												}
											}
											NPCtag[2][set][front.way] = NPC_WAYS-1;
											if (((front.type == ReqFormat::RD))&& (front.linked == false) &&(state == 0x04)) {
												NPCtag[1][set][front.way] = 0x1; //shared
												data_to_l1->write_data(&NPCMem[front.way][set]);
												ack_we_to_l1 = false;
											}
											else if (((front.type == ReqFormat::RD) ||(front.type == ReqFormat::WR))&& (front.linked == false)&&((state == 0x06))) {
												NPCtag[1][set][front.way] = 0x02; //modified
												ack_we_to_l1 = true;
												NPCMem[front.way][set] = front.data; // new data to be written
											} 
											else{
												cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< "Error!! type:"<<(unsigned int)front.type<<" linked:"<<(unsigned int)front.linked <<" state:"<<(unsigned int)state <<endl;
											}
											req_count_to_l1 = front.req_counter;
											address_to_l1 = progrs_ReQ.address;
											ack_to_l1 = true;													
											wait();
											ack_to_l1 = false;
											wait();	
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
									else if(((front.type == ReqFormat::RD) || (front.type == ReqFormat::WR)) && (progrs_ReQ.type == ReqFormat::ACK_SMALL) && ((state == 0x03) || (state == 0x06)) )
									{
										cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< "ACK type2 ackex:"<<(unsigned int)front.ex_ack_count <<" acksc:"<<(unsigned int)front.sc_ack_count << " state: "<< (unsigned int)state<<endl;
										if ((front.ex_ack_count == 0x0) && (front.sc_ack_count == 0x0)) {
											//cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< "ACK type21"<<endl;
											NPCtag[0][set][front.way] = (front.address) >> (NPC_SET_COUNT_2 + CL_SIZE) ; // tag
											for(char j=0; j<NPC_WAYS; j++)
											{
												if (j == front.way) continue;
												else if (NPCtag[2][set][j] != 0)
												{
												NPCtag[2][set][j] = NPCtag[2][set][j] - 1;
												}
											}
											NPCtag[2][set][front.way] = NPC_WAYS-1;
											NPCtag[1][set][front.way] = 0x02; //modified
											NPCMem[front.way][set] = front.data;
											
											ack_we_to_l1 = true;
											address_to_l1 = progrs_ReQ.address;
											req_count_to_l1 = front.req_counter;
											ack_to_l1 = true;													
											wait();
											ack_to_l1 = false;
											wait();		
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
									else if ((front.type == ReqFormat::WR_BACK) && (front.linked == true) && (progrs_ReQ.type == ReqFormat::ACK_FULL) && (state == 0x07))
									{
										if(back != front)
											BlockedQ.pop();
										else {
											cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< "Error!!"<<endl;
											BlockedQ.pop();
											break;
										}
										front = BlockedQ.front(); //actual req received RD or WR
										cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< "ACK type3"<<endl;
										if((front.type == ReqFormat::RD)) {
											rd_rdx_to_dir(front.address, false,true,front.req_counter);
											NPCtag[1][set][front.way] = 0x04; //Inv to S
											NPCtag[0][set][front.way] = (front.address) >> (NPC_SET_COUNT_2 + CL_SIZE);
											front.ex_ack_count = 0x0;
											front.sc_ack_count = 0x1;
											BlockedQ.pop();
											BlockedQ.push(front);
											//cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< "ACK type3 RD"<<endl;
										} else if ((front.type == ReqFormat::WR)) {
											rd_rdx_to_dir(front.address, true,true,front.req_counter);	
											NPCtag[1][set][front.way] = 0x6; //Inv to M
											NPCtag[0][set][front.way] = (front.address) >> (NPC_SET_COUNT_2 + CL_SIZE);
											front.ex_ack_count = 0x1;
											front.sc_ack_count = 0x1;
											//cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< "ACK type3 WR"<<endl;
											BlockedQ.pop();
											BlockedQ.push(front);
										} else if((front.type == ReqFormat::FLUSH)) {											
											NPCtag[1][set][front.way] = 0x1; //S
											NPCtag[0][set][front.way] = (front.address) >> (NPC_SET_COUNT_2 + CL_SIZE);
											inv_ack_to_dir = true;
											address_to_dir = front.address;
											req_count_to_dir = front.req_counter;
											wait();
											inv_ack_to_dir = false;	
											wait();	
											//cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< "ACK type3 FLUSH"<<endl;
											if(back != front)
												BlockedQ.pop();
											else {
												BlockedQ.pop();
												break;
											}
										} else if ((front.type == ReqFormat::INV)){										
											NPCtag[1][set][front.way] = 0x0;
											NPCtag[0][set][front.way] = (front.address) >> (NPC_SET_COUNT_2 + CL_SIZE);
											#ifdef INV_4PHASE
											inv_ack_to_dir = true;
											address_to_dir = front.address;
											req_count_to_dir = front.req_counter;
											inv_to_l1 = true;
											address_to_l1 = front.address;
											req_count_to_l1 = front.req_counter;
											wait();
											inv_to_l1 = false;
											inv_ack_to_dir = false;
											wait();	
											#endif	
											for(int j = 0;j<NPC_WAYS; j++)
											{
												if (j== front.way) continue;
												if(NPCtag[2][set][j] < NPCtag[2][set][front.way]) 
												{
													NPCtag[2][set][j] = NPCtag[2][set][j]+1;
												}		
												
											}
											NPCtag[2][set][front.way] = 0x0;
											//cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< "ACK type3 INV"<<endl;
											if(back != front)
												BlockedQ.pop();
											else {
												BlockedQ.pop();
												break;
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
								} 
								else  {
									BlockedQ.pop();
									BlockedQ.push(front);
								}
							} while(back != front);
							cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< "ACK last"<<endl;
						}	
						
					} 					
					req_in_progress = false;
					//cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<< "ACK done"<<endl;
					} break;
			case ReqFormat::RD:
			case ReqFormat::WR:{
					cout<<flush;
					if (state <0x03)
						cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<<"RD/WR req received Qsize"<<ReqQ.size()<<endl;
					if ((hit == true) && (state == 0x02)) {
						cout<<"NPC::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<"hit M"<<endl;
						for(char j=0; j<NPC_WAYS; j++)
						{
							if (j == way) continue;
							if (NPCtag[2][set][j] > NPCtag[2][set][way])
							{
							NPCtag[2][set][j] = NPCtag[2][set][j] - 1;
							}
						}
						NPCtag[2][set][way] = NPC_WAYS-1;
						address_to_l1 = progrs_ReQ.address;
						req_count_to_l1 = progrs_ReQ.req_counter;
						ack_to_l1 = true;
						if(progrs_ReQ.type == ReqFormat::RD) {
							data_to_l1->write_data(&NPCMem[way][set]);
							ack_we_to_l1 = false;
						}else {
							NPCMem[way][set] = progrs_ReQ.data;
							ack_we_to_l1 = true;
						}
						wait();
						ack_to_l1 = false;	
						wait();	
						l2_hit_count ++;
					} else if ((hit == true) && (state == 0x01)) {
						cout<<"NPC::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<"hit S"<<endl;						
						if(progrs_ReQ.type == ReqFormat::RD) {
							for(char j=0; j<NPC_WAYS; j++)
							{
								if (j == way) continue;
								if (NPCtag[2][set][j] > NPCtag[2][set][way])
								{
								NPCtag[2][set][j] = NPCtag[2][set][j] - 1;
								}
							}
							NPCtag[2][set][way] = NPC_WAYS-1;
							data_to_l1->write_data(&NPCMem[way][set]);
							ack_we_to_l1 = false;
							address_to_l1 = progrs_ReQ.address;
							req_count_to_l1 = progrs_ReQ.req_counter;
							ack_to_l1 = true;
							wait();
							ack_to_l1 = false;
							wait();	
							l2_hit_count ++;	
						}else {
							// start a exclusive perm req
							// block this WR req										
							if (find_in_blockedQ(progrs_ReQ.address) == true){
							ReqQ.push(progrs_ReQ);
							} else{
							l2_hit_count ++;
							NPCtag[1][set][way] = 0x03; //S to M
							progrs_ReQ.type = ReqFormat::WR;
							progrs_ReQ.ex = true;
							progrs_ReQ.we = true;
							progrs_ReQ.way = way;
							progrs_ReQ.ex_ack_count = 0x1;
							progrs_ReQ.sc_ack_count = 0x0;
							BlockedQ.push(progrs_ReQ);
							rd_rdx_to_dir(progrs_ReQ.address, true,false,progrs_ReQ.req_counter);
							cout<<"NPC::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<"hit S write! req_count = "<<progrs_ReQ.req_counter<<" address:"<<hex<<progrs_ReQ.address<<dec<<endl;
							}
						}						
					}
					else if (hit == true) {
					// address in transition state, push the request to back 
						ReqQ.push(progrs_ReQ);
						//cout<<"NPC::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<"hit T"<<endl;	
					}
					else {	// miss case
						bool wb_enable = true;
						bool status = true;
						cout<<"NPC::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<"hit Miss"<<endl;
						if (find_in_blockedQ(progrs_ReQ.address) == true){
							ReqQ.push(progrs_ReQ);
						} else{
						way = find_free_way(progrs_ReQ.address,progrs_ReQ.req_counter,&wb_enable, &status);
						if(status == false)
							ReqQ.push(progrs_ReQ);
						else{
							//cout<<"NPC::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<" step 1"<<endl;
							l2_miss_count ++; 
							if(wb_enable == true) {
								// wait for WB to complete and then proceed with RD/WR
								ReqFormat ReQ;
								ReQ.address = progrs_ReQ.address;
								ReQ.type = progrs_ReQ.type;
								ReQ.ex = true;
								ReQ.we = progrs_ReQ.we;
								ReQ.way = way;
								if(ReQ.type == ReqFormat::WR) {
									ReQ.sc_ack_count = 0x1;
									ReQ.ex_ack_count = 0x1;
								} else {
									ReQ.sc_ack_count = 0x1;
									ReQ.ex_ack_count = 0x0;
								}
								ReQ.data = progrs_ReQ.data;
								ReQ.linked = false;
								ReQ.req_counter = progrs_ReQ.req_counter;
								//cout<<"NPC::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<" step 2"<<endl;
								BlockedQ.push(ReQ);			
							} else
							{
								//start a read/exclusive read req to dir and then block the request.
								//cout<<"NPC::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<" step 3"<<endl;
								ReqFormat ReQ;
								if(ReqFormat::WR == progrs_ReQ.type) {								
									NPCtag[1][set][way] = 0x06; // I to M
									rd_rdx_to_dir(progrs_ReQ.address, true,true,progrs_ReQ.req_counter);
									ReQ.sc_ack_count = 0x1;
									ReQ.ex_ack_count = 0x1;
									//cout<<"NPC::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<" step 4X"<<endl;
								} else {
									NPCtag[1][set][way] = 0x04; // I to S
									rd_rdx_to_dir(progrs_ReQ.address, false,true,progrs_ReQ.req_counter);
									ReQ.sc_ack_count = 0x1;
									ReQ.ex_ack_count = 0x0;
									//cout<<"NPC::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<" step 4-"<<endl;
								}
								NPCtag[0][set][way] = (progrs_ReQ.address) >> (NPC_SET_COUNT_2 + CL_SIZE);	
								cout<<flush;								
								ReQ.address = progrs_ReQ.address;
								ReQ.type = progrs_ReQ.type;
								ReQ.ex = progrs_ReQ.ex;
								ReQ.we = progrs_ReQ.we;
								ReQ.way = way;
								ReQ.data = progrs_ReQ.data;
								ReQ.linked = false;
								ReQ.req_counter = progrs_ReQ.req_counter;
								//cout<<"NPC::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<" step 5"<<endl;
								BlockedQ.push(ReQ);
								//cout << inode <<" : @NPC::"<<__func__ <<" : "<< __LINE__<<" RD/WR state:"<<NPCtag[1][set][way]<<endl;
							}	
						}
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


