#ifndef __MULTIDECODER_H
#define __MULTIDECODER_H

#include <functional>
#include <queue>
#include "decoder.h"

extern "C" {
#include <x264.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/frame.h>
}

class MultiH264Decoder : public H264Decoder {
public:
	void decode(uint8_t type, uint8_t* data, int size){
		H264Decoder::decode(type, data, size);
	}
protected:
	virtual void postProcessFrame();
};

#endif // __MULTIDECODER_H