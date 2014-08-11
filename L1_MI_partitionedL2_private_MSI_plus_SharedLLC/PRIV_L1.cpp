
#include "PRIV_L1.h"

#define LOOKUP_DELAY_PRIV_L1 1

void PRIV_L1::PRIV_L1_init(void){
	int i=PRIV_L1_WAYS;
	//cout<<"PRIV_L1::"<<__func__<<" : "<< __LINE__<<endl;
	for (int i = 0; i< PRIV_L1_WAYS; i++){
	PRIV_L1Mem[i] =  new datatype [PRIV_L1_SET_COUNT];
	memset(PRIV_L1Mem[i], 0x0,PRIV_L1_SET_COUNT);
	for(int j=0;j<PRIV_L1_SET_COUNT;j++)
		{
		PRIV_L1tag[2][j][i] = 0x0;
		PRIV_L1tag[1][j][i] = 0x0;
		PRIV_L1tag[0][j][i] = 0x0;
		}
	}
	l1_miss_count = 0x0;
	l1_hit_count = 0x0;
}
bool PRIV_L1::find_in_blockedQ(unsigned int address)
{
	bool found;
	found = false;
	if(BlockedQ.empty() == false) {
		ReqFormat_PrivL1 front;
		ReqFormat_PrivL1 back = BlockedQ.back();
		
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
bool PRIV_L1::table_lookup(int address, char * hit_way, int * state){
	 
	// 8 sets of 8 way associative 8 word cl = 5 bit offset + 10 bit set  + 3 bit in each way for lru = ttl 256kb **** as of now 32sets => 5 bit
	//PRIV_L1tag[0][set][way] = tag, PRIV_L1tag[1][set][way] = 2 bit state I 00 /V 01/M 10, PRIV_L1tag[2][set][way] = 3 bit lru */ 
	bool hit = false;
	if((hit_way == NULL) || (state == NULL)) {
		cout << inode <<" : @PRIV_L1::"<<__func__ <<" : "<< __LINE__<< " error: way not allocated "<<endl;
	}
	int set = (address >> CL_SIZE) & ((unsigned int)PRIV_L1_SET_COUNT-1); //tag/set/offset , where offset = 5 (8 words in a cl) +  (10 bits of set) 1024*8*8*4 bytes, ttl size of the PRIV_L1 cache = 256kb 
	int tag = (address) >> (PRIV_L1_SET_COUNT_2 + CL_SIZE);
	for(char i=0; i<PRIV_L1_WAYS; i++)
	{
		if((PRIV_L1tag[0][set][i] == tag) && ((PRIV_L1tag[1][set][i] > 0x0))) // 00: invalid, 10: Invalid to Valid (Rd from L2), 01: valid, 11: Valid to Valid (Wr to L2)
		{
			hit = true;
			*hit_way = (char) i;
			*state = (int) PRIV_L1tag[1][set][i];			
			break;
		} else if((PRIV_L1tag[0][set][i] == tag) && ((PRIV_L1tag[1][set][i] == 0x0)))	{
			*state = (int) PRIV_L1tag[1][set][i];
		}		
	}

	return hit;
}

char PRIV_L1::find_free_way (int address,int req_count, bool *status) {
	int set = (address >> CL_SIZE) & ((unsigned int)PRIV_L1_SET_COUNT-1); 
	int tag = (address) >> (PRIV_L1_SET_COUNT_2 + CL_SIZE);
	char j = 0;	
	unsigned int LRUvalue = 0x0;
	bool found;
	//cout<<"PRIV_L1::"<<inode << " : "<<__func__<<" : "<< __LINE__<<endl;
	found = false;
	for(; LRUvalue<PRIV_L1_WAYS; LRUvalue++) {
		for(; j<PRIV_L1_WAYS; j++)
		{
			if (PRIV_L1tag[2][set][j] == LRUvalue)
			{
				if(PRIV_L1tag[1][set][j] > 0x1) // trans state dont remove this /*either wait for this to get proper state (probable deadlock) or take LRU number 1*/
				{
					continue;
				}
				else
				{
					//cout<<"PRIV_L1::"<<inode << " : "<<__func__<<" : "<< __LINE__<<endl;
					found = true;
					PRIV_L1tag[1][set][j] = 0x0;
				}
			break;
			}
			
		}
		if(j == PRIV_L1_WAYS)  {
			//cout << inode <<" : @PRIV_L1::"<<__func__ <<" : "<< __LINE__<<" Error No way with LRU 0!!"<<endl;
			j = 0;	
		} else 
			break;
	}
	if(found == false) {
		//cout << inode <<" : @PRIV_L1::"<<__func__ <<" : "<< __LINE__<<" Error No way with LRU minimum and without trans!!"<<endl;
		*status = found;
	}
	return j;
}

bool PRIV_L1::read_write_to_l2 (int address, bool RD_WR, datatype* data,int req_count, char way){
	//cout<<"PRIV_L1::"<<inode << " : "<<__func__<<" : "<< __LINE__<<endl;	
	int set = (address >> CL_SIZE) & ((unsigned int)PRIV_L1_SET_COUNT-1); 
	int tag = (address) >> (PRIV_L1_SET_COUNT_2 + CL_SIZE);
	rd_wr_to_l2 = RD_WR;
	ce_to_l2 = true;
	address_to_l2 = address;
	req_count_to_l2 = req_count;
	if((RD_WR) && (data != NULL))
		data_to_l2->write_data(data);
	wait();
	ce_to_l2 = false;
	wait();
	ReqFormat_PrivL1 ReQ;
	ReQ.address = address;
	ReQ.req_count = req_count;
	ReQ.way = way;
	if(RD_WR){
		ReQ.type = ReqFormat_PrivL1::WR;
		ReQ.ex = true;
		ReQ.we = true;
		ReQ.data = *data;
		PRIV_L1tag[1][set][way] = 0x03; //Wr through to L2
	} else {
		ReQ.type = ReqFormat_PrivL1::RD;
		ReQ.ex = false;
		ReQ.we = false;
		PRIV_L1tag[1][set][way] = 0x02; //rd from L2
	}
	PRIV_L1tag[0][set][way] = (address) >> (PRIV_L1_SET_COUNT_2 + CL_SIZE);
	for(char j=0; j<PRIV_L1_WAYS; j++)
	{
		if (j == way) continue;
		if (PRIV_L1tag[2][set][j] > PRIV_L1tag[2][set][way])
		{
		PRIV_L1tag[2][set][j] = PRIV_L1tag[2][set][j] - 1;
		}
	}
	PRIV_L1tag[2][set][way] = PRIV_L1_WAYS-1;
	BlockedQ.push(ReQ);				
}

bool PRIV_L1::invalidating_cl (int address ,int way, int req_count, unsigned int state, ReqFormat_PrivL1::ReQType tp)
{
	int set = (address >> CL_SIZE) & ((unsigned int)PRIV_L1_SET_COUNT-1); 
	int tag = (address) >> (PRIV_L1_SET_COUNT_2 + CL_SIZE);
	if ((state == 0x0) || (state == 0x1))  {//Invalid or valid
		//cout<<"PRIV_L1::"<<inode << " : "<<__func__<<" : "<< __LINE__<<"invalidate shared"<<endl;
		PRIV_L1tag[1][set][way] = 0x0;
		for(int j = 0;j<PRIV_L1_WAYS; j++)
		{
			//cout<<"PRIV_L1tag[2][set]: "<<PRIV_L1tag[2][set][j]<<endl;
			if (j== way) continue;
			if(PRIV_L1tag[2][set][j] < PRIV_L1tag[2][set][way]) // invalidated line is least preferred as cud be 
			{
				PRIV_L1tag[2][set][j] = PRIV_L1tag[2][set][j]+1;
			}		
			
		}
		PRIV_L1tag[2][set][way] = 0x0;
	} 
	else  //transition state
	{
		//search blockedQ		
		if(BlockedQ.empty() == true) 
		{
			PRIV_L1tag[1][set][way] = 0x0;
			cout<<"PRIV_L1::"<<inode << " : "<<__func__<<" : "<< __LINE__<<" INV 1"<<endl;
		} else
		{
			ReqFormat_PrivL1 front,fronttemp;
			ReqFormat_PrivL1 back = BlockedQ.back();
			do {
				front = BlockedQ.front();			
				if(((front.address >> CL_SIZE) == (address >> CL_SIZE)) && ((front.type == ReqFormat_PrivL1::WR) || (front.type == ReqFormat_PrivL1::RD)))
				{
					ADDRESS_DB_OUT = front.address;
					REQ_COUNT_OUT = front.req_count;
					ACK_DB = true;
					WE_DB_OUT = front.we;
					DATA_DB_OUT->write_data(&front.data);	
					STATUS_DB = false;
					wait();
					ACK_DB = false;		
					wait();				
					PRIV_L1tag[1][set][way] = 0x0;		
					cout<<"PRIV_L1::"<<inode << " : "<<__func__<<" : "<< __LINE__<<" INV 2"<<endl;						
					for(int j = 0;j<PRIV_L1_WAYS; j++)
					{
						//cout<<"PRIV_L1tag[2][set]: "<<PRIV_L1tag[2][set][j]<<endl;
						if (j== way) continue;
						if(PRIV_L1tag[2][set][j] < PRIV_L1tag[2][set][way]) // invalidated line is least preferred as cud be 
						{
							PRIV_L1tag[2][set][j] = PRIV_L1tag[2][set][j]+1;
						}		
						
					}
					PRIV_L1tag[2][set][way] = 0x0;
					if(back != front)
						BlockedQ.pop();
					else {
						BlockedQ.pop();
						break;
					}
				}					
				else 
				{
					BlockedQ.pop();
					BlockedQ.push(front);
				}
			} while(back != front);
		}
		//search in reqq
		if(ReqQ.empty() == true)
                {
                       // PRIV_L1tag[1][set][way] = 0x0;
                        //cout<<"PRIV_L1::"<<inode << " : "<<__func__<<" : "<< __LINE__<<" INV 1"<<endl;
                } else
                {
                        ReqFormat_PrivL1 front,fronttemp;
                        ReqFormat_PrivL1 back = ReqQ.back();
                        do {
                                front = ReqQ.front();
                                if(((front.address >> CL_SIZE) == (address >> CL_SIZE)) && ((front.type == ReqFormat_PrivL1::WR) || (front.type == ReqFormat_PrivL1::RD)))
                                {
                                        ADDRESS_DB_OUT = front.address;
                                        REQ_COUNT_OUT = front.req_count;
                                        ACK_DB = true;
                                        WE_DB_OUT = front.we;
                                        DATA_DB_OUT->write_data(&front.data);
                                        STATUS_DB = false;
                                        wait();
                                        ACK_DB = false;
					wait();
                                        PRIV_L1tag[1][set][way] = 0x0;
                                        cout<<"PRIV_L1::"<<inode << " : "<<__func__<<" : "<< __LINE__<<" INV 2"<<endl;                                       
                                        for(int j = 0;j<PRIV_L1_WAYS; j++)
                                        {
                                                //cout<<"PRIV_L1tag[2][set]: "<<PRIV_L1tag[2][set][j]<<endl;
                                                if (j== way) continue;
                                                if(PRIV_L1tag[2][set][j] < PRIV_L1tag[2][set][way]) // invalidated line is least preferred as cud be
                                                {
                                                        PRIV_L1tag[2][set][j] = PRIV_L1tag[2][set][j]+1;
                                                }

                                        }
                                        PRIV_L1tag[2][set][way] = 0x0;
                                        if(back != front)
                                                ReqQ.pop();
                                        else {
                                                ReqQ.pop();
                                                break;
                                        }
                                }
                                else
                                {
                                        ReqQ.pop();
                                        ReqQ.push(front);
                                }
                        } while(back != front);
                }
			
	} 	
	return true;	
}

void PRIV_L1::request_collector(void){
	//cout<<"PRIV_L1::"<< inode << " : "<<__func__<<" : "<< __LINE__<<dec<<endl;	
	enum state {IDLE, INV, RD_WR, ACK_ANY} st,st1;
	char way;
	int state;
	bool hit, hit1, hit2;
	st1 = IDLE;
	st = IDLE;
	int i;
	while(1){
	i = 3;
	while (i>0)
	{
		switch (st) {
		case IDLE: {					
					
					if((CE_DB == true) || (inv_from_l2 == true) || (ack_from_l2 == true)){
						//cout << inode <<" : @PRIV_L1::"<<__func__ <<" : "<< __LINE__<<"ack/inv with hit or ce"<< endl;
						if ((CE_DB == true) && ((inv_from_l2 == true) || (ack_from_l2 == true)))
						{
							if((inv_from_l2 == true) && (ack_from_l2 == false))
							{	
								//cout << inode <<" : @PRIV_L1::"<<__func__ <<" : "<< __LINE__<<endl;	
								st = INV;
								st1 = RD_WR;
							} else if ((ack_from_l2 == true) && (inv_from_l2 == false))
							{	
								//cout << inode <<" : @PRIV_L1::"<<__func__ <<" : "<< __LINE__<<endl;	
								st = ACK_ANY;								
								st1 = RD_WR;
							} 
							else 
							{	
								//cout << inode <<" : @PRIV_L1::"<<__func__ <<" : "<< __LINE__<<endl;	
								st = RD_WR;
								st1 = IDLE;
							}
						}
						else if ((inv_from_l2 == true))
						{
							//cout << inode <<" : @PRIV_L1::"<<__func__ <<" : "<< __LINE__<<" @time: "<<sc_time_stamp()<<"@priv_l1 CE is "<<CE_DB<<endl;
							st = INV;
							st1 = IDLE;	
						}
						else if (ack_from_l2 == true) 
						{
							st = ACK_ANY;
							st1 = IDLE;
						}
						else if (CE_DB == true)
						{
							st = RD_WR;
							st1 = IDLE;
						}
						else
						{
							cout<<inode<<" : "<<"PRIV_L1::"<<__func__<<" : "<< __LINE__<<" Error!!"<<endl;
							i = 1; 
							st = IDLE;
							st1 = IDLE;
						}							
						
					} 
					else
					{
						i = 1; 
						st = IDLE;
						st1 = IDLE;
					}					
					} break;
		case INV: {
					ReqFormat_PrivL1 ReQ1;
					ReQ1.address = address_from_l2;
					ReQ1.req_count = req_count_from_l2; 
					ReQ1.type = ReqFormat_PrivL1::INV;
					cout<<"PRIV_L1::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<" INV for address:0x"<<hex<<address_from_l2<<dec<<endl;					
					ReqQ.push(ReQ1);
					st = st1;
					if(st1 == IDLE) 
						i = 1;			
					} break;
		case RD_WR: {
					ReqFormat_PrivL1 ReQ2;
					ReQ2.address = ADDRESS_DB_IN;
					ReQ2.req_count = REQ_COUNT_IN;
					//ReQ.way = way;
					if(WE_DB_IN == true)
					{ 						
						ReQ2.we = true;
						ReQ2.ex = true;
						ReQ2.type = ReqFormat_PrivL1::WR;
						DATA_DB_IN->read_data(&ReQ2.data);
						cout<<"PRIV_L1::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<" WR"<<endl;	
					}
					else
					{ // read case
						ReQ2.we = false;
						ReQ2.ex = false;
						ReQ2.type = ReqFormat_PrivL1::RD;
						cout<<"PRIV_L1::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<" RD"<<endl;				
					}
					ReqQ.push(ReQ2); 
					st = IDLE;
					st1 = IDLE;
					i = 1;	
					} break;
		case ACK_ANY:{
					ReqFormat_PrivL1 ReQ3;
					if (ack_from_l2 == true) {
						ReQ3.type = ReqFormat_PrivL1::ACK_L2;
						ReQ3.address = address_from_l2;
						ReQ3.req_count = req_count_from_l2; //((inode<<3) | 0x0);	
						ReQ3.we = ack_we_l2;
						if(!(ack_we_l2))
							data_from_l2->read_data(&ReQ3.data);
						cout<<"PRIV_L1::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<" ACK from L2"<<endl;
					}	
					ReqQ.push(ReQ3);
					st = st1;
					if(st1 == IDLE) 
						i = 1;					
					} break;			
		default: { 
				cout<<"PRIV_L1::"<<inode<<" : "<<__func__<<" : "<< __LINE__<< "Error!!, state:" <<st<<endl; 
				st = st1;
				if(st1 == IDLE) 
					wait(); 
			}
		}
		i--;
	}
	wait();
	}
}

void PRIV_L1::priv_l1_main_thread(void){

	char way;
	int state;
	bool hit;
	int set;
	ReqFormat_PrivL1 progrs_ReQ;
	
	while (1) {	
		while(ReqQ.empty() == true) wait();
		state = 0x0;
		hit = false;
		set = 0;
		way = 0;
		//cout<<"PRIV_L1::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<endl;	
		progrs_ReQ = ReqQ.front();
		ReqQ.pop();
		hit = table_lookup(progrs_ReQ.address, &way, &state);
		set = (progrs_ReQ.address >> CL_SIZE) & ((unsigned int)PRIV_L1_SET_COUNT-1);
		switch (progrs_ReQ.type) {
			case ReqFormat_PrivL1::INV: {
					bool ret = false;
					if((hit == true)) 
						invalidating_cl(progrs_ReQ.address,way,progrs_ReQ.req_count, state,progrs_ReQ.type);
				} break;
			case ReqFormat_PrivL1::ACK_L2:{
					cout<<flush;	
					if ((hit == false) || (state == 0x1)) {
						if(state == 0x0)
						cout << inode <<" : @PRIV_L1::"<<__func__ <<" : "<< __LINE__<<" State:0 hit:"<<(unsigned int) hit <<"address:"<<hex<<progrs_ReQ.address<<dec<<endl;
					}
					else{
						cout << inode <<" : @PRIV_L1::"<<__func__ <<" : "<< __LINE__<<"ACK received Qsize"<<(unsigned int)BlockedQ.size()<<endl;
						if(BlockedQ.empty() == true) 
						{
							cout << inode <<" : @PRIV_L1::"<<__func__ <<" : "<< __LINE__<<"ACK received for non-blocked req"<< endl;
						} else
						{
							ReqFormat_PrivL1 front;
							ReqFormat_PrivL1 back = BlockedQ.back();
							do {
								front = BlockedQ.front();			
								if(((front.address >> CL_SIZE) == (progrs_ReQ.address >> CL_SIZE)) && (front.req_count == progrs_ReQ.req_count))
								{
									/*
									if RD ack received, 									 
									save the data thr 
									respond to pr
									
									if WR ack received,
									save the data thr 
									respond to pr
									*/									
									if (((front.type == ReqFormat_PrivL1::RD) && (progrs_ReQ.we == false) && (state == 0x2)) || ((front.type == ReqFormat_PrivL1::WR) && (progrs_ReQ.we == true) && (state == 0x3))){ 
										cout << inode <<" : @PRIV_L1::"<<__func__ <<" : "<< __LINE__<< "ACK type1 state: "<< (unsigned int)state<<" address:"<<hex<<progrs_ReQ.address<<dec<<endl;
										if((front.type == ReqFormat_PrivL1::RD)){											
											PRIV_L1Mem[front.way][set] = progrs_ReQ.data;
											DATA_DB_OUT->write_data(&PRIV_L1Mem[front.way][set]);
											WE_DB_OUT = false;
										} else {
											PRIV_L1Mem[front.way][set] = front.data;
											WE_DB_OUT = true;
										}										
										for(char j=0; j<PRIV_L1_WAYS; j++)
										{
											if (j == front.way) continue;
											else if (PRIV_L1tag[2][set][j] != 0)
											{
											PRIV_L1tag[2][set][j] = PRIV_L1tag[2][set][j] - 1;
											}
										}
										PRIV_L1tag[2][set][front.way] = PRIV_L1_WAYS-1;
										PRIV_L1tag[1][set][front.way] = 0x1;
										PRIV_L1tag[0][set][front.way] = (progrs_ReQ.address) >> (PRIV_L1_SET_COUNT_2 + CL_SIZE);
										REQ_COUNT_OUT = progrs_ReQ.req_count;
										ADDRESS_DB_OUT = progrs_ReQ.address;
										STATUS_DB = true;
										ACK_DB = true;													
										wait();
										ACK_DB = false;	
										wait();
										if(back != front)
											BlockedQ.pop();
										else {
											BlockedQ.pop();
											break;
										}								
										
									} 
								} 								
								else 
								{
									BlockedQ.pop();
									BlockedQ.push(front);
								}
							} while(back != front);
							cout << inode <<" : @PRIV_L1::"<<__func__ <<" : "<< __LINE__<< "ACK last"<<endl;
						}	
						
					} 					
					
					cout << inode <<" : @PRIV_L1::"<<__func__ <<" : "<< __LINE__<< "ACK done"<<endl;
					} break;
			case ReqFormat_PrivL1::RD:
			case ReqFormat_PrivL1::WR:{
					cout<<flush;					
					if ((hit == true) && (state == 0x01)) {						
						cout<<"PRIV_L1::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<"hit"<<endl;						
						if(progrs_ReQ.type == ReqFormat_PrivL1::RD) {							
							wait();
							ADDRESS_DB_OUT = progrs_ReQ.address;
							REQ_COUNT_OUT = progrs_ReQ.req_count;
							ACK_DB = true;
							STATUS_DB = true;
							DATA_DB_OUT->write_data(&PRIV_L1Mem[way][set]);
							WE_DB_OUT = false;
							wait();
							ACK_DB = false;	
							wait();
							for(char j=0; j<PRIV_L1_WAYS; j++)
							{
								if (j == way) continue;
								if (PRIV_L1tag[2][set][j] > PRIV_L1tag[2][set][way])
								{
								PRIV_L1tag[2][set][j] = PRIV_L1tag[2][set][j] - 1;
								}
							}
							PRIV_L1tag[2][set][way] = PRIV_L1_WAYS-1;
							l1_hit_count ++;
						} else {
						       if (find_in_blockedQ(progrs_ReQ.address) == true){
                                                        ReqQ.push(progrs_ReQ);
                                                	} else {
							l1_hit_count ++;
							read_write_to_l2(progrs_ReQ.address, true,&progrs_ReQ.data,progrs_ReQ.req_count,way);									     }
						}
												
					} 
					else if (hit == true) {
					// address in transition state, push the request to back 
						ReqQ.push(progrs_ReQ);
						//cout<<"PRIV_L1::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<"hit T"<<endl;	
					}
					else {	// miss case
						bool wb_enable = true;
						bool status = true;
						if (find_in_blockedQ(progrs_ReQ.address) == true){
							ReqQ.push(progrs_ReQ);
						} else {
						way = find_free_way(progrs_ReQ.address,progrs_ReQ.req_count,&status);						
						if(status == false)
							ReqQ.push(progrs_ReQ);
						else{
							l1_miss_count ++;
							cout<<"PRIV_L1::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<"hit Miss! Qsize"<<ReqQ.size()<<endl;					
							ReqFormat_PrivL1 ReQ;
							if(ReqFormat_PrivL1::WR == progrs_ReQ.type) {	
								read_write_to_l2(progrs_ReQ.address, true,&progrs_ReQ.data,progrs_ReQ.req_count,way);	
								cout<<"PRIV_L1::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<" step 4X"<<endl;
							} else {
								read_write_to_l2(progrs_ReQ.address, false,NULL,progrs_ReQ.req_count,way);
								cout<<"PRIV_L1::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<" step 4-"<<endl;
							}								
						}	
						}
					}						
					
					//cout<<"PRIV_L1::"<<inode<<" : "<<__func__<<" : "<< __LINE__<<"RDWR done"<<endl;	
					
				} break;	
			default: 
					cout << inode <<" : @PRIV_L1::"<<__func__ <<" : "<< __LINE__<< ": Error: PRIV_L1::main_thread !!" <<endl; wait();
		}	
	wait();
	//cout<<inode<<" : "<<"PRIV_L1::"<<__func__<<" : -- "<< __LINE__<<endl;	
	}
}


