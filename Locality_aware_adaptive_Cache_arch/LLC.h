#ifndef LLC_H
#define LLC_H

#include <queue>
#include <iostream>
#include <systemc.h>
#include "LSU_defines.h"
#include "write_read_if.h" 
#include "datatype_ch.h"

#define NO_CLS_IN_LLC
#define LLC_WAYS 2 
#define LLC_SET_COUNT 1024 //256 
#define LLC_SET_COUNT_2 10 // //8
#define TOTAL_NO_OF_NODES_CARRYING_LLC_2 TOTAL_NO_OF_NODES_CARRYING_DIR_2 //2 // in terms of power of 2 
#define TOTAL_NO_OF_NODES_CARRYING_LLC TOTAL_NO_OF_NODES_CARRYING_DIR //4 // in terms of power of 2 

using namespace std;

class ReqFormat_LLC {
public : 
enum ReQType_LLC {INV, RD, WR, ACK, WR_BACK};
	/*				0x0   	0x1    0x2    0x3       0x4		 */
enum STATEType_LLC {REG_I, REG_S, REG_M, TRANS_MI, TRANS_IS, TRANS_IM};
	unsigned int address;
	unsigned int req_count;
	bool we;
	bool dirack_en;
	datatype data;
	unsigned int way;
	ReQType_LLC type;
	unsigned int npc_node;
	unsigned int dir_node;
	bool linked;
	bool mode_pri_remote;
	ReqFormat_LLC(void) {
		linked =  false;
		address = 0x0;
		req_count = 0x0;
		we = false;
		type = INV;
		npc_node = 0x0;
		dir_node = 0x0;
		dirack_en = false;
	}
	bool operator!=(const ReqFormat_LLC& other){
		if((this->address == other.address) && (this->req_count == other.req_count) && (this->type == other.type) && (this->npc_node == other.npc_node) && (this->dir_node == other.dir_node) )
			return false;
		else 
			return true;
	 }
};

class ReqData_LLC {
public : 
	datatype data;
	int address;
	int req_count;
	int npc_node;
	ReqData_LLC(void) {
	}
	bool operator!=(const ReqData_LLC& other){
		if((this->address == other.address) && (this->req_count == other.req_count) && (this->npc_node == other.npc_node))
			return false;
		else 
			return true;
	}
};

SC_MODULE(LLC)
{
public:
	sc_in <bool> clk;
	sc_out <int> nodeaddr_to_lru;
	sc_in <int> node_address_from_lru;

// dir interaction 
	sc_in <bool> rd_wr_;
	sc_in <int> addr_f_dir;
	sc_out <int> addr_t_dir;
	sc_in <int> req_count_f_dir;
	sc_out <int> req_count_t_dir;
	sc_in <int> dst_nodeaddr_; // _ce cud be enabled, addr_en is part of this
	sc_in <bool> ce_;
	sc_out <bool> ack_to_dir;
	sc_in <bool> without_dir_ack;
	sc_in <bool> mode_pri_remote_f_dir;
// npc
	sc_in <int> addr_f_npc;
	sc_out <int> addr_t_npc;
	sc_in <int> req_count_f_npc;
	sc_out <int> req_count_t_npc;
	sc_in <bool> addr_en_frm_npc; 
	sc_port<write_read_if> data_in;
	sc_port<write_read_if> data_out;
	sc_out <bool> ack_to_npc;
	sc_out <bool> ack_rw_to_npc;
	sc_out <bool> mode_pri_remote_to_npc;

// mem
	sc_out <int> addr_to_mem;
	sc_in <int> addr_from_mem;
	sc_in <int> req_count_from_mem;
	sc_out <int> req_count_to_mem;
	sc_out <bool> addr_en_to_mem;
	sc_out <bool> r_w_to_mem;
	sc_in <bool> ack_from_mem; 
	sc_in <bool> ack_rw_from_mem;
	sc_port<write_read_if> data_mem;
	

	SC_HAS_PROCESS(LLC);
	LLC(sc_module_name name, int id, bool instantiate):sc_module(name),inode(id), ack_to_dir(0x0),ack_to_npc(0),nodeaddr_to_lru(0x0),addr_en_to_mem(0),r_w_to_mem(0),addr_to_mem(0x0){
		LLC_init(instantiate);
		ack = false;
		SC_THREAD(LLC_request_collector); 
		sensitive << clk.pos() ;
		SC_THREAD(LLC_main_thread); 
		sensitive << clk.pos() ;
		llc_miss_count = 0x0;
		llc_hit_count = 0x0;
	}
	void setinode(int id) { inode = id;}
	~LLC(){
	int i=0, j=0;
	while(j <= (D_O_C+1)){
		while(i<LLC_WAYS){
		if(LLCMem[j][i] != NULL) 
			free(LLCMem[j][i]);
		i++;
		}
		j++;
	}
	}
	long llc_miss_count;
	long llc_hit_count;
private:
	void LLC_request_collector(void); // thread needs to put to a internal fifo ReqQ
	/*
	Main jobs: 
	1) do WR/RD/EX request from bus and its respective actions
	2) do Inv from noc 
	
	take req from ReqQ, process req thru different states IDLE/RD/RDX/WR/INV	
	*/
	void LLC_main_thread(void); // thread which takes requests and process them one by one 
	void LLC_init(bool instantiate);
	bool llc_lookup(unsigned int address, char *way, bool *self);
	void ack_to_npc_dir(unsigned int address, datatype *data, int req_count, bool we, unsigned int npc_node, unsigned int dir_node, bool dirack_en, bool mode_pri_remote);
	void rdwr_to_mem ( unsigned int address, datatype *data, int req_count, bool we, unsigned int npc_node, unsigned int dir_node, unsigned int way, bool dirack_en, bool mode_pri_remote);
	bool find_way(unsigned int address, char *way, int req_count, bool *WB_enable);
	unsigned int find_coop_sect(unsigned int address);
	bool find_in_blockedQ(unsigned int address);
	queue <ReqFormat_LLC> ReqQ;
	queue <ReqData_LLC> ReqQ_data;
	queue <ReqFormat_LLC> BlockedQ; 
	int inode;
	unsigned int LLCtag[(D_O_C+1)][3][LLC_WAYS][LLC_SET_COUNT];
	datatype * LLCMem[(D_O_C+1)][LLC_WAYS]; //0 for self and 1 for cooperative
	bool ack;
	unsigned int cooper_section_cnt;
};

#endif //LLC_H
