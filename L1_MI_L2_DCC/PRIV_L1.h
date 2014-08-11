#ifndef PRIV_L1_H
#define PRIV_L1_H

#include <queue>
#include <iostream>
#include <systemc.h>
#include "write_read_if.h"
#include "datatype_ch.h"
#include "LSU_defines.h"

#define PRIV_L1_TAGSIZE 
#define PRIV_L1_SET_COUNT 256
#define PRIV_L1_SET_COUNT_2 8
#define QMAX 5
#define PRIV_L1_WAYS 0x2

//TODO: note all data widths needs to be changed from int to 8 word length
using namespace std;

class ReqFormat_PrivL1 {
public : 
enum ReQType {RD, WR, INV, RDX, ACK_L2};
	datatype data;
	int address;
	bool we;
	bool ex;
	int way;
	//int req_number;
	int req_count;
	ReQType type;
	ReqFormat_PrivL1(void) {
	req_count = 0x0;
	}
	 bool operator!=(const ReqFormat_PrivL1& other){
		if((this->address == other.address) && (this->req_count == other.req_count) && (this->type == other.type) && (this->we == other.we) && (this->way == other.way) )
			return false;
		else 
			return true;
	 }
};

SC_MODULE(PRIV_L1)
{
public:
	
// the l2 side 	
	sc_out <bool> ce_to_l2;
	sc_out <bool> rd_wr_to_l2;
	sc_out <int> address_to_l2;
	sc_out <int> req_count_to_l2;
	sc_out <bool> ack_to_l2;	
	sc_port<write_read_if> data_to_l2;	
	
	sc_in <int> req_count_from_l2;
	sc_in <int> address_from_l2;
	sc_port<write_read_if> data_from_l2;
	sc_in <bool> ack_from_l2;
	sc_in <bool> ack_we_l2;
	sc_in <bool> inv_from_l2;
	
// the proc side
	sc_in<bool> clk;
	sc_in <bool> CE_DB;
	sc_out <bool> ACK_DB;
	sc_port<write_read_if> DATA_DB_IN;
	sc_port<write_read_if> DATA_DB_OUT;
	sc_in<int> ADDRESS_DB_IN;
	sc_out<int> ADDRESS_DB_OUT;
	sc_in<int> REQ_COUNT_IN;
	sc_out<int> REQ_COUNT_OUT;
	sc_out <bool> STATUS_DB;
	sc_in<bool> WE_DB_IN;
	sc_out<bool> WE_DB_OUT;
	
	void setinode(int id) { 
		inode = id;
	}
	SC_HAS_PROCESS(PRIV_L1);
	PRIV_L1(sc_module_name name, int id): sc_module(name), inode(id),ACK_DB(0), rd_wr_to_l2(0),ce_to_l2(0),address_to_l2(0x0),req_count_to_l2(0x0), REQ_COUNT_OUT(0x0), ADDRESS_DB_OUT(0x0), WE_DB_OUT(0x0)
	{		
		PRIV_L1_init();
		SC_METHOD(request_collector); 
		sensitive << clk.pos() ;
		SC_THREAD(priv_l1_main_thread); 
		sensitive << clk.pos() ;
		l1_miss_count = 0x0;
		l1_hit_count = 0x0;		
	}
	
	~PRIV_L1(){
		int i=0;
		while(i<PRIV_L1_WAYS){
			free(PRIV_L1Mem[i]);
			i++;
		}
	}
	long l1_miss_count;
	long l1_hit_count;
private:	
	void request_collector(void); // thread needs to put to a internal fifo ReqQ
	/*
	Main jobs: 
	1) do WR/RD/EX request from bus and its respective actions
	2) do Inv from noc 
	
	take req from ReqQ, process req thru different states IDLE/RD/RDX/WR/INV	
	*/
	void priv_l1_main_thread(void); // thread which takes requests and process them one by one 
	void PRIV_L1_init(void);
	bool table_lookup(int address, char * hit_way, int * state);
	bool read_write_to_l2 (int address, bool rd_wr, datatype* data,int req_count, char way);
	bool invalidating_cl (int address ,int way,int req_count, unsigned int state,ReqFormat_PrivL1::ReQType tp);
	char find_free_way (int address,int req_count, bool *status);
	bool find_in_blockedQ(unsigned int address);
	queue <ReqFormat_PrivL1> ReqQ; //sc_fifo <ReqFormat_PrivL1> ReqQ;
	queue <ReqFormat_PrivL1> BlockedQ; 
	int inode;
	int PRIV_L1tag[3][PRIV_L1_SET_COUNT][PRIV_L1_WAYS];
	datatype * PRIV_L1Mem[PRIV_L1_WAYS];		
	//datatype ** PRIV_L1Mem;
};

#endif
