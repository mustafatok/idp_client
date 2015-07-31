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
	explicit MultiH264Decoder(int mode);
	virtual ~MultiH264Decoder();

	// Observers' functions override.
    virtual void onEncodedDataReceived(int id, uint8_t type, uint8_t* data, int size);
	virtual void onDecodeFrameSuccess(int id, AVFrame *Frame);
	virtual void onDecodeFrameSuccess(int id, AVFrame *lFrame, AVFrame *rFrame){}


protected:
	virtual void verticalConcat(AVFrame *frame);
	void deserializeAndDecode(int id, uint8_t type, uint8_t* data, int size);


	H264Decoder _decoders[2];
	int _mode;

	int _tmpCnt = 0;
	AVFrame *lFrame;
	AVFrame *rFrame;
};

#endif // __MULTIDECODER_H