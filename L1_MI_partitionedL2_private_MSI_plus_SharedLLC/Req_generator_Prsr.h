#ifndef PR_REQ_GEN_H
#define PR_REQ_GEN_H

#include <fstream>
#include <iostream>
#include<systemc.h>
//#include "NoximGlobalTrafficTable.h"
#include <vector>
#include <queue>
#include "write_read_if.h"

using namespace std; 

class ReqFormat_processor {
public : 
	int address;
	bool rw;
	datatype data;
	sc_time start;
	int req_count;
	ReqFormat_processor(void) {
		start = sc_time_stamp();
		address = 0;
		req_count = 0;
		rw= 0;
		for(int i=0;i<8;i++)
			data.data[i] = 0;
	}
	 bool operator!=(const ReqFormat_processor& other){
		if((this->address == other.address) && (this->req_count == other.req_count) && (this->start == other.start) && (this->rw == other.rw))
			return false;
		else 
			return true;
	 }
};

SC_MODULE(PR_REQ_GEN)//: public L1D
{
public:
	sc_in <bool> clk;
	sc_port<write_read_if> DATA_DB_IN;
	sc_port<write_read_if> DATA_DB_OUT;
	sc_in<int> ADDRESS_DB_IN;
	sc_out<int> ADDRESS_DB_OUT;
	sc_in<int> REQ_COUNT_IN;
	sc_out<int> REQ_COUNT_OUT;
	sc_in <bool> STATUS_DB;
	//sc_inout<unsigned int> PROC_ADDRESS_DB;
	sc_out <bool> CE_DB;
	sc_in<bool> WE_DB_IN;
	sc_out<bool> WE_DB_OUT;
	//sc_out<bool> EX_DB;
	sc_in <bool> ACK_DB;
	
	SC_HAS_PROCESS(PR_REQ_GEN);
	
	long long int time_taken;

	PR_REQ_GEN(sc_module_name name, char p_id):sc_module(name),pnode(p_id),ADDRESS_DB_OUT(0x0),WE_DB_OUT(0),CE_DB(0)
	{
		SC_THREAD(self);
		sensitive << clk.pos();	
		
		SC_METHOD(ack_reception);
		sensitive << ACK_DB;
		
		time_taken = 0;
		resend = false;
	}
	void setnode (char pid, const char* fname) {		 
		inode = pid; 
/*		if(pid == 0) fname = "fmm_1core_3.txt";// "barnes_1core_3.txt";
                if(pid == 1) fname =  "ocean_1core_3.txt"; //"lu_1core_3.txt";
                if(pid == 2) fname =  "cholesky_1core_3.txt"; //"water_1core_3.txt";*/
		//"lu_15core_3.txt";
		/*
		if(inode <= 1) {
                                fname = "cholesky_2core_3.txt";
                                pnode = inode;
                                }
                if(inode > 1 && inode <= 3) {
                                fname = "lu_2core_3.txt";
                                pnode = inode - 2;
                                }
                if(inode >3 && inode <= 5) {
                                fname = "ocean_2core_3.txt";
                                pnode = inode - 4;
                                }
                if(inode > 5 && inode <= 8) {
                                fname = "fmm_3core_3.txt";
                                pnode = inode - 6;
                                }
                if(inode >8 && inode <= 11) {
                                fname = "water_3core_3.txt";
                                pnode = inode - 9;
                                }
                if(inode >= 12) {
                                fname = "barnes_3core_3.txt";
                                pnode = inode - 12;
                                }	*/	
		if(fname == NULL) cout<<"error!!!!"<<endl;
		traffic_table.open(fname, ios::in);
    		if (!traffic_table)
			cout<<"file open failed in  processor " <<(unsigned int) inode<< " !!!"<<endl;
	}	
private:
	//NoximGlobalTrafficTable *traffic_table;	// Reference to the Global traffic Table
	void self (void);
	void ack_reception (void);
	ifstream traffic_table;
	char inode;
	char pnode;	
	queue <ReqFormat_processor> blocked_req_Q;
	ReqFormat_processor resendreq_B;
	bool resend;
};
#endif //PR_REQ_GEN_H
