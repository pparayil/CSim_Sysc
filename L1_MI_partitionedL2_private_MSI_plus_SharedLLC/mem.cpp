
#include "mem.h"


void MEM::MEM_init(void){
	cout<<"MEM::"<<inode<<" : "<<__func__<<" : "<<__LINE__<<endl;
	MEMptr = new datatype[MEM_SIZE_VALUE];
	if (MEMptr == NULL)
		cout<<"MEM::"<<inode<<" : "<<__func__<<" : "<<__LINE__<<" error!"<<endl;
	for(int i=0;i<MEM_SIZE_VALUE;i++)
	{
		for(int j=0;j<8;j++)
		MEMptr[i].data[j] = 10+i+j;
	}
	mem_access_count_rd = 0x0;
	mem_access_count_wr = 0x0;
	cout<<"MEM::"<<inode<<" : "<<__func__<<" : "<<__LINE__<<endl;
}
void MEM::MEM_req_collector(void){
	while(1){
		if(ce_ == true) {
		NoCCommPacket pkt;
		pkt.en = ce_;
		pkt.rw = rd_wr_;
		pkt.address = addr_to_mem;
		pkt.req_count = req_count_to_mem;
		if(rd_wr_ == true)
			data_->read_data(&pkt.data);
		pkt.srcnode = nodeaddr_from_lru;
		cout<<"MEM::"<<inode<<" : "<<__func__<<" : "<<__LINE__<<" : Q size: "<<ReqQ1.size()<<" WR_RD: "<<rd_wr_<<hex<<" 0x"<<pkt.address<<" frm llc:"<<nodeaddr_from_lru <<dec<<endl;
		ReqQ1.push(pkt);		
	}
		wait();
	}
}
void MEM::MEM_self(void){
cout<<__func__<<" : "<<__LINE__<<endl;

while(1)
{	
	if(ReqQ1.empty() == false)
	{
		NoCCommPacket pkt1;
		pkt1 = ReqQ1.front();
		
		if(pkt1.rw == true)
		{
			cout<<"MEM::"<<inode<<" : "<<__func__<<__LINE__<<" waiting for write to address: 0x"<<hex<< pkt1.address<<dec<<endl;
			wait(MEM_DELAY,SC_NS);
			MEMptr[pkt1.address>>CL_SIZE] = pkt1.data;		
			wait();
			cout<<"MEM::"<<inode<<" : "<<"written to memory: 0x"<<hex<<pkt1.address<<", first data[0] = 0x"<<MEMptr[pkt1.address>>5].data[0]<<dec<<endl;
			node_address_to_lru = pkt1.srcnode; 
			ack_rw = pkt1.rw ;
			ack_ = true;
			addr_from_mem = pkt1.address;
			req_count_from_mem = pkt1.req_count;
			wait();
			ack_ = false;
			wait();
			//cout<<"MEM::"<<inode<<" : "<<"ack from mem to llc: "<<pkt1.srcnode<<endl;
			ReqQ1.pop();
			cout<<"MEM::"<<inode<<" : "<<"write COMPLETE addr:"<<hex<<(pkt1.address)<<dec<<" Qsize:"<<ReqQ1.size()<<endl;
			mem_access_count_wr ++ ;
		}
		else
		{
			cout<<"MEM::"<<inode<<" : "<<"read from memory: 0x"<<hex<<(pkt1.address)<<", req_count"<<pkt1.req_count<<dec<<endl;
			wait(MEM_DELAY,SC_NS);
			datatype data = MEMptr[pkt1.address>>CL_SIZE];
			//cout<<"MEM::"<<inode<<" : "<<"read from memory: 0x"<<hex<<(pkt1.address)<<", data[0] = 0x"<<MEMptr[(pkt1.address>>5)].data[0]<<dec<<endl;
			data_->write_data(&data);
			wait();
			node_address_to_lru = pkt1.srcnode ;
			ack_ = true;
			ack_rw = pkt1.rw ;
			addr_from_mem = pkt1.address;
			req_count_from_mem = pkt1.req_count;
			wait();
			ack_ = false;
			wait();
			//cout<<"MEM::"<<inode<<" : "<<"ack from mem to llc: "<<pkt1.dstnode <<endl;
			ReqQ1.pop();			
			cout<<"MEM::"<<inode<<" : "<<"read COMPLETE addr:"<<hex<<(pkt1.address)<<dec<<" Qsize:"<<ReqQ1.size()<<endl;
			mem_access_count_rd ++ ;
		}
	
	
	}
	else
	{	
		wait();
	}
}
}


