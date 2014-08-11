
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
	mem_access_count_wr = 0x0;
	mem_access_count_rd = 0x0;
	cout<<"MEM::"<<inode<<" : "<<__func__<<" : "<<__LINE__<<endl;
}
void MEM::MEM_req_collector(void){
	if((ce_from_dir == true) && (address_from_l2_en == false)) {
		NoCCommPacket pkt;
		pkt.en = true;
		pkt.rw = rd_wr_from_dir;
		pkt.address = addr_from_dir;
		pkt.req_count = req_count_from_dir;
		pkt.redirectionnode = dst_nodeaddr_from_dir;
		pkt.srcnode = nodeaddr_from_lru_dir;	
		if(without_dir_ack == true)
			pkt.ex = true;
		else
			pkt.ex = false;
		ReqQ1.push(pkt);
		cout<<"MEM::"<<inode<<" : "<<__func__<<" : "<<__LINE__<<" : Q size: "<<ReqQ1.size()<<" WR_RD: "<<rd_wr_from_dir<<hex<<" 0x"<<pkt.address<< dec<<endl;	
		if(pkt.rw == true){
		cout<<"MEM::"<<inode<<" : "<<__func__<<" : "<<__LINE__<<" l2node:"<<pkt.redirectionnode<<endl;
		}
		//wait();		
	} 
	else if ((address_from_l2_en == true) && (ce_from_dir == false)) {
		NoCCommPacket pkt;
		pkt.en = address_from_l2_en;	
		pkt.ex = false;
		pkt.address = address_from_l2;	
		pkt.req_count = req_count_from_l2;
		pkt.srcnode = nodeaddr_from_lru_l2;
		data_from_l2->read_data(&pkt.data);
		ReqQ2.push(pkt);
		cout<<"MEM::"<<inode<<" : "<<__func__<<" : "<<__LINE__<<" : Q size: "<<ReqQ2.size()<<" data for address:"<<hex<< pkt.address<<dec<<" l2node:"<<pkt.srcnode<<endl;
		//wait();
	} 
	else if ((address_from_l2_en == true) && (ce_from_dir == true)) {
		NoCCommPacket pkt, pkt1;
		pkt.en = true;
		pkt.rw = rd_wr_from_dir;
		pkt.address = addr_from_dir;
		pkt.req_count = req_count_from_dir;
		pkt.redirectionnode = dst_nodeaddr_from_dir;
		pkt.srcnode = nodeaddr_from_lru_dir;	
		if(without_dir_ack == true)
			pkt.ex = true;
		else
			pkt.ex = false;
		ReqQ1.push(pkt);
		pkt1.en = address_from_l2_en;		
		pkt1.address = address_from_l2;	
		pkt1.req_count = req_count_from_l2;
		pkt1.srcnode = nodeaddr_from_lru_l2;
		data_from_l2->read_data(&pkt1.data);
		ReqQ2.push(pkt1);
		cout<<"MEM::"<<inode<<" : "<<__func__<<" : "<<__LINE__<<" req + data for address:"<<hex<< pkt.address<<dec<<endl;
		//wait();
	}

}
void MEM::MEM_self(void){
cout<<__func__<<" : "<<__LINE__<<endl;
bool req_completed = true;
bool found = false;
while(1)
{	
	if(ReqQ1.empty() == false)
	{
		NoCCommPacket pkt1,pkt2,temp,back;
		pkt1 = ReqQ1.front();		
		if(pkt1.rw == true)
		{
			found = false;
			if(ReqQ2.empty() == true) {
				// 
				ReqQ1.pop();
				ReqQ1.push(pkt1);
				wait();
				continue;
			}
			/*int address_dir = addr_from_dir;
			int dst_l2node_ = dst_nodeaddr_to_dir;
			int dst_dirnode_ = nodeaddr_from_lru;*/
			//cout<<"MEM::"<<inode<<" : "<<__func__<<__LINE__<<" waiting for data for address: 0x"<<hex<< pkt1.address<<" reqcnt "<<pkt1.req_count <<" for l2:" << pkt1.redirectionnode<<dec<<endl;		
			back = ReqQ2.back();
			//cout<<"MEM::"<<inode<<" : "<<__func__<<__LINE__<<" back address: 0x"<<hex<< back.address<< " from l2 "<<back.srcnode <<" for reqcnt:" << back.req_count<<dec<<endl;
			do {
				temp = ReqQ2.front();
				if((temp.address == pkt1.address) && (pkt1.redirectionnode == temp.srcnode) && (temp.req_count == pkt1.req_count )) 
				{ // found the match
					found = true;
					pkt2 = temp;
					ReqQ2.pop();
					//ReqQ1.pop();
					if(!(back != temp))
						break;					
				} else {
					ReqQ2.pop();
					ReqQ2.push(temp);
				}
		//	cout<<"MEM::"<<inode<<" : "<<__func__<<__LINE__<<"received for address: 0x"<<hex<< temp.address<< " from l2 "<<temp.srcnode <<" for reqcnt:" << temp.req_count<<dec<<endl;	
			} while(back != temp);		
			/*if (found == false) {
				back = ReqQ1.back();
				pkt2 = ReqQ2.front();
				do {
					temp = ReqQ1.front();
					if((temp.address == pkt2.address) && (pkt2.redirectionnode == temp.srcnode) && (temp.req_count == pkt2.req_count ) && (temp.rw == true)) 
					{ // found the match
						found = true;
						pkt1 = temp;
						ReqQ1.pop();
						ReqQ2.pop();
						if(!(back != temp))
						break;
					} else {
						ReqQ1.pop();
						ReqQ1.push(temp);
					}
				//cout<<"MEM::"<<inode<<" : "<<__func__<<__LINE__<<" waiting for data for address: 0x"<<hex<< pkt1.address<< " data address received is:"<<pkt2.address<<dec<<endl;
				} while(back != temp);
			} */
			if (found == false) {
				ReqQ1.pop();
				ReqQ1.push(pkt1);
				//cout<<"MEM::"<<inode<<" : "<<__func__<<__LINE__<<" couln't find data for address: 0x"<<hex<< pkt1.address<<dec<<endl;
				wait();
				continue;
			} 
			else 
			{
				cout<<"MEM::"<<inode<<" : "<<__func__<<__LINE__<<" waiting for write to address: 0x"<<hex<< pkt1.address<< " next address received is:"<<pkt2.address<<dec<<endl;
				wait(MEM_DELAY,SC_NS);
				MEMptr[pkt2.address>>CL_SIZE] = pkt2.data;	
				address_to_l2 = pkt2.address;
				req_count_to_l2 = pkt2.req_count;
				node_address_to_lru = pkt1.redirectionnode;
				//cout<<"MEM::"<<inode<<" : "<<__func__<<" ack from mem to l2: "<<pkt1.redirectionnode<<endl;
				ack_to_l2 = true;
				ack_rw_to_l2 = true;
				wait();
				ack_to_l2 = false;
				wait();
				//cout<<"MEM::"<<inode<<" : "<<__func__<<" written to memory: 0x"<<hex<<pkt2.address<<", first data[0] : 0x"<<MEMptr[pkt2.address>>5].data[0]<<" for l2:"<< pkt2.srcnode<<dec<<endl;
				if(pkt1.ex == false) {
					node_address_to_lru = pkt1.srcnode; 
					ack_to_dir = true;
					addr_to_dir = pkt1.address;
					req_count_to_dir = pkt1.req_count;
					wait();
					ack_to_dir = false;
					wait();
				}	
				ReqQ1.pop();
				cout<<"MEM::"<<inode<<" : "<<"write COMPLETE"<<endl;
				mem_access_count_wr ++;
			}
		}
		else
		{
			cout<<"MEM::"<<inode<<" : "<<"read from memory: 0x"<<hex<<(pkt1.address)<<", data[0] : 0x"<<MEMptr[(pkt1.address>>5)].data[0]<<dec<<endl;
			wait(MEM_DELAY,SC_NS);
			datatype data = MEMptr[pkt1.address>>CL_SIZE];			
			address_to_l2 = pkt1.address;
			req_count_to_l2 = pkt1.req_count;
			node_address_to_lru = pkt1.redirectionnode;
			data_to_l2->write_data(&data);
			ack_to_l2 = true;
			ack_rw_to_l2 = false;
			wait();
			ack_to_l2 = false;
			wait();
			if(pkt1.ex == false) {
				node_address_to_lru =pkt1.srcnode ; 
				ack_to_dir = true;
				addr_to_dir = pkt1.address;
				req_count_to_dir = pkt1.req_count;
				wait();
				ack_to_dir = false;
				wait();
			}
			//cout<<"MEM::"<<inode<<" : "<<"ack from mem to l2: "<<pkt1.redirectionnode<<" and to dir: "<< pkt1.srcnode <<" req_count:"<< pkt1.req_count<<endl;		
			ReqQ1.pop();
			cout<<"MEM::"<<inode<<" : "<<"read COMPLETE"<<endl;
			mem_access_count_rd ++;
		}
	
	
	}
	else
	{	
		wait();
	}
}
}


