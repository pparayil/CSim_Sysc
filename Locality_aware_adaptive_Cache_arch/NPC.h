#ifndef NPC_H
#define NPC_H

#include <queue>
#include <iostream>
#include <systemc.h>
#include "write_read_if.h"
#include "datatype_ch.h"
#include "LSU_defines.h"

#define NPC_TAGSIZE 
#define NPC_SET_COUNT 1024
#define NPC_SET_COUNT_2 10
#define QMAX 5
#define NPC_WAYS 0x2

//TODO: note all data widths needs to be changed from int to 8 word length
using namespace std;

class ReqFormat {
public : 
enum ReQType {RD, WR, INV, FLUSH, RDX,  ACK_SMALL, ACK_FULL, WR_BACK, ACKL1_WB};
	datatype data;
	unsigned int address;
	bool we;
	bool ex;
	bool linked;
	int way;
	//int req_number;
	int req_counter;
	int ex_ack_count;
	int sc_ack_count;
	bool mode_pri_remote;
	unsigned int ack_type;
	ReQType type;
	ReqFormat(void) {
	ex_ack_count = 0x1;
	sc_ack_count = 0x1;
	req_counter = 0x0;
	mode_pri_remote = false; //pri
	linked = false;
	we = false;
	}
	 bool operator!=(const ReqFormat& other){
		if((this->address == other.address) && (this->req_counter == other.req_counter) && (this->we == other.we))
			return false;
		else 
			return true;
	 }
};

SC_MODULE(NPC)
{
public:
	
// to the lsu side 
	sc_out <bool> generate_wr_req_npc;
	sc_in <bool> inv_frm_dir;
	sc_in <bool> flush_frm_npc_dir;
 	sc_in <int> inv_addr_from_dir;
	sc_out <bool> cl_evt_info_to_dir;
	sc_out <bool> inv_ack_to_dir;
	sc_out <int> address_to_dir;
	sc_in <int> address_from_dir;
	sc_out <int> req_count_to_dir;
	sc_in <int> req_count_from_dir;
	//sc_out <int> min_time_stamp_to_dir;
	sc_out <int> utiliz_to_dir;
	sc_in <bool> mode_pri_remote_from_dir; 
	sc_out <bool> ex_to_dir;
	sc_out <bool> ce_to_dir;
	sc_out <bool> rd_wr_to_dir;
	sc_in <bool> ack_from_dir;
	sc_in <bool> ack_from_llc;
	sc_out <int> address_to_llc;
	sc_in <int> address_from_llc;
	sc_out <int> req_count_to_llc;
	sc_in <int> req_count_from_llc;
	sc_in <bool> mode_pri_remote_from_llc;
	sc_out <bool> address_to_llc_en;
	sc_port<write_read_if> data_to_llc;
	sc_port<write_read_if> data_from_llc;
// L1 side
	sc_in <bool> ce_from_l1;
	sc_in <bool> rd_wr_from_l1;
	sc_in <int> address_from_l1;
	sc_in <int> req_count_from_l1;
	sc_in <bool> ack_from_l1;	
	sc_port<write_read_if> data_from_l1;	
	
	sc_out <int> req_count_to_l1;
	sc_out <int> address_to_l1;
	sc_port<write_read_if> data_to_l1;
	sc_out <bool> ack_to_l1;
	sc_out <bool> ack_we_to_l1;
	sc_out <bool> inv_to_l1;
	sc_out <bool> mode_pri_remote_to_l1;

	sc_in<bool> clk;
	SC_HAS_PROCESS(NPC);
	NPC(sc_module_name name, int id): sc_module(name), inode(id), cl_evt_info_to_dir(0), inv_ack_to_dir(0),ex_to_dir(0),rd_wr_to_dir(0),ce_to_dir(0),address_to_llc_en(0),address_to_dir(0x0),req_count_to_dir(0x0),address_to_llc(0x0),req_count_to_llc(0x0)
	{
		
		NPC_init();
		SC_THREAD(request_collector); 
		sensitive << clk.pos() ;
		SC_THREAD(npc_main_thread); 
		sensitive << clk.pos() ;
		l2_miss_count = 0x0;
		l2_hit_count = 0x0;		
	}
	void setinode(int id) { inode = id;}
	~NPC(){
	int i=0;
	while(i<NPC_WAYS){
	free(NPCMem[i]);
	i++;
	}
	}
	long l2_miss_count;
	long l2_hit_count;
	enum MSI_NPC_STATES {INVALID,SHARED,MODIFIED, S_M, I_S, S_I, I_M, M_I_WB};
private:	
	void request_collector(void); // thread needs to put to a internal fifo ReqQ
	/*
	Main jobs: 
	1) do WR/RD/EX request from bus and its respective actions
	2) do Inv from noc 
	
	take req from ReqQ, process req thru different states IDLE/RD/RDX/WR/INV	
	*/
	void npc_main_thread(void); // thread which takes requests and process them one by one 
	void NPC_init(void);
	bool table_lookup(unsigned int address, char * hit_way, int * state);
	bool rd_rdx_to_dir (unsigned int address, bool Exclusive, bool refill,int req_count);
	void write_to_llc (unsigned int address, datatype * DATA, bool evict, int req_count, int way, bool req_to_directory, bool req_to_llc, unsigned int utilz);
	bool invalidating_cl (unsigned int address ,int way,int req_count, unsigned int type,ReqFormat::ReQType tp);
	char find_free_way (unsigned int address,int req_count,bool *wb_enable, bool *status);
	bool find_in_blockedQ(unsigned int address);
	void LRU_uliz_update(bool down_up, unsigned int set,unsigned int way, bool util);
	void ACK_L1(unsigned int address,unsigned int req_count,bool status, bool we,datatype * data, bool rm);
	bool inv_ack_dir (unsigned int address, int req_count, int way);
	void inv_l1(unsigned int address,unsigned int req_count);
	queue <ReqFormat> ReqQ; //sc_fifo <ReqFormat> ReqQ;
	queue <ReqFormat> BlockedQ; 
	int inode;
	unsigned int NPCtag[4][NPC_SET_COUNT][NPC_WAYS];
	datatype * NPCMem[NPC_WAYS];		
	//datatype ** NPCMem;
};

#endif
