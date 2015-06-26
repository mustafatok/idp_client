#ifndef __H264DECODER_H
#define __H264DECODER_H

#include <functional>
#include <queue>
#include "decoder.h"
#include "../observer/inputobserver.h"

extern "C" {
#include <x264.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/frame.h>
}

class H264Decoder : public Decoder, public InputObserver{
public:
	explicit H264Decoder();
	virtual ~H264Decoder();

	virtual void decode(uint8_t type, uint8_t* data, int size);

	// Observer Interface
	virtual void onEncodedDataReceived(int id, uint8_t type, uint8_t* data, int size){
		this->decode(type, data, size);
	}

protected:
	virtual void decodeHeader(uint8_t *data, int size);
	virtual void decodeFrame(uint8_t *data, int size);
	virtual void postProcessFrame();

	AVCodec *codec = nullptr;
	AVCodecContext *codecContext = nullptr;
	AVFrame *picture = nullptr;
	AVPacket packet;
};

#endif // __H264DECODER_H