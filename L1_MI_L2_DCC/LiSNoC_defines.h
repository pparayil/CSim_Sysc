#ifndef __LISNOCDEFINES_H__
#define __LISNOCDEFINES_H__

#include <systemc.h>
#include <vector>
using namespace std;
enum LisNocFlitType {
    FLIT_TYPE_HEAD, FLIT_TYPE_BODY, FLIT_TYPE_TAIL
};


struct LisNocPayload {
    sc_uint<64> data;	// Bus for the data to be exchanged

    inline bool operator ==(const LisNocPayload & payload) const {
	return (payload.data == data);
}};

struct LisNocFlit {
	int src_id;
	int dst_id;
	LisNocFlitType flit_type;	// The flit type (FLIT_TYPE_HEAD, FLIT_TYPE_BODY, FLIT_TYPE_TAIL)
	int sequence_no;		// The sequence number of the flit inside the packet
	LisNocPayload payload;	// Optional payload
	double timestamp;		// Unix timestamp at packet generation
	int hop_no;			// Current number of hops from source to destination
	///////////////////////////////////////////////////////////////////////////
	int read_or_write;
	int cntrl; /*preethi*/
	bool single_flit_p;
	vector< int > data;
	vector< int > address;
	///////////////////////////////////////////////////////////////////////////
	inline bool operator ==(const LisNocFlit & flit) const {
	return (flit.src_id == src_id && flit.dst_id == dst_id
		&& flit.flit_type == flit_type
		&& flit.sequence_no == sequence_no
		&& flit.payload == payload 
		&& flit.timestamp == timestamp
		&& flit.hop_no == hop_no
		&& flit.read_or_write == read_or_write
		&& flit.data == data
		&& flit.address == address
		&& flit.cntrl == cntrl && flit.single_flit_p == single_flit_p);
}

};


struct LisNocPacket {
	int src_id;
	int dst_id;
	double timestamp;		// SC timestamp at packet generation
	int size;
	int flit_left;		// Number of remaining flits inside the packet
	int read_or_write;
	bool en;/*changed preethi ++*/
	bool ex;
	bool evt;
	bool ack;
	bool inv;
	unsigned int vc;
	unsigned int redirectionnode;
	unsigned int type; /*changed preethi --*/
	int burst;
	int offset;
	vector < int > address;
	vector < int > data;
	unsigned int req_count;
	// Constructors
	LisNocPacket() { }
	LisNocPacket(const int s, const int d, const double ts, const int sz) {
	make(s, d, ts, sz);
	}

	void make(const int s, const int d, const double ts, const int sz) {
	src_id = s;
	dst_id = d;
	timestamp = ts;
	size = sz;
	flit_left = sz;
	read_or_write = 0;
	burst = 0;
	offset = 0;
	en = false;/*changed preethi ++*/
	ex = false;
	evt = false;
	ack = false;
	redirectionnode = 0;
	req_count = 0;
	inv = false; /*changed preethi --*/
	type = 0x0;
	}

};


#endif //LISNOCDEFINES

