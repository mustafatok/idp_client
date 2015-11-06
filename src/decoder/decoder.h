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
	void releaseObserver(){
		_observer = nullptr;
	}
protected:

	DecoderObserver* _observer = nullptr;
	int _id;
};

#endif // __DECODER_H