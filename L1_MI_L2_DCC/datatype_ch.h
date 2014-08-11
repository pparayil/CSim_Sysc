#ifndef DATATYPE_CH_H
#define DATATYPE_CH_H

#include "systemc.h"
#include "write_read_if.h"


class datatype_ch : public write_read_if, public sc_module {
public:
bool write_data(datatype *data);
bool read_data(datatype *data);
datatype storage_variable;
sc_mutex hand;
SC_CTOR(datatype_ch) 
{
	//hand.unlock();
	int i = CL_WIDTH;
	while(i>0) {
	storage_variable.data[i] = 0x0;
	i--;
	}
}
// other declarations like in an SC_MODULE
};

#endif

