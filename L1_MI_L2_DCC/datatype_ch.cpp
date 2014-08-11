

#include "datatype_ch.h"

bool datatype_ch::write_data(datatype *data) {
	//hand.lock();
	if(data != NULL)
	storage_variable = *data;
	else
	cout<<"NULL data";
	//hand.unlock();
	return(true);

}

bool datatype_ch::read_data(datatype *data) {
	//hand.lock();
	if(data != NULL)
	*data = storage_variable;
	else
	cout<<"NULL data";
	//hand.unlock();
	return(true);

}
