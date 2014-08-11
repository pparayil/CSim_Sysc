
#ifndef WRITE_READ_IF_H
#define WRITE_READ_IF_H

#include "systemc.h"
#include "datatype.h"

class write_read_if : public sc_interface {
public:
virtual bool write_data(datatype *data)=0;
virtual bool read_data(datatype *data)=0;
};

#endif
