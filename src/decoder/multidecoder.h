#ifndef __MULTIDECODER_H
#define __MULTIDECODER_H

#include <iostream>
#include <queue>
#include "decoder.h"
#include "h264decoder.h"
#include "../observer/inputobserver.h"
#include "../observer/decoderobserver.h"

extern "C" {
#include <x264.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/frame.h>
}

class MultiH264Decoder : public Decoder, public InputObserver, public DecoderObserver{
public:
	explicit MultiH264Decoder(std::string mode);
	virtual ~MultiH264Decoder();

	// Observers' functions override.
    virtual void onEncodedDataReceived(int id, uint8_t type, uint8_t* data, int size);
	virtual void onDecodeFrameSuccess(int id, AVFrame *Frame);
	virtual void onDecodeFrameSuccess(int id, AVFrame *lFrame, AVFrame *rFrame){}


protected:
	virtual void mergedOutput(AVFrame *frame);

	H264Decoder _decoders[2];
	std::string _mode;
};

#endif // __MULTIDECODER_H