#ifndef DIR_H
#define DIR_H

#include <queue>
#include <iostream>
#include <systemc.h>
#include "LSU_defines.h"

#define TOTAL_CL_IN_MEM MEM_SIZE_VALUE
#define TOTAL_CL_IN_DIR (TOTAL_CL_IN_MEM/TOTAL_NO_OF_NODES_CARRYING_DIR)
#define QMAX_DIR 5
#define CL_STATUS_STORAGE_BASE 4
#define RM_UTIL_BASE 1
#define RM_UTIL_WIDTH 4 //(RATmax == 16)
#define RATMAX	0x0f
#define RAT_LEVEL_BASE (RM_UTIL_BASE+RM_UTIL_WIDTH)

using namespace std;

enum ReQType_Dir {INV_D, RDX_D, RD_D, WR_D, ACK_NPC, ACK_SC, EVT_D};

class ReqFormat_dir {
public : 
	int address;
	int req_count;
	bool we;
	bool evict;
	int DstNode;
	int SrcNode;
	int inv_ack_count;
	int sc_ack_count;
	ReQType_Dir type;
	ReqFormat_dir(void) {
		inv_ack_count = 0x0;
		sc_ack_count = 0x0;
	}
	bool operator!=(const ReqFormat_dir& other){
	if((this->address == other.address) && (this->req_count == other.req_count) && (this->type == other.type) /*&& (this->inv_ack_count == other.inv_ack_count)*/ && (this->we == other.we) && (this->evict == other.evict) && (this->DstNode == other.DstNode) && (this->SrcNode == other.SrcNode) )
		return false;
	else 
		return true;
	 }
};

SC_MODULE(DIR)
{
public:
	sc_in <bool> clk;
	sc_out <int> nodeaddr_to_lru;
	sc_in <int> node_address_from_lru;
// from othr nodes

	sc_out <bool> inv_to_node;
	sc_out <bool> flush_frm_npc;
	sc_out <int> inv_addr_to_node;
	sc_in <bool> cl_evt_info_to_dir;
	sc_in <bool> inv_ack_to_dir;
	sc_in <int> priv_utiliz_to_dir;
	sc_in <int> address_to_dir;
	sc_out <int> address_from_dir;
	sc_in <int> req_count_to_dir;
	sc_out <int> req_count_from_dir;
	sc_in <bool> ex_to_dir;
	sc_in <bool> rd_wr_to_dir;
	sc_out <bool> ack_to_node;
	sc_in <bool> ce_to_dir;
	sc_out <bool> mode_pri_remote_from_dir;

// LLC interaction 
	sc_out <bool> rd_wr_to_llc;
	sc_out <int> addr_to_llc;
	sc_out <int> req_count_to_llc;
	sc_in <int> addr_from_llc;
	sc_in <int> req_count_from_llc;
	sc_out <int> dst_nodeaddr_to_llc;
	sc_out <bool> mode_pri_remote_to_llc;
	sc_out <bool> ce_to_llc;
	sc_in <bool> ack_from_llc;
	//sc_in <int> addr_from_llc;
	SC_HAS_PROCESS(DIR);
	DIR(sc_module_name name, int id):sc_module(name),inode(id), inv_to_node(0),inv_addr_to_node(0),ack_to_node(0),addr_to_llc(0x0),rd_wr_to_llc(0),ce_to_llc(0),nodeaddr_to_lru(0x0),dst_nodeaddr_to_llc(0x0){
		dir_init();
		SC_THREAD(request_collector); 
		sensitive << clk.pos() ;
		SC_THREAD(dir_main_thread); 
		sensitive << clk.pos() ;

	}
	void setinode(int id) { inode = id;}
	~DIR(){
	free(DirMem);
	for(char j=0; j<NO_OF_NODES;j++){
		free(DirACCmem[j]);
	}
	}
	enum DIR_STATES {INVALID,SHARED,MODIFIED, S_M, I_S, S_I, I_M, M_I_WB, M_M, M_S};
private:
	void request_collector(void); // thread needs to put to a internal fifo ReqQ
	/*
	Main jobs: 
	1) do WR/RD/EX request from bus and its respective actions
	2) do Inv from noc 
	
	take req from ReqQ, process req thru different states IDLE/RD/RDX/WR/INV	
	*/
	void dir_main_thread(void); // thread which takes requests and process them one by one 
	void dir_init(void);
	bool refill_cl_to_npc ( int address, int nodeaddress, int req_count) ; // refill cl -> read from llc and write in npc
	bool write_to_llc ( int address, int nodeaddress,  bool evict, int req_count, unsigned int priv_util) ;
	bool Invalidation_fn(int address, int nodeaddress, int req_count, bool we, bool evict, unsigned int type);
	void WR_RD_CL (int address, int nodeaddress, int req_count, bool we);
	void ack_node (int address, int nodeaddress, int req_count, bool mode);
	void core_rm_mode_util_update(int address, int nodeaddress);
	void core_priv_rm_util_mode_update(int address, int nodeaddress, unsigned int priv_util, bool inv);
	void set_ratlevel(int address, int nodeaddress, bool rat);
	unsigned int get_rat(int address, int nodeaddress);
	void set_rm_mode(int address, int nodeaddress, bool mode);
	bool get_rm_mode(int address, int nodeaddress);
	bool remove_all_other_reqs_for_address(unsigned int nodeid, unsigned int req_count,unsigned int address);

	queue <ReqFormat_dir> ReqQ, BlockedQ;
	int inode;
	unsigned int *DirMem; //[TOTAL_CL_IN_DIR]; //[cl_status|state]
	unsigned int  *DirACCmem[NO_OF_NODES];
};

#endif //DIR_H
