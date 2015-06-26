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
	_decoders[0].onEncodedDataReceived(id, type, data, size);
	// if(_mode == "mergedOutput"){
		// this->mergedOutput(uint8_t type, uint8_t* data, int size);
	// }else{

	// }
}
void MultiH264Decoder::onDecodeFrameSuccess(int id, AVFrame *frame){
	mergedOutput(frame);
}

void MultiH264Decoder::mergedOutput(AVFrame *frame)
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

