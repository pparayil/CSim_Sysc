#ifndef LSU_DEFINES_H
#define LSU_DEFINES_H
#include "datatype.h"

#define NODE_WITH_SDRAM 15 //3
#define CPU_COUNT_IN_CLUSTER 4
#define CL_SIZE 5 // 8 word 
#define CL_SIZE_WORDWISE 8
#define OFFSET_LENGTH 5
#define NO_OF_NODES 16 //4 
#define NO_OF_NODES_2 4 //2 
#define TOTAL_NO_OF_NODES_CARRYING_DIR_2 4 //2 // in terms of power of 2 
#define TOTAL_NO_OF_NODES_CARRYING_DIR 16 //4 // in terms of power of 2 
//#undef INV_4PHASE
#define INV_4PHASE 1

#undef L3_TRUE
//#define L3_TRUE_1
#undef L3_TRUE_1
#undef L3_TRUE_0
//#define L3_TRUE_0
#define D_O_C 0 

#define MEM_SIZE_VALUE 262500 //16384 // 2097152//1048576 //134217727 //1024 //4294967295
#define MAX_PROCESSOR_COUNT_WIDTH 4
#define MEM_DELAY 300
				/*0 		1		2	    		3       4           5          6          7        8       9       10         11     	12				13				14*/
enum LSU_STATE {IDLE, L2CEEXEVT, L2DATA_TO_L3MEM, DIRINV, DIRACK, DIRCE_TO_L3MEM,MEMDIRACK, MEML2ACK,L3ACKL2,L3ACKDIR,L3REQMEM,MEML3ACK,L2DATA_WB_TO_L3MEM,DIRFWD_TO_NPC,NPC_NPC_DATA_FWD};
class NoCCommPacket {
public: 

	bool inv;
	bool ack;
	bool rw;
	bool en;
	bool ex;
	bool evt;
	datatype data;
	unsigned int address;
	int req_count;
	unsigned int srcnode;
	unsigned int redirectionnode;
	unsigned int dstnode;
	LSU_STATE type;

	bool operator!=(const NoCCommPacket& other){
		if((this->address == other.address) && (this->req_count == other.req_count) && (this->type == other.type) && (this->srcnode == other.srcnode) && (this->dstnode == other.dstnode) && (this->en == other.en) && (this->ack == other.ack) )
			return false;
		else 
			return true;
	}
	NoCCommPacket(void){
		address = 0x0;
		req_count = 0x0;
		type = IDLE;
		srcnode = 0x0;
		dstnode = 0x0;
		en = false;
		ack = false;
	}
};


#endif //LSU_DEFINES_H
