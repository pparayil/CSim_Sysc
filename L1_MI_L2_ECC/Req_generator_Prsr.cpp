
#include <fstream>
#include <iostream>
#include "Req_generator_Prsr.h"
#include "LSU_defines.h"
using namespace std;
void PR_REQ_GEN::self (void){

int src, src_node;	// Mandatory
unsigned int addrs;
long unsigned int addrs_long;
float pir, por;
int t_on, t_off, t_period;
int _read_or_write;
int _burst;
int _offset;
int now;
int params = 0;
unsigned int wait_count = 0x0;

cout<<__func__<<" : "<<__LINE__<<endl;
wait();
while (!traffic_table.eof()) 
{
	char line[512];	
	if(resend == true) {		
		{
			if (resendreq_B.rw == true)
			{
				DATA_DB_OUT->write_data(&resendreq_B.data);
				wait();
			}
			ADDRESS_DB_OUT = resendreq_B.address;
			REQ_COUNT_OUT = resendreq_B.req_count+1;
			resendreq_B.req_count = resendreq_B.req_count+1;
			WE_DB_OUT = resendreq_B.rw;
			CE_DB = true;
			wait();
			CE_DB = false;
			blocked_req_Q.push(resendreq_B);
			resend = false;
			wait();
			cout<<"@Node "<<(unsigned int) inode<<" Resent request for address:"<<hex<<resendreq_B.address<<dec<<endl;
		} /*else {
		cout << "error!! resend is true"<<endl;
		resend = false;
		}*/
	}	
	if((((unsigned int)blocked_req_Q.size()) > 0x1)) {
		wait();
		wait_count ++;
		continue;
	} 	
	traffic_table.getline(line, sizeof(line) - 1);
	if (line[0] != '\0') 
	{
   	 	if (line[0] != '%') 
		{
			now = (sc_time_stamp().to_double() / 1000);
			params = sscanf(line, "%d %d %d %lx", 
           			&t_on,&src_node, &_read_or_write, &addrs_long); 
			t_on = (int) t_on/100;
			if (params != 0) 
			{		
                addrs = addrs_long & 0x07fffff; // approximate 				
				if(src_node == inode) //0) //inode)
				{
				while ((t_on>wait_count)) {
					if(t_on != 0x0) 
						t_on--;
					wait();
				}
				wait_count = 0x0;
				if ((addrs>>CL_SIZE) >= (MEM_SIZE_VALUE-1)) {addrs = (MEM_SIZE_VALUE-10)<<(CL_SIZE) ; cout<<"Error!"<<endl; }
				cout<<"@Node "<<(unsigned int) inode<<hex<</*" Processor "<<hex<<(unsigned int)pnode<<*/" rw: "<<_read_or_write<<" from 0x"<< addrs<<dec<<endl;
                cout<<"@Node "<<(unsigned int)inode<<hex<</*" Processor "<<hex<<(unsigned int)pnode<<*/ "time now:" <<(sc_time_stamp().to_double() / 1000) << dec<<endl;
						
				datatype data;
				sc_time start = sc_time_stamp();				
				if (_read_or_write == 1) // read
				{
					ReqFormat_processor reqB;
					ADDRESS_DB_OUT = addrs;
					REQ_COUNT_OUT = ((inode << MAX_PROCESSOR_COUNT_WIDTH) | pnode);
					WE_DB_OUT = false;
					//wait();							
					CE_DB = true;
					//wait(SC_ZERO_TIME);
					//cout<< "@Node "<<(unsigned int)inode<< " CE:"<<CE_DB<<dec<<endl;
					wait();
					CE_DB = false;
					reqB.rw = false;
					reqB.address = addrs;
					reqB.req_count =((inode << MAX_PROCESSOR_COUNT_WIDTH) | pnode);
					reqB.start = start;
					blocked_req_Q.push(reqB);
					wait();
				} else
				{
					for(int i=0;i<8;i++)
					{
						data.data[i] = inode*(i+1);
					}
					ReqFormat_processor reqB;
					DATA_DB_OUT->write_data(&data);
					wait();
					ADDRESS_DB_OUT = addrs;
					REQ_COUNT_OUT = ((inode << MAX_PROCESSOR_COUNT_WIDTH)| pnode);
					WE_DB_OUT = true;
					CE_DB = true;
					//wait(SC_ZERO_TIME);
					wait();
					//cout<< "@Node "<<(unsigned int)inode<<" CE:"<<CE_DB<<dec<<endl;
					CE_DB = false;
					reqB.rw = true;
					reqB.address = addrs;
					reqB.req_count =((inode << MAX_PROCESSOR_COUNT_WIDTH) | pnode);
					reqB.data = data;
					reqB.start = start;
					blocked_req_Q.push(reqB);
					wait();
				}						
				
				}
				else
				{
					//cout<<"error how come??"<<endl;
					wait();
				}	
			}
			else {
			cout<<"Error1" <<endl;
			wait();
			}
		}
		else {
		cout<<"Error2" <<endl;
		wait();
		}
	}
	else
	{
		cout<<"@Node "<<(unsigned int)inode<<" Processor "<<hex<<(unsigned int)pnode<<" blocked req count:" <<(unsigned int)blocked_req_Q.size()<<dec<<" end_of_file_time_taken:"<<time_taken <<endl;
		break;
	}
}
//sc_stop();
do {
	if(resend == true) {
		{
			if (resendreq_B.rw == true)
			{
				DATA_DB_OUT->write_data(&resendreq_B.data);
				wait();
			}
			ADDRESS_DB_OUT = resendreq_B.address;
			REQ_COUNT_OUT = resendreq_B.req_count+1;
			resendreq_B.req_count = resendreq_B.req_count+1;
			WE_DB_OUT = resendreq_B.rw;
			CE_DB = true;
			wait();
			CE_DB = false;
			blocked_req_Q.push(resendreq_B);
			resend = false;
			wait();
		} 
	} else 
		wait(); 
}while(blocked_req_Q.empty() == false);
cout<<"@Node "<<(unsigned int)inode<<" Done by Pr: "<<(unsigned int)pnode<<"and the total time taken is "<<time_taken<<"ns"<<endl;
while(1) wait();
}

void PR_REQ_GEN::ack_reception (void) {
	ReqFormat_processor reqB;
	bool completed;
	completed = false;
	if(blocked_req_Q.empty() != true) {
		ReqFormat_processor reqBack = blocked_req_Q.back();
		do {
			reqB = blocked_req_Q.front();
			if((completed == false) &&(reqB.address == ADDRESS_DB_IN) && (reqB.rw == WE_DB_IN) && (reqB.req_count == REQ_COUNT_IN)) {			
				if(STATUS_DB == false) 
					{
						// send a resend req command 
						resend = true;
						cout<<"@Node "<<(unsigned int)inode<<" RESEND"<<endl;
						completed = true;
						resendreq_B.address = reqB.address;
						resendreq_B.rw = reqB.rw;
						resendreq_B.req_count = reqB.req_count;
						if (reqB.rw == true)
						{
							resendreq_B.data = reqB.data;
						}
						if (reqB != reqBack)
							blocked_req_Q.pop();
						else {
							blocked_req_Q.pop();
							break;
						}
					}
				else 
					{
						cout<<flush;
						time_taken = time_taken+(sc_time_stamp() - reqB.start).to_double()/1000;
						cout<<"@Node "<<(unsigned int)inode<<hex<</*" Processor "<<hex<<(unsigned int)pnode<<*/" the total processing time for address:0x"<<reqB.address<<" = "<<dec<<(sc_time_stamp() - reqB.start).to_double()/1000<< dec<<" Qsize:"<<(unsigned int)blocked_req_Q.size()<< endl;						
						completed = true;
						if (reqB != reqBack)
							blocked_req_Q.pop();
						else {
							blocked_req_Q.pop();
							break;
						}
					}
			} else {
				blocked_req_Q.pop();
				blocked_req_Q.push(reqB);
			}
		} while (reqB != reqBack);
	}
}
