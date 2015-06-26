#ifndef __DECODER_H
#define __DECODER_H

#include <stdint.h>
#include "../observer/decoderobserver.h"

class Decoder{
public:
	void setDecoderObserver(int id, DecoderObserver* observer){
		this->_observer = observer; 
		this->_id = id;
	}
protected:

	DecoderObserver* _observer;
	int _id;
};

#endif // __DECODER_H