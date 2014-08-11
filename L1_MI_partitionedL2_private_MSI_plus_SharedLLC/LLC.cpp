#include "LLC.h"

#define LOOKUP_DELAY_LLC 1

// LLCMEM needs to be 8 ways * number of sets

void LLC::LLC_init(bool instantiate){	
	//cout<<"LLC::"<<inode<<", "<<__func__<<" : "<< __LINE__<<endl;
	if (instantiate == true) {
		for (int i = 0; i< LLC_WAYS; i++){
			for(int k = 0; k < (D_O_C+1); k++){
				//cout<<"LLC::"<<inode<<", "<<__func__<<" : "<< __LINE__<<"LLC allocating!!!"<<endl;
				LLCMem[k][i] =  new datatype [LLC_SET_COUNT];
				memset(LLCMem[k][i],0x0,LLC_SET_COUNT);
				for(int j=0;j<LLC_SET_COUNT;j++)
					{
					LLCtag[k][2][i][j] == 0x0;
					LLCtag[k][1][i][j] == 0x0;
					LLCtag[k][0][i][j] == 0x0;
					}
			}
		}
	} else 
	{
		int i=0;
		//cout<<"LLC::"<<inode<<", "<<__func__<<" : "<< __LINE__<<endl;
		for(int k = 0; k < (D_O_C+1); k++){
			while(i<LLC_WAYS){
			LLCMem[k][i] = NULL;
			i++;
		}
		}
	}	
	cooper_section_cnt = D_O_C;
	llc_miss_count =0x0;	
	llc_hit_count = 0x0;
}

bool LLC::find_in_blockedQ(unsigned int address)
{
	bool found;
	found = false;
	if(BlockedQ.empty() == false) {
		ReqFormat_LLC front;
		ReqFormat_LLC back = BlockedQ.back();
		
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

unsigned int LLC::find_coop_sect(unsigned int address){
	return 0x0;
}
bool LLC::llc_lookup(unsigned int address, char *way, bool *self){
	unsigned int set = (address >> (CL_SIZE + TOTAL_NO_OF_NODES_CARRYING_LLC_2 + D_O_C))&(LLC_SET_COUNT-1);
	unsigned int tag = address >> (CL_SIZE + TOTAL_NO_OF_NODES_CARRYING_LLC_2 + D_O_C + LLC_SET_COUNT_2);	
	
	if((way == NULL) || (self == NULL)) {
		cout<<"LLC::"<<inode<<", "<<__func__<<" : "<< __LINE__<<" Error"<<endl;
		return false;
	}
	//for(char c=0; c<(cooper_section_cnt+1); c++) {
	unsigned int c = 0x0; //find_coop_sect(address);
	for (char i = 0; i< LLC_WAYS; i++) {		
		if((LLCtag[c][0][i][set] == tag) && (((unsigned int) LLCtag[c][1][i][set]) > (unsigned int) (ReqFormat_LLC::REG_I))){ 
			*way = i; 
			if(0x0 == c)
				*self = true;// self part hit
			else
				*self = false; // cooperative part hit			
			return true;
		}
	}	
	return false;
	
}
void LLC::rdwr_to_mem ( unsigned int address, datatype *data, int req_count, bool we, unsigned int npc_node, unsigned int dir_node, unsigned int way, bool dirack_en) {
	cout<<"LLC::"<<inode<<", "<<__func__<<" : "<< __LINE__<<" : "<< __LINE__<<endl;
	addr_to_mem = address;
	req_count_to_mem = req_count;
	addr_en_to_mem = true;
	r_w_to_mem = we;
	nodeaddr_to_lru = NODE_WITH_SDRAM;
	if((we == true) && (data == NULL))
		cout<<"LLC::"<<inode<<", "<<__func__<<" : "<< __LINE__<<" Error"<<endl;
	else if(we == true){
		cout<<"LLC::"<<inode<<", "<<__func__<<" : "<< __LINE__<<" : "<< __LINE__<<" WR address:"<<hex<<address<<dec<<" req_count:"<<hex<<req_count<<dec<<endl;
		data_mem->write_data(data);
		ReqFormat_LLC ReQ;
		ReQ.address = address;
		ReQ.type = ReqFormat_LLC::WR_BACK;
		ReQ.we = true;
		ReQ.data = *data;
		ReQ.dir_node = 0xffff;
		ReQ.npc_node = 0xffff;
		ReQ.way = way;
		ReQ.dirack_en = false;
		ReQ.linked = true;
		ReQ.req_count = req_count;
		BlockedQ.push(ReQ);
	} else {
		ReqFormat_LLC ReQ;
		cout<<"LLC::"<<inode<<", "<<__func__<<" : "<< __LINE__<<" : "<< __LINE__<<" RD address:"<<hex<<address<<dec<<" req_count:"<<hex<<req_count<<dec<<endl;
		ReQ.address = address;
		ReQ.type = ReqFormat_LLC::RD;
		ReQ.we = false;
		ReQ.dirack_en = dirack_en;
		ReQ.linked = false;
		ReQ.npc_node = npc_node;
		ReQ.dir_node = dir_node;
		ReQ.way = way;
		ReQ.req_count = req_count;
		BlockedQ.push(ReQ);
	}
	wait();
	addr_en_to_mem = false;
	wait();	
}
void LLC::ack_to_npc_dir(unsigned int address, datatype *data, int req_count, bool we, unsigned int npc_node, unsigned int dir_node, bool dirack_en){
	cout<<"LLC::"<<inode<<", "<<__func__<<" : "<< __LINE__<<endl;
	if((we == false) && (data == NULL)){
		cout<<"LLC::"<<inode<<", "<<__func__<<" : "<< __LINE__<<" Error"<<endl;
	} else if(we == false){
		data_out->write_data(data);
	}
	nodeaddr_to_lru = npc_node;
	addr_t_l2 = address;
	req_count_t_l2 = req_count;
	ack_to_l2 = true;
	ack_rw_to_l2 = we;
	wait();
	ack_to_l2 = false;
	wait();
	if(dirack_en == true) {
		ack_to_dir = true;
		addr_t_dir = address;
		req_count_t_dir = req_count;
		nodeaddr_to_lru = dir_node;								
		wait();
		ack_to_dir = false;
		wait();	
	}
}
bool LLC::find_way(unsigned int address, char *way, int req_count, bool *WB_enable){
	
	unsigned int set = (address >> (CL_SIZE + TOTAL_NO_OF_NODES_CARRYING_LLC_2 + D_O_C))&(LLC_SET_COUNT-1);
	unsigned int tag = address >> (CL_SIZE + TOTAL_NO_OF_NODES_CARRYING_LLC_2 + D_O_C + LLC_SET_COUNT_2);
	unsigned int coop_sect = 0x0; //find_coop_sect(address);
	unsigned int doc_value =0x0;
	if((way == NULL) || (WB_enable==NULL)) {
		cout<<"LLC::"<<inode<<", "<<__func__<<" : "<< __LINE__<<" Error"<<endl;
		*WB_enable = false;
		return false;
	}		
	//cout<<"LLC::"<<inode<<", "<<__func__<<" : "<< __LINE__<<endl;
	for(char c=0, i=0;c<LLC_WAYS;c++){
		i = 0;
		while(i<LLC_WAYS)
		{			
			if(LLCtag[coop_sect][2][i][set] < (c+1)){
				if(LLCtag[coop_sect][1][i][set] == (unsigned int) ReqFormat_LLC::REG_M) { // need to writeback
					cout<<"LLC::"<<inode<<", "<<__func__<<" : "<< __LINE__<<" for address "<<hex<<address<<dec<<endl;
					unsigned int temp_address = ((LLCtag[coop_sect][0][i][set] << (LLC_SET_COUNT_2+CL_SIZE+TOTAL_NO_OF_NODES_CARRYING_LLC_2+D_O_C)) | (set << (CL_SIZE + TOTAL_NO_OF_NODES_CARRYING_LLC_2 + D_O_C)) | (doc_value << (CL_SIZE + TOTAL_NO_OF_NODES_CARRYING_LLC_2)) |(inode <<(CL_SIZE)) | 0b00000) ;
					rdwr_to_mem(temp_address,&LLCMem[coop_sect][i][set],req_count, true, 0x0,0x0, i, false);
					LLCtag[coop_sect][1][i][set] = (unsigned int) ReqFormat_LLC::TRANS_MI;
					/*LLCtag[coop_sect][0][i][set] = (unsigned int) tag;*/
					/* shud put req in blocked req queue*/
					*WB_enable = true;
					*way = i;
					return true;
				} else if ((LLCtag[coop_sect][1][i][set] == (unsigned int)ReqFormat_LLC::REG_S) || (LLCtag[coop_sect][1][i][set] == (unsigned int)ReqFormat_LLC::REG_I)) { // shared or invalid
					LLCtag[coop_sect][1][i][set] = (unsigned int) ReqFormat_LLC::REG_I;
					LLCtag[coop_sect][0][i][set] = (unsigned int) tag;
					*way = i;
					*WB_enable = false;
					cout<<"LLC::"<<inode<<", "<<__func__<<" : "<< __LINE__<<endl;
					return true;
				} else {//if(LLCtag[coop_sect][1][i][set] > (unsigned int) ReqFormat_LLC::REG_M) { //trans wait
					i++;
					continue;
				}
			}
			i++;
		}		
	}
	//cout<<"LLC::"<<inode<<", "<<__func__<<" : "<< __LINE__<<" couldnt find freeway"<<endl;
	*WB_enable = false;
	return false;
}

void LLC::LLC_request_collector(void){
	//cout<<"LLC::"<<inode<<", "<<__func__<<" : "<< __LINE__<<endl;
	while(1) {
	if(addr_en_frm_l2 == true){
			ReqData_LLC ReQ1;
			ReQ1.npc_node = node_address_from_lru;
			ReQ1.address = addr_f_l2;
			ReQ1.req_count = req_count_f_l2;
			data_in->read_data(&ReQ1.data);
			ReqQ_data.push(ReQ1);
			cout<<"LLC::"<<inode<<", "<<__func__<<" : "<< __LINE__<<" for address: "<<hex<< addr_f_l2 << " from "<<node_address_from_lru<<dec<<" DataQsize:"<< ReqQ_data.size()<< endl;	
		}

		if(ack_from_mem == true) {
			cout<<"LLC::"<<inode<<", "<<__func__<<" : "<< __LINE__<<" ack address:0x"<<hex<<addr_from_mem<<" reqcount:"<<req_count_from_mem<<dec<<" Q size:"<<ReqQ.size()<<endl;
			ReqFormat_LLC ReQ2;
			ReQ2.address = addr_from_mem;
			ReQ2.req_count = req_count_from_mem;
			if(ack_rw_from_mem == true)
			{ 					
			ReQ2.we = true;					
			}
			else
			{ // read case
			ReQ2.we = false;	
			data_mem->read_data(&ReQ2.data);
			}
			ReQ2.type = ReqFormat_LLC::ACK;
			ReqQ.push(ReQ2);
			/*if(inode == 0x0){
			cout<<endl<<"$$$$$$$$$$$$$$$$$$$$$"<<endl;
			//cout<<"llc: "<<hex<<"0x"<<ReQ.address<<" @node:"<<inode<<" hit = "<<hit<<dec<<endl;
			for (char j = 0; j< LLC_SET_COUNT; j++) {
			for (char i = 0; i< LLC_WAYS; i++) {
			//cout<<"LLC::"<<inode<<", "<<__func__<<" In loc["<<hex<<(unsigned int)i<<"]["<< (unsigned int)j<<"]: "<< LLCtag[0][1][i][j]<<" tag is:"<< LLCtag[0][0][i][j]<<dec<<endl;
			cout<<"LLC::"<<inode<<", "<<__func__<<" In loc["<<hex<<(unsigned int)i<<"]["<< (unsigned int)j<<"]: "<< (unsigned int) LLCtag[0][1][i][j] <<" tag is:"<< LLCtag[0][0][i][j] <<" LRU state: "<< LLCtag[0][2][i][j] <<dec<<endl;
			}
			}
			cout<<endl<<"$$$$$$$$$$$$$$$$$$$"<<endl;
			} */
		}
		if (ce_ == true)
		{
			ReqFormat_LLC ReQ;
			ReQ.address = addr_f_dir;
			ReQ.req_count = req_count_f_dir;
			if(rd_wr_ == true)
			{ 					
			ReQ.we = true;					
			ReQ.type = ReqFormat_LLC::WR;									
			}
			else
			{ // read case
			ReQ.we = false;
			ReQ.type = ReqFormat_LLC::RD;						
			}
			ReQ.dirack_en = !without_dir_ack;
			ReQ.npc_node = dst_nodeaddr_;
			ReQ.dir_node = node_address_from_lru;
			cout<<"LLC::"<<inode<<", "<<__func__<<" : "<< __LINE__<<" DIR Req Q size:"<<ReqQ.size()<<" addr:0x"<<hex<<addr_f_dir<<dec<<endl;
			ReqQ.push(ReQ); 
			/*if(inode == 0x0) {
			cout<<endl<<"$$$$$$$$$$$$$$$$$$$$$"<<endl;
			//cout<<"llc: "<<hex<<"0x"<<ReQ.address<<" @node:"<<inode<<" hit = "<<hit<<dec<<endl;
			for (char j = 0; j< LLC_SET_COUNT; j++) {
			for (char i = 0; i< LLC_WAYS; i++) {
			//cout<<"LLC::"<<inode<<", "<<__func__<<" In loc["<<hex<<(unsigned int)i<<"]["<< (unsigned int)j<<"]: "<< LLCtag[0][1][i][j]<<" tag is:"<< LLCtag[0][0][i][j]<<dec<<endl;
			cout<<"LLC::"<<inode<<", "<<__func__<<" In loc["<<hex<<(unsigned int)i<<"]["<< (unsigned int)j<<"]: "<< (unsigned int) LLCtag[0][1][i][j] <<" tag is:"<< LLCtag[0][0][i][j] <<" LRU state: "<< LLCtag[0][2][i][j] <<dec<<endl;
			}
			}
			cout<<endl<<"$$$$$$$$$$$$$$$$$$$"<<endl;
			} */
		}
	wait();
	}
}

void LLC::LLC_main_thread(void){
	//cout<<"LLC::"<<inode<<", "<<__func__<<" : "<< __LINE__<<endl;
	while (1) {
		ReqFormat_LLC ReQ;
		char way = 0;
		bool hit;
		bool self;
		unsigned int set;
		unsigned int tag;
		unsigned int coopert_section=0x0;
		unsigned int state=0x0;
		bool WB_enable = false;
		while(ReqQ.empty() == true) wait();
		//cout<<"LLC::"<<inode<<", "<<__func__<<" : "<< __LINE__<<endl;
		ReQ = ReqQ.front();
		ReqQ.pop();		
		hit = llc_lookup(ReQ.address, &way, &self);
		coopert_section = 0x0; //find_coop_sect(ReQ.address);
		set = (ReQ.address >> (CL_SIZE + TOTAL_NO_OF_NODES_CARRYING_LLC_2 + D_O_C))&(LLC_SET_COUNT-1);
		tag = ReQ.address >> (CL_SIZE + TOTAL_NO_OF_NODES_CARRYING_LLC_2 + D_O_C + LLC_SET_COUNT_2);	
		state = (unsigned int) LLCtag[coopert_section][1][way][set];
		switch (ReQ.type) {
			case ReqFormat_LLC::RD:{
					if (hit == false) {
						datatype data;
						if (find_in_blockedQ(ReQ.address) == true){
							ReqQ.push(ReQ);
							cout<<"LLC::"<<inode<<", "<<__func__<<" : "<< __LINE__<<" RDMiss address blocked already!"<<endl;
						} else{
						if(find_way(ReQ.address,&way,ReQ.req_count,&WB_enable) == false){
							ReqQ.push(ReQ);
							cout<<"LLC::"<<inode<<", "<<__func__<<" : "<< __LINE__<<" couldnt find way!"<<endl;
						} 
						else {
							cout<<"LLC::"<<inode<<", "<<__func__<<" : "<< __LINE__<<" RDMiss: address"<<hex<<ReQ.address<<dec<<endl;
							llc_miss_count++;
							if(WB_enable == false){
								LLCtag[coopert_section][0][way][set] = (unsigned int) tag;
								LLCtag[coopert_section][1][way][set] = (unsigned int) ReqFormat_LLC::TRANS_IS;
								rdwr_to_mem(ReQ.address,NULL,ReQ.req_count, false,ReQ.npc_node,ReQ.dir_node,way,ReQ.dirack_en);	
								cout<<"LLC::"<<inode<<", "<<__func__<<" : "<< __LINE__<<" RD req sent address:"<<hex<<ReQ.address<<" tag:"<<(unsigned int)LLCtag[coopert_section][0][way][set]<<" set:"<<set <<" state:"<<(unsigned int)LLCtag[coopert_section][1][way][set] <<dec<<endl;
							}
							else {
								ReqFormat_LLC BReQ;
								BReQ.address = ReQ.address;
								BReQ.type = ReqFormat_LLC::RD;
								BReQ.we = false;
								BReQ.way = way;
								BReQ.data = ReQ.data;
								BReQ.npc_node = ReQ.npc_node;
								BReQ.dir_node = ReQ.dir_node;
								BReQ.dirack_en = ReQ.dirack_en;
								BReQ.linked = false;
								BReQ.req_count = ReQ.req_count;
								BlockedQ.push(BReQ);								
							}
						}
						}
					} 
					else {					
						if ((hit == true) && (state > (unsigned int)(ReqFormat_LLC::REG_M))) { // trans state
							ReqQ.push(ReQ);
						}
						else {
							llc_hit_count++; 
							cout<<"LLC::"<<inode<<", "<<__func__<<" : "<< __LINE__<<" RDHit address"<<hex<<ReQ.address<<dec<<endl;							
							for(char j=0; j<LLC_WAYS; j++)
							{
								if (j == way) continue;
								if (LLCtag[coopert_section][2][j][set] > LLCtag[coopert_section][2][way][set])
								{
								LLCtag[coopert_section][2][j][set] = LLCtag[coopert_section][2][j][set] - 1;
								}
							}
							if(self == true) {
								//LLCtag[0][0][way][set] = tag;
								LLCtag[0][2][way][set] = (LLC_WAYS -1); //0x7;
							}
							else {
								//LLCtag[coopert_section][0][way][set] = tag;
								LLCtag[coopert_section][2][way][set] = (LLC_WAYS -1); //0x7;
							}
							ack_to_npc_dir(ReQ.address,&LLCMem[coopert_section][way][set],ReQ.req_count,false,ReQ.npc_node,ReQ.dir_node, ReQ.dirack_en);
						}
					}
				} break;
			case ReqFormat_LLC::WR:{
				bool found;
				found = false;
				if((ReqQ_data.empty() == true) || ((hit == true) && (state > (unsigned int)(ReqFormat_LLC::REG_M)))){ // trans state or no data
					found = false;
				} 
				else {
				//	cout<<"LLC::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<" WR l2 data received for address: 0x"<<hex<< ReQ.address << " from l2: "<<ReQ.npc_node<<dec<<endl;
					ReqData_LLC front,tmp;					
					tmp = ReqQ_data.back();					
					do {
						front = ReqQ_data.front();
						if((front.address == ReQ.address) || (front.npc_node == ReQ.npc_node) || (front.req_count == ReQ.req_count)){
							if (hit == false){
								if(find_way(ReQ.address,&way,ReQ.req_count,&WB_enable) == false){
									ReqQ_data.pop();
									ReqQ_data.push(front);
									break;
								} 
								else{									
									found = true;
									llc_miss_count ++;
									if(WB_enable == false){									
										LLCMem[coopert_section][way][set]=front.data;
										LLCtag[coopert_section][1][way][set] = (unsigned int)(ReqFormat_LLC::REG_M);
										LLCtag[coopert_section][0][way][set] = tag;
										cout<<"LLC::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<endl;
										for(char j=0; j<LLC_WAYS; j++)
										{
											if (j == way) continue;
											if (LLCtag[coopert_section][2][j][set] > LLCtag[coopert_section][2][way][set])
											{
											LLCtag[coopert_section][2][j][set] = LLCtag[coopert_section][2][j][set] - 1;
											}
										}
										LLCtag[coopert_section][2][way][set] = (LLC_WAYS -1); //0x7;
										ack_to_npc_dir(ReQ.address,NULL,ReQ.req_count,true,ReQ.npc_node,ReQ.dir_node, ReQ.dirack_en);
									}
									else {
										ReqFormat_LLC BReQ;
										BReQ.address = ReQ.address;
										BReQ.type = ReqFormat_LLC::WR;
										BReQ.we = true;
										BReQ.way = way;
										BReQ.data = ReQ.data;
										BReQ.npc_node = ReQ.npc_node;
										BReQ.dir_node = ReQ.dir_node;
										BReQ.dirack_en = ReQ.dirack_en;
										BReQ.linked = false;
										BReQ.req_count = ReQ.req_count;
										BlockedQ.push(BReQ);									
									}
									if(front != tmp)
										ReqQ_data.pop();
									else{
										ReqQ_data.pop();
										break;
									}
								}
							} 
							else if ((hit == true) && (state > (unsigned int)(ReqFormat_LLC::REG_M))) { // trans state
								cout<<"LLC::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<" Error"<<endl;
								ReqQ_data.pop();
								ReqQ_data.push(front);
							} 
							else {
								found = true;
								llc_hit_count ++;
								cout<<"LLC::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<" Hit S/M"<<endl;								
								for(char j=0; j<LLC_WAYS; j++)
								{
									if (j == way) continue;
									if (LLCtag[coopert_section][2][j][set] > LLCtag[coopert_section][2][way][set])
									{
									LLCtag[coopert_section][2][j][set] = LLCtag[coopert_section][2][j][set] - 1;
									}
								}
								if(self == true) {
									LLCMem[0][way][set]=front.data;
									LLCtag[0][1][way][set] = (unsigned int)(ReqFormat_LLC::REG_M);
									//LLCtag[0][0][way][set] = tag;
									LLCtag[0][2][way][set] = (LLC_WAYS -1); //0x7;
								}
								else {
									LLCMem[coopert_section][way][set]=front.data;
									LLCtag[coopert_section][1][way][set] = (unsigned int)(ReqFormat_LLC::REG_M);
									//LLCtag[coopert_section][0][way][set] = tag;
									LLCtag[coopert_section][2][way][set] = (LLC_WAYS -1); //0x7;
								}
								ack_to_npc_dir(ReQ.address,NULL,ReQ.req_count,true,ReQ.npc_node,ReQ.dir_node, ReQ.dirack_en);																
								if(front != tmp)
									ReqQ_data.pop();
								else{
									ReqQ_data.pop();
									break;
								}
							}													
						}
						else {
							ReqQ_data.pop();
							ReqQ_data.push(front);
						}
					} while(front != tmp);						
				} 
				if(found == false) {
					ReqQ.push(ReQ);	
				} else
					cout<<"LLC::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<" ReqQ size:"<<ReqQ.size()<<" DataQsize:"<< ReqQ_data.size()<< endl;
			} break;
			case ReqFormat_LLC::ACK:{
				//cout<<"LLC::"<<inode<<", "<<__func__<<" : "<< __LINE__<<" : "<< __LINE__<<" address:"<<hex<<ReQ.address<<dec<<" req_count:"<<hex<<ReQ.req_count<<dec<<endl;
				if((BlockedQ.empty() == true) || (hit == false) || (state < (unsigned int)(ReqFormat_LLC::TRANS_MI))) {
					if(BlockedQ.empty() == true) cout<<"LLC::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<"Error1!!"<<endl;	
					if((hit == false)) cout<<"LLC::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<"Error2!!"<<endl;	
					if(state < (unsigned int)(ReqFormat_LLC::TRANS_MI)) cout<<"LLC::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<"Error3!! state="<<(unsigned int) state<<endl;						
				} else {
					ReqFormat_LLC front;
					ReqFormat_LLC back = BlockedQ.back();
					do{
						front = BlockedQ.front();
						//cout<<"LLC::"<<inode<<", "<<__func__<<" : "<< __LINE__<<" : "<< __LINE__<<" address:"<<hex<<front.address<<dec<<" req_count:"<<hex<<front.req_count<<dec<<endl;
						if(((ReQ.address) == (front.address)) && (ReQ.req_count == front.req_count)&& ((front.type == (unsigned int)ReqFormat_LLC::WR_BACK) || (front.type == (unsigned int)ReqFormat_LLC::RD))){
							cout<<"LLC::"<<inode<<", "<<__func__<<" : "<< __LINE__<<" ACK address"<<hex<<ReQ.address<<dec<<endl;
							state = LLCtag[coopert_section][1][front.way][set];
							if((ReQ.we == true) && (front.type == (unsigned int)ReqFormat_LLC::WR_BACK) && (state == (unsigned int)(ReqFormat_LLC::TRANS_MI))){
								BlockedQ.pop(); // data is written and ack is received 
								front = BlockedQ.front();
								cout<<"LLC::"<<inode<<", "<<__func__<<" : "<< __LINE__<<" WB"<<endl;
								if(front.type == (unsigned int)ReqFormat_LLC::RD){
									LLCtag[coopert_section][1][front.way][set] = (unsigned int)(ReqFormat_LLC::TRANS_IS);
									LLCtag[coopert_section][0][front.way][set] = front.address >> (CL_SIZE + TOTAL_NO_OF_NODES_CARRYING_LLC_2 + D_O_C + LLC_SET_COUNT_2);
									rdwr_to_mem(front.address,NULL,front.req_count, false,front.npc_node,front.dir_node,front.way, front.dirack_en);
									if(back != front)
										BlockedQ.pop();
									else{
										BlockedQ.pop();
										break;
									}	
								} 
								else if(front.type == (unsigned int)ReqFormat_LLC::WR) {
									LLCMem[coopert_section][front.way][set]= front.data;
									LLCtag[coopert_section][1][front.way][set] = (unsigned int)(ReqFormat_LLC::REG_M);
									LLCtag[coopert_section][0][front.way][set] = front.address >> (CL_SIZE + TOTAL_NO_OF_NODES_CARRYING_LLC_2 + D_O_C + LLC_SET_COUNT_2);
									for(char j=0; j<LLC_WAYS; j++)
									{
										if (j == front.way) continue;
										if (LLCtag[coopert_section][2][j][set] > LLCtag[coopert_section][2][front.way][set])
										{
										LLCtag[coopert_section][2][j][set] = LLCtag[coopert_section][2][j][set] - 1;
										}
									}
									LLCtag[coopert_section][2][front.way][set] = (LLC_WAYS -1); //0x7;
									ack_to_npc_dir(front.address,NULL,front.req_count,true,front.npc_node,front.dir_node,front.dirack_en);												
									if(back != front)
										BlockedQ.pop();
									else{
										BlockedQ.pop();
										break;
									}								
								}
								else {
									cout<<"LLC::"<<inode<<", "<<__func__<<" : "<< __LINE__<<" Error!"<<endl;
									BlockedQ.pop();
									BlockedQ.push(front);
								}
							} 
							else if ((ReQ.we == false) && (front.type == (unsigned int)ReqFormat_LLC::RD) && (state == (unsigned int)(ReqFormat_LLC::TRANS_IS))){
								LLCMem[coopert_section][front.way][set]= ReQ.data;
								LLCtag[coopert_section][1][front.way][set] = (unsigned int)(ReqFormat_LLC::REG_S);
								LLCtag[coopert_section][0][front.way][set] = front.address >> (CL_SIZE + TOTAL_NO_OF_NODES_CARRYING_LLC_2 + D_O_C + LLC_SET_COUNT_2);
								for(char j=0; j<LLC_WAYS; j++)
								{
									if (j == front.way) continue;
									if (LLCtag[coopert_section][2][j][set] > LLCtag[coopert_section][2][front.way][set])
									{
									LLCtag[coopert_section][2][j][set] = LLCtag[coopert_section][2][j][set] - 1;
									}
								}
								LLCtag[coopert_section][2][front.way][set] = (LLC_WAYS -1); //0x7;
								cout<<"LLC::"<<inode<<", "<<__func__<<" : "<< __LINE__<<" RDACK"<<endl;
								ack_to_npc_dir(front.address,&LLCMem[coopert_section][front.way][set],front.req_count,false,front.npc_node,front.dir_node, front.dirack_en);												
								if(back != front)
									BlockedQ.pop();
								else{
									BlockedQ.pop();
									break;
								}
							}  else{
								BlockedQ.pop();
								BlockedQ.push(front);
								cout<<"LLC::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<"Error:!!" <<endl;
							}							
						} else{
							BlockedQ.pop();
							BlockedQ.push(front);
						}
					} while(front != back);
				}				
			} break;
			default: 
					cout<<"LLC::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<"Error:!!" <<endl;
		}
		wait();
	}
}

