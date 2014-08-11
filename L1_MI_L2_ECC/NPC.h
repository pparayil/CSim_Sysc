#ifndef NPC_H
#define NPC_H

#include <queue>
#include <iostream>
#include <systemc.h>
#include <stdlib.h> 
#include "write_read_if.h"
#include "datatype_ch.h"
#include "LSU_defines.h"

#define NPC_TAGSIZE 
#define NPC_SET_COUNT 1024
#define NPC_SET_COUNT_2 10
#define QMAX 5
#define NPC_WAYS 0x4
#define TTL_NO_NPC 15

//TODO: note all data widths needs to be changed from int to 8 word length
using namespace std;

class ReqFormat {
public : 
enum ReQType {RD, WR, INV, FLUSH, RDX, FWD_RD, SPILL_RD, ACK_SMALL, ACK_FULL, FWD_RD_ACK, WR_BACK, SPILL_DATA_ECC_ACK, REPL_REQ_ECC_ACK, SPILL_DATA_ECC, ECC_REPLACEMENT};
	datatype data;
	int address;
	bool we;
	bool fwd;
	bool ex;
	bool linked;
	int way;
	//int req_number;
	int req_counter;
	int redirect_nodeid;
	int ex_ack_count;
	int sc_ack_count;
	unsigned int ack_type;
	ReQType type;
	ReqFormat(void) {
	ex_ack_count = 0x1;
	sc_ack_count = 0x1;
	req_counter = 0x0;
	redirect_nodeid = 0x0;
	linked = false;
	ex = false;
	fwd = false;
	}
	 bool operator!=(const ReqFormat& other){
		if((this->address == other.address) && (this->req_counter == other.req_counter) && (this->type == other.type) && (this->we == other.we) && (this->linked == other.linked) && (this->way == other.way) )
			return false;
		else 
			return true;
	 }
};

SC_MODULE(NPC)
{
public:
	sc_in <bool> clk;
// to the lsu side 
	sc_out <bool> generate_wr_req_npc;
	sc_in <bool> inv_frm_dir;
	sc_in <bool> spill_inv_frm_dir;
	sc_in <bool> flush_frm_npc_dir;
	sc_in <bool> forward_req_from_dir;
	sc_out <bool> forward_data_to_npc;
	sc_in <bool> forward_spillreq_from_dir;
	sc_in <bool> dcc_transfer_en_from_dir;
	sc_out <bool> forward_spill_data_to_npc;
	sc_in <bool> forward_spilldata_ack_from_npc;
	sc_in <bool> spill_data_dirty_from_npc;
	sc_out <bool> spill_data_dirty_to_npc;
	sc_in <bool> replace_data_dcc_ack_from_npc;
	sc_out <bool> replace_data_dcc_ack_to_npc;
	sc_out <bool> replace_data_dcc_to_npc;
	sc_in <bool> replace_data_dcc_from_npc;
	sc_in <bool> replace_req_dcc_ack_from_dir;
	//sc_in <bool> switch_to_shared_from_dir;
	sc_in <int> nodeid_from_lsu; //forward destination nodeid
	sc_out <int> nodeid_to_lsu;
	sc_in <bool> ack_from_npc;
 	sc_in <int> inv_addr_from_dir;
	sc_out <bool> cl_evt_info_to_dir;
	sc_out <bool> inv_ack_to_dir;
	sc_out <int> address_to_dir;
	sc_in <int> address_from_dir;
	sc_out <int> req_count_to_dir;
	sc_in <int> req_count_from_dir;
	sc_out <bool> ex_to_dir;
	sc_out <bool> ce_to_dir;
	sc_out <bool> rd_wr_to_dir;
	sc_in <bool> ack_from_dir;
	sc_in <bool> ack_from_l3;
	sc_out <int> address_to_l3;
	sc_in <int> address_from_l3;
	sc_out <int> req_count_to_l3;
	sc_in <int> req_count_from_l3;
	sc_out <bool> address_to_l3_en;
	sc_port<write_read_if> data_to_l3;
	sc_port<write_read_if> data_from_l3;
	
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


	SC_HAS_PROCESS(NPC);
	NPC(sc_module_name name, int id): sc_module(name), inode(id),cl_evt_info_to_dir(0), inv_ack_to_dir(0),ex_to_dir(0),rd_wr_to_dir(0),ce_to_dir(0),address_to_l3_en(0),address_to_dir(0x0),req_count_to_dir(0x0),address_to_l3(0x0),req_count_to_l3(0x0)
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
	/* 4 bit state 
		/I 0000
		/S 0001 
		/M 0010 
		/F 0011
		/F-singlet 0100
		0101: S spilled
		0110: M spilled, 
		 
		// transition states: 
		1000: F to I,
		1001: M to I ECC replacement	
		1010: F to I singlet ECC replacement,		
		1011: Shared to M ,
		1100 : INV - SHARED (RD) ,
		1101 : Shared -INV(INV simple), 
		1110: inv to M (exRD+WR) , 
		1111: M to INV (INV with WB),	
		 	
		*/
	enum NPC_STATES {INVALID,SHARED,MODIFIED,FORWARD,FORWARD_SNG,S_SP,M_SP,F_M, F_I, SNG_ECC_TRANS,F_I_SNG, S_M, I_S, S_I, I_M, M_I_WB};
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
	bool table_lookup(int address, char * hit_way, int * state, bool inv_req_provoked);
	bool rd_rdx_to_dir (int address, bool Exclusive, bool refill,datatype* data,int req_count);
	void write_to_l3 ( int address, datatype * DATA, bool evict, int req_count, int way, bool EN_req_to_directory, unsigned int ack_receiver, bool ecc, bool dirty);
	bool invalidating_cl (int address ,int way,int req_count, unsigned int type,ReqFormat::ReQType tp, unsigned int redirect_node);
	char find_free_way (int address,int req_count,int *prev_state, bool *status, bool ecc_reception);
	bool find_in_blockedQ(unsigned int address);
	void INV_L1(unsigned int address,unsigned int req_count);
	void ack_to_L1(bool we,unsigned int address,unsigned int req_count,datatype* data);
	void req_to_DIR (bool evict,bool rw,bool ex,bool ce, unsigned int address,unsigned int req_count, bool ecc_en, bool inv_ack);
	void fwd_rd_to_npc(unsigned int nodeid, unsigned int address,unsigned int req_count,datatype* data);
	void LRU_update(bool down_up, unsigned int set,unsigned int way );
	queue <ReqFormat> ReqQ; //sc_fifo <ReqFormat> ReqQ;
	queue <ReqFormat> BlockedQ; 
	int inode;
	int NPCtag[3][NPC_SET_COUNT][NPC_WAYS];
	void find_way(int address, char * hit_way);
	datatype * NPCMem[NPC_WAYS];		
	//datatype ** NPCMem;
};

#endif
