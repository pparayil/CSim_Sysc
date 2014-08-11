#ifndef DIR_H
#define DIR_H

#include <queue>
#include <iostream>
#include <systemc.h>
#include "LSU_defines.h"

#define TOTAL_CL_IN_MEM MEM_SIZE_VALUE
#define TOTAL_CL_IN_DIR (TOTAL_CL_IN_MEM/TOTAL_NO_OF_NODES_CARRYING_DIR)
#define QMAX_DIR 5
#define CL_STATUS_STORAGE_BASE (6+2*NO_OF_NODES_2)
#define FORWARDER_EN_BASE (4+NO_OF_NODES_2)
#define FORWARDER_BASE (4)
#define DCC_NODE_BASE (5+NO_OF_NODES_2)
#define DCC_NODE_EN_BASE (5+2*NO_OF_NODES_2)


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
	sc_out <bool> spill_inv_to_node;
	sc_out <bool> flush_frm_npc;
	sc_out <bool> forward_req_from_dir;	
	sc_out <bool> dcc_transfer_en_to_npc;
	sc_out <bool> replace_req_dcc_ack_from_dir;
	sc_out <bool> forward_spillreq_to_npc;
	sc_out <int> inv_addr_to_node;
	sc_in <bool> cl_evt_info_to_dir;
	sc_in <bool> inv_ack_to_dir;
	sc_in <int> address_to_dir;
	sc_out <int> address_from_dir;
	sc_in <int> req_count_to_dir;
	sc_out <int> req_count_from_dir;
	sc_in <bool> ex_to_dir;
	sc_in <bool> rd_wr_to_dir;
	sc_out <bool> ack_to_node;
	sc_in <bool> ce_to_dir;

// L3 interaction 
	sc_out <bool> rd_wr_to_l3;
	sc_out <int> addr_to_l3;
	sc_out <int> req_count_to_l3;
	sc_in <int> addr_from_l3;
	sc_in <int> req_count_from_l3;
	sc_out <int> dst_nodeaddr_to_l3;
	sc_out <bool> ce_to_l3;
	sc_in <bool> ack_from_l3;
	//sc_in <int> addr_from_l3;
	SC_HAS_PROCESS(DIR);
	DIR(sc_module_name name, int id):sc_module(name),inode(id), inv_to_node(0),inv_addr_to_node(0),ack_to_node(0),addr_to_l3(0x0),rd_wr_to_l3(0),ce_to_l3(0),nodeaddr_to_lru(0x0),dst_nodeaddr_to_l3(0x0){
		dir_init();
		SC_THREAD(request_collector); 
		sensitive << clk.pos() ;
		SC_THREAD(dir_main_thread); 
		sensitive << clk.pos() ;

	}
	void setinode(int id) { inode = id;}
	~DIR(){
	free(DirMem);
	}
	enum STATE_Dir {INVALID, SHARED, MODIFIED, S_to_S, I_to_M, M_to_S, S_to_I, S_to_M, M_to_M, I_to_S, I_to_S_DCC /*DCC read*/, I_to_M_DCC /*DCC read n inv*/, M_to_M_DCC /* inv*/, S_I_DCC, M_I_DCC, M_I_WB};
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
	bool refill_cl_to_npc ( int address, int nodeaddress, int req_count) ; // refill cl -> read from l3 and write in npc
	bool write_to_l3 ( int address,int nodeaddress, bool evict, int req_count) ;
	bool Invalidation_fn(int address, int nodeaddress, int req_count, bool we, bool evict, unsigned int type);
	void WR_RD_CL_FRM_MEML3 (int address, int nodeaddress, int req_count, bool we);
	void WR_RD_CL_FRM_NPC (int address, int nodeaddress, int req_count, int forwarder, unsigned int type) ;
	bool remove_all_other_reqs_for_address(unsigned int nodeid, unsigned int req_count,unsigned int address);
	void ACKcmd_to_caches (unsigned int address,unsigned int req_count, unsigned int nodeaddress);
	void Invcmd_to_caches (unsigned int address,unsigned int req_count, unsigned int nodeaddress, bool spill_inv);
	void DCC_READ_n_INVALIDATE(unsigned int address,unsigned int req_count, unsigned int nodeaddress);
	void DCC_INVALIDATE(unsigned int address,unsigned int req_count);
	unsigned int count_one(unsigned int x);
	queue <ReqFormat_dir> ReqQ, BlockedQ;
	int inode;
	unsigned int *DirMem; //[TOTAL_CL_IN_DIR]; //[cl_status|FWD_VALID|FORWARDER_NODE|state]

};

#endif //DIR_H
