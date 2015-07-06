#include "multidecoder.h"

#include <iostream>

extern "C" {
#include <x264.h>
#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/common.h>
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
#include <libavutil/frame.h>
}

#include <string.h>

#include "../global.h"

MultiH264Decoder::MultiH264Decoder(std::string mode)
{
	this->_mode = mode;
	_decoders[0].setDecoderObserver(LEFT, this);
	_decoders[1].setDecoderObserver(RIGHT, this);
}

MultiH264Decoder::~MultiH264Decoder()
{
}

void MultiH264Decoder::onEncodedDataReceived(int id, uint8_t type, uint8_t* data, int size){
	if(_mode == "verticalConcat"){
		_decoders[0].onEncodedDataReceived(id, type, data, size);
	}else{
		this->deserializeAndDecode(id, type, data, size);
	}
}

void MultiH264Decoder::deserializeAndDecode(int id, uint8_t type, uint8_t* data, int size){
	uint8_t lType, rType;

	lType = data[0];
	rType = data[1];

	uint32_t leftSize = *(reinterpret_cast<uint32_t*>(data + 2));
	uint32_t rightSize = *(reinterpret_cast<uint32_t*>(data + 2 + sizeof(uint32_t)));

	uint8_t* lData = new uint8_t[leftSize + 8];
	uint8_t* rData = new uint8_t[rightSize + 8];

	memcpy(lData, data + 2 +  2*sizeof(uint32_t), leftSize);
	memcpy(rData, data + 2 +  2*sizeof(uint32_t) + leftSize, rightSize);

	_decoders[0].onEncodedDataReceived(id, lType, lData, leftSize);
	_decoders[1].onEncodedDataReceived(id, rType, rData, rightSize);
}
void MultiH264Decoder::onDecodeFrameSuccess(int id, AVFrame *frame){
	if(_mode == "verticalConcat"){
		verticalConcat(frame);
		return;
	}
	if(id == LEFT){
		lFrame = frame;
		// _observer->onDecodeFrameSuccess(_id, frame, nullptr);

	}else{
		rFrame = frame;
		// _observer->onDecodeFrameSuccess(_id, nullptr, frame);
	}
	if((++_tmpCnt) == 2){
		// AVFrame *tmpFrame = nullptr;

		// if(_mode == "leftResized"){
		// 	tmpFrame = lFrame;
		// }else if(_mode == "rightResized"){
		// 	tmpFrame = rFrame;
		// }

		// if(tmpFrame != nullptr){
		// 	AVFrame* frame2 = av_frame_alloc();
		// 	int num_bytes = avpicture_get_size(PIX_FMT_YUV420P, tmpFrame->width * 2, tmpFrame->height * 2);
		// 	uint8_t* frame2_buffer = (uint8_t *)av_malloc(num_bytes*sizeof(uint8_t));
		// 	avpicture_fill((AVPicture*)frame2, frame2_buffer, PIX_FMT_YUV420P, tmpFrame->width * 2, tmpFrame->height * 2);
		// 	sws_scale(swsctx, tmpFrame->data, tmpFrame->linesize, 0, tmpFrame->height * 2, frame2->data, frame2->linesize);

		// }


		_observer->onDecodeFrameSuccess(_id, lFrame, rFrame);
		_tmpCnt = 0;
	}	

}

void MultiH264Decoder::verticalConcat(AVFrame *frame)
{
	// copy the frame data, it will be used asynchronously and the original
	// memory will already be overwritten by the next decoded frame 

	AVFrame *leftFrame = av_frame_alloc();
	AVFrame *rightFrame = av_frame_alloc();

	leftFrame->linesize[0] = frame->linesize[0];
	leftFrame->linesize[1] = frame->linesize[1];
	leftFrame->linesize[2] = frame->linesize[2];
	leftFrame->linesize[3] = frame->linesize[3];

	rightFrame->linesize[0] = frame->linesize[0];
	rightFrame->linesize[1] = frame->linesize[1];
	rightFrame->linesize[2] = frame->linesize[2];
	rightFrame->linesize[3] = frame->linesize[3];

	leftFrame->width = frame->width;
	leftFrame->height = frame->height / 2;
	leftFrame->format = frame->format;

	rightFrame->width = frame->width;
	rightFrame->height = frame->height / 2;
	rightFrame->format = frame->format;

	av_frame_get_buffer(leftFrame, 64);
	av_frame_get_buffer(rightFrame, 64);



	// TODO Hardcoded i!!!
	for (int i = 0; i < 3; ++i){
		int height = frame->height;
		if(i != 0){
			height /= 2;
		}
		int size = frame->linesize[i]*height;

		int j;
		for (j = 0; j < size; ++j)
		{
			if(j < size / 2){
				leftFrame->data[i][j] = frame->data[i][j];
			}else{
				rightFrame->data[i][j -( size / 2)] = frame->data[i][j];
			}
		}
	}
	_observer->onDecodeFrameSuccess(_id, leftFrame, rightFrame);
}

