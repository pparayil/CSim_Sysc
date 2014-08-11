

#include "LisNoCProcessingElementunit.h"

using namespace std;
#if 0
void LisNoCProcessingElement::rxProcess(void)
{
while(1){
    if (rst.read()) {
	link_out_ready_i.write(0x3);
	//cout<<"LisNoCProcessingElement rst "<<__func__<<" : "<<__LINE__<<endl;
	wait();
	continue;
    } else {
		//cout<<"receive pkt"<<endl;
		LisNocPacket pkt;
		unsigned int vc;
		int flit_left;
		bool complete_pkt_received = false;
		link_out_ready_i.write(0x3);
		while(link_out_valid_o.read() == 0x0) { wait(); //cout<<" rx waiting"<<endl; 
		}
		vc = (unsigned int)((link_out_valid_o.read() & 0x1) == 0x1) ? (0x1) : 0x2;
		pkt.vc = vc;
		while(1){	
		if (link_out_valid_o.read() != 0x0 /*&&  (pkt.vc == vc)*/) { //link_out_flit_o.read() != 0 &&
			vluint64_t flit_tmp = link_out_flit_o.read();
			link_out_ready_i.write(0x0);

			if((((flit_tmp>>32) & 0x1) == 0x1)) // header flit or single sized flit 01 or 11
			{
				//cout<<"received at node "<<((flit_tmp>>27) & 0x1f)<< " from " << ((flit_tmp>>15) & 0x1f)<<" type "<< ((flit_tmp>>6) & 0x0f)<< endl;
				pkt.en = (flit_tmp >> 0) & (0x1) ;
				pkt.ack = (flit_tmp>> 1) & (0x1) ;
				pkt.inv = (flit_tmp >> 2) & (0x1) ;
				pkt.ex = (flit_tmp >> 3) & (0x1);
				pkt.evt = (flit_tmp >> 4) & (0x1)   ;
				pkt.read_or_write = ((flit_tmp>>5) & 0x1);
				pkt.type = ((flit_tmp>>6) & 0x0f);
				pkt.redirectionnode = ((flit_tmp>>10) & 0x0f); //(0x0ffffff) ;
				pkt.src_id = ((flit_tmp>>14) & 0x0f);
				pkt.size = (((flit_tmp>>18) & 0x01) == 1)? 10:2;
				pkt.req_count = ((flit_tmp>>19) & 0x0ff);
				pkt.dst_id = ((flit_tmp>>27) & 0x1f);
				flit_left = pkt.size - 1 ;
				//cout<<"@ "<<local_id<<" header Flit received 0x"<<hex<<flit_tmp<<" with type "<<pkt.type<<" "<<pkt.evt<<" "<<pkt.ex<<" "<<pkt.inv<<" "<<pkt.ack<<" "<<pkt.en<<dec<<endl;
			}
			else if ((pkt.size == 2 && (((flit_tmp>>32) & 0x3) == 0x2)) || (flit_left == (pkt.size - 1)) ) // 2 flit will have 2nd flit for data 
			{
				pkt.address.push_back((unsigned int) flit_tmp & 0xffffffff);
				flit_left--;
				//cout<<"@ "<<local_id<<" second flit received Flit received: "<<hex<<flit_tmp<<dec<<endl;
			}
			else
			{
				pkt.data.push_back((unsigned int) flit_tmp & 0xffffffff);
				flit_left--;
				if((((flit_tmp>>32) & 0x3) == 0x2) && (flit_left != 0)) {
//				   if(local_id == 0xd) pkt.address.push_back((unsigned int)0x1fec0  & 0xffffffff);
				   cout<<"@ "<<local_id<<" Error! body flit received Flit received: "<<hex<<flit_tmp<<dec<<endl;
				   flit_left = 0;
				}
			}
			
			cout << (sc_time_stamp().to_double() / 1000) << "ns : ProcessingElement[" << local_id << "] RECEIVED " << hex <<flit_tmp <<dec<< endl;

			if((local_id != NODE_WITH_SDRAM) && ((flit_left == 0) &&(((flit_tmp>>32) & 0x3) == 0x2))){
				//cout<<__func__<<" : "<<__LINE__<<endl;
				node_internals->receive_packet(local_id, &pkt) ;
//				wait();
				break;
				}
			else if((flit_left == 0) &&(((flit_tmp>>32) & 0x3) == 0x2)) {
				//cout<<__func__<<" : "<<__LINE__<<endl;
				memnode_internals->receive_packet(local_id, &pkt) ;
//				wait();
				break;
				}
			
		}
		else 
			link_out_ready_i.write(0x3);

		//cout<<" ***************** "<<endl;
		wait();
		vc = (unsigned int)((link_out_valid_o.read() & 0x1) == 0x1) ? (0x1) : 0x2;
		}
	wait();
	}
}  
}
#else
void LisNoCProcessingElement::rxProcess(void)
{

unsigned int vc;
    if (rst.read()) {
	link_out_ready_i.write(0x3);
	//noc_packet_counter_out = 0x0;	
    } else {
	if (link_out_valid_o.read() == 0x0) { 
		link_out_ready_i.write(0x3);
		//noc_packet_counter_out = 0x0;
	} else {
		vc = (unsigned int)((link_out_valid_o.read() & 0x1) == 0x1) ? (0x1) : 0x2;		
		link_out_ready_i.write(0x0);
		flit_tmp = link_out_flit_o.read();
		flit_filled = true;
		cout << (sc_time_stamp().to_double() / 1000) << "ns : ProcessingElement[" << local_id << "] RECEIVED " << hex <<flit_tmp <<dec<< endl;
		if(noc_out_done == true) {
			noc_packet_counter_out = 0x1;
			noc_out_done = false;
		} else
			noc_packet_counter_out ++;
		
		//noc_packet_counter_out ++;			
	}
     }
}

void LisNoCProcessingElement::rxpacketizer(void)
{
LisNocPacket pkt;
int flit_left;
bool complete_pkt_received = false;
while(1){
	if(flit_filled == false) {
		wait();
	} else {
		if((((flit_tmp>>32) & 0x1) == 0x1)) // header flit or single sized flit 01 or 11
		{
			//cout<<"received at node "<<((flit_tmp>>27) & 0x1f)<< " from " << ((flit_tmp>>15) & 0x1f)<<" type "<< ((flit_tmp>>6) & 0x0f)<< endl;
			pkt.en = (flit_tmp >> 0) & (0x1) ;
			pkt.ack = (flit_tmp>> 1) & (0x1) ;
			pkt.inv = (flit_tmp >> 2) & (0x1) ;
			pkt.ex = (flit_tmp >> 3) & (0x1);
			pkt.evt = (flit_tmp >> 4) & (0x1)   ;
			pkt.read_or_write = ((flit_tmp>>5) & 0x1);
			pkt.type = ((flit_tmp>>6) & 0x0f);
			pkt.redirectionnode = ((flit_tmp>>10) & 0x0f); //(0x0ffffff) ;
			pkt.src_id = ((flit_tmp>>14) & 0x0f);
			pkt.size = (((flit_tmp>>18) & 0x01) == 1)? 10:2;
			pkt.req_count = ((flit_tmp>>19) & 0x0ff);
			pkt.dst_id = ((flit_tmp>>27) & 0x1f);
			flit_left = pkt.size - 1 ;
			//cout<<"@ "<<local_id<<" header Flit received 0x"<<hex<<flit_tmp<<" with type "<<pkt.type<<" "<<pkt.evt<<" "<<pkt.ex<<" "<<pkt.inv<<" "<<pkt.ack<<" "<<pkt.en<<dec<<endl;
		}
		else if ((pkt.size == 2 && (((flit_tmp>>32) & 0x3) == 0x2)) || (flit_left == (pkt.size - 1)) ) // 2 flit will have 2nd flit for data 
		{
			pkt.address.push_back((unsigned int) flit_tmp & 0xffffffff);
			flit_left--;
			//cout<<"@ "<<local_id<<" second flit received Flit received: "<<hex<<flit_tmp<<dec<<endl;
		}
		else
		{
			pkt.data.push_back((unsigned int) flit_tmp & 0xffffffff);
			flit_left--;
			if((((flit_tmp>>32) & 0x3) == 0x2) && (flit_left != 0)) {
	//				   if(local_id == 0xd) pkt.address.push_back((unsigned int)0x1fec0  & 0xffffffff);
			   cout<<"@ "<<local_id<<" Error! body flit received Flit received: "<<hex<<flit_tmp<<dec<<endl;
			   pkt.data.push_back((unsigned int) flit_tmp & 0xffffffff);
			   flit_left = 0;
			}
		}


		if((local_id != NODE_WITH_SDRAM) && ((flit_left == 0) &&(((flit_tmp>>32) & 0x3) == 0x2))){
			//cout<<__func__<<" : "<<__LINE__<<endl;
			flit_filled = false;
			wait();
			node_internals->receive_packet(local_id, &pkt) ;
		}
		else if((flit_left == 0) &&(((flit_tmp>>32) & 0x3) == 0x2)) {
			//cout<<__func__<<" : "<<__LINE__<<endl;
			flit_filled = false;
			wait();
			memnode_internals->receive_packet(local_id, &pkt) ;

		}else {
			flit_filled = false;
			wait();
			
		}
		
	} 
} 
}
#endif
void LisNoCProcessingElement::txProcess(void)
{

    if (rst.read()) {
	link_in_flit_i.write(0);
	link_in_valid_i = 0x0;
	transmittedAtPreviousCycle = false;	
    	//cout<<"LisNoCProcessingElement rst "<<__func__<<" : "<<__LINE__<<endl;
	//noc_packet_counter_in = 0x0;
    } else {
	//cout<<"make pkt";
	LisNocPacket packet;	
	if (canShot(packet)) {
	    packet.vc = 0; 
	    flip  = true;
	    packet_queue.push(packet);  	         
	    transmittedAtPreviousCycle = true;
	    }
    	else
	    transmittedAtPreviousCycle = false;

	
	if((link_in_ready_o.read() & 0x1) == 0x1) {
	    if (!packet_queue.empty()) {
			if(packet_queue.front().vc == 0)
				packet_queue.front().vc = 1;

			//cout<<local_id<<": 1 : vc : "<<packet_queue.front().vc<<endl;

			if((packet_queue.front().vc == 1)  && (flip == true)) {
				//cout<<local_id<<": send flit from "<< packet_queue.front().src_id <<" to "<<packet_queue.front().dst_id<<" through vc 1"<<endl;
				vluint64_t flit = nextFlit(false);	// Generate a new flit
				//if(packet.src_id != packet.dst_id)
				link_in_flit_i = (vluint64_t) (flit);	// Send the generated flit
				link_in_valid_i = 1; //packet_queue.front().vc; //0x3;				
				flip = !flip;
				if(noc_in_done == true) {
					noc_packet_counter_in = 0x1;
					noc_in_done = false;
				} else
					noc_packet_counter_in ++; //= 0x1;
				//noc_packet_counter_in ++;		
			}
			else
			{
				//link_in_flit_i = 0;
				link_in_valid_i = 0x0;
				flip  = true;
				//noc_packet_counter_in = 0x0;
			}
		}
	     else
		{
		//link_in_flit_i = 0;
		link_in_valid_i = 0x0;
		flip  = true;
		//noc_packet_counter_in = 0x0;
		}
	}
	else if((link_in_ready_o.read() & 0x2) == 0x2) {
	     if (!packet_queue.empty()) {
			if(packet_queue.front().vc == 0)
				packet_queue.front().vc = 2;
			//cout<<local_id<<": 2: vc : "<<packet_queue.front().vc<<endl;
			if((packet_queue.front().vc == 2)  && (flip == true)) {
				//cout<<local_id<<": send flit from "<< packet_queue.front().src_id <<" to "<<packet_queue.front().dst_id<<" through vc 2"<<endl;
				vluint64_t flit = nextFlit(true);	// Generate a new flit
				//if(packet.src_id != packet.dst_id)
				link_in_flit_i = (vluint64_t) flit;	// Send the generated flit
				link_in_valid_i = 2; //packet_queue.front().vc; //0x3;
				flip = !flip;
				if(noc_in_done == true) {
					noc_packet_counter_in = 0x1;
					noc_in_done = false;
				} else
					noc_packet_counter_in ++; //= 0x1;
				//noc_packet_counter_in ++;		
			}
			else
			{
				//link_in_flit_i = 0;
				link_in_valid_i = 0x0;
				flip  = true;
				//noc_packet_counter_in = 0x0;
			}
		}
	     else
		{
		//link_in_flit_i = 0;
		link_in_valid_i = 0x0;
		flip  = true;
		//noc_packet_counter_in = 0x0;
		}
	
	}
	else {
	//link_in_flit_i = 0;
	link_in_valid_i = 0x0;
	flip  = true;
	//noc_packet_counter_in = 0x0;
	cout<<"LisNoCProcessingElement::txProcess,"<< __LINE__<<" Error"<<endl;
	}
    }

}

vluint64_t LisNoCProcessingElement::nextFlit(bool vc)
{
    vluint64_t flit = 0;
    LisNocPacket packet = packet_queue.front();
	/*bool en; 
	bool ex;
	bool evt;
	bool ack;
	int redirectionnode;
	bool inv;
	int type; // 4 bits as cud b maximum 16 different types.
	cntrl = pkt_size|src|redirectionnode|t|y|p|e|rw|evt|ex|inv|ack|en*/
	bool size = 0; 
	if(packet.size == 2) 
		size = 0;
	else
		size = 1;
	if(packet.size == 1) 
		{
		flit = (vluint64_t)(0x3)<< 32;
		flit = flit | ((((unsigned int) packet.dst_id & (NO_OF_NODES-1))<<27) | (((unsigned int) packet.req_count & 0x0ff)<<19) |  ((size & 0x01)<<18) | (((unsigned int) packet.src_id & (NO_OF_NODES-1))<<14) | ((((unsigned int) packet.redirectionnode&(NO_OF_NODES-1))<<10))| (((unsigned int) packet.type & 0x0f) << 6) |  (((bool) packet.read_or_write & 0b1) << 5) | (((bool) packet.evt& 0b1) << 4 )| (((bool) packet.ex & 0b1 ) << 3 )| (((bool) packet.inv & 0b1) << 2) | (((bool) packet.ack & 0b1) <<1 ) | (((bool) packet.en & 0b1 )));
		}
	else if (packet.size == packet.flit_left)
		{
		flit = (vluint64_t)( 0x1)<< 32;
		flit = flit | ((((unsigned int) packet.dst_id & (NO_OF_NODES-1))<<27) |  (((unsigned int) packet.req_count & 0x0ff)<<19) |  ((size & 0x01)<<18) | (((unsigned int) packet.src_id & (NO_OF_NODES-1))<<14) | ((((unsigned int) packet.redirectionnode&(NO_OF_NODES-1))<<10))| (((unsigned int) packet.type & 0x0f) << 6) |  (((bool) packet.read_or_write & 0b1) << 5) | (((bool) packet.evt& 0b1) << 4 )| (((bool) packet.ex & 0b1 ) << 3 )| (((bool) packet.inv & 0b1) << 2) | (((bool) packet.ack & 0b1) <<1 ) | (((bool) packet.en & 0b1 )));
		}
	else if (packet.size == 2)
		{
		flit = (vluint64_t) ( 0x2)<< 32;
		flit = flit | ((unsigned int) packet.address.back());
		packet.address.pop_back();
		}
	else if (packet.size == packet.flit_left+1) // second flit which carries address
		{
		flit = (vluint64_t)( 0x0)<< 32;
		flit = flit | ((unsigned int) packet.address.back());
		packet.address.pop_back();
		}
	else if (packet.flit_left == 1) //tail flit
	{
		flit = (vluint64_t) (0x2)<< 32;
		flit = flit | ((unsigned int)packet.data.back());
		packet.data.pop_back();
	}
	else {
		flit = (vluint64_t) (0x0)<< 32; // body flit
		flit = flit | ((unsigned int)packet.data.back());
		packet.data.pop_back();
	}
	cout<<"@node: "<< local_id <<" flit prepared is 0x"<<hex<< flit<<dec<<" vc:"<<((vc == true)? 0x2: 0x1)<<endl;
	packet_queue.front().flit_left--;
	if ((packet_queue.front().flit_left < 1) && (packet_queue.empty() == false)) {
		packet_queue.pop();
	} else if ((packet_queue.front().flit_left < 1))
		cout<<inode<<" "<<__func__<<" : "<<__LINE__<<" Error"<<endl; 
	return flit;
}

bool LisNoCProcessingElement::canShot(LisNocPacket & packet)
{
    bool shot;
    double threshold;
    //cout<<__func__<<" : "<<__LINE__<<endl;
///////////////////////////////////////////////////////////////////////////
	/*changed preethi ++*/
	
	double now = (int) (sc_time_stamp().to_double() / 1000);
	double prob = (double) rand() / RAND_MAX;
	shot = false;
	if((prob < 1)) {
		packet.make(local_id, 0, now, 1); //getRandomSize());
		if((local_id != NODE_WITH_SDRAM)){
		  shot = node_internals->set_packet(local_id, &packet);
		  //packet.size = 3;
		  //packet.flit_left = 3;
		  if(shot == true) {
		    //cout<<local_id<<" @ "<<__func__<<" type "<<packet.type<<" "<<packet.evt<<" "<<packet.ex<<" "<<packet.inv<<" "<<packet.ack<<" "<<packet.en<<" "<<packet.size<<endl; //" "<<packet.size<<
		   }
		}
		else{	 

		shot = memnode_internals->set_packet(local_id, &packet);
		//packet.size = 3;
		//packet.flit_left = 3;
		if(shot == true) {
		    //cout<<local_id<<" @ "<<__func__<<" type "<<packet.type<<" "<<packet.evt<<" "<<packet.ex<<" "<<packet.inv<<" "<<packet.ack<<" "<<packet.en<<" "<<packet.size<<endl;//" "<<packet.size<<
		}
		}
		if(packet.read_or_write == 2){ 
			//LisNoCProcessingElement::internal++;
			//cout<<"error no internal type"<<endl;
	        }
		if(packet.read_or_write == 0){
                //cout<<"At "<<(sc_time_stamp().to_double() / 1000)<<" ";
                //cout<<"Node "<<local_id<<" reading from SDRAM"<<endl;
			//LisNoCProcessingElement::read++;
		}
		if(packet.read_or_write == 1){
                //cout<<"At "<<(sc_time_stamp().to_double() / 1000)<<" ";
                //cout<<"Node "<<local_id<<" writing to SDRAM"<<endl;
			//LisNoCProcessingElement::write++;
		}
			
		
	    
	}

    return shot;
}

long LisNoCProcessingElement::total_noc_count_in (void) {
	long x = noc_packet_counter_in;
	//if(x > 0xfffffffffffffffe) cout<<inode<<" LisNoCProcessingElement:: count greater than expected!"<<endl;
	noc_in_done = true;		
	noc_out_done = true;
	return (x); 	
}

long LisNoCProcessingElement::total_noc_count_out (void) {
	long x = noc_packet_counter_in - noc_packet_counter_out;
	//if(x > 0xfffffffffffffffe) cout<<inode<<" LisNoCProcessingElement:: count greater than expected!"<<endl;
	noc_in_done = true;		
	noc_out_done = true;
	return (x); 	
}


