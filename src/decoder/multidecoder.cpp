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

using namespace std;

#if LIBAVUTIL_VERSION_INT < AV_VERSION_INT(52, 66, 100)
int av_frame_copy(AVFrame *dst, const AVFrame *src);
#endif


void MultiH264Decoder::postProcessFrame()
{
	// copy the frame data, it will be used asynchronously and the original
	// memory will already be overwritten by the next decoded frame 

	AVFrame *leftFrame = av_frame_alloc();
	AVFrame *rightFrame = av_frame_alloc();

	leftFrame->linesize[0] = picture->linesize[0];
	leftFrame->linesize[1] = picture->linesize[1];
	leftFrame->linesize[2] = picture->linesize[2];
	leftFrame->linesize[3] = picture->linesize[3];

	rightFrame->linesize[0] = picture->linesize[0];
	rightFrame->linesize[1] = picture->linesize[1];
	rightFrame->linesize[2] = picture->linesize[2];
	rightFrame->linesize[3] = picture->linesize[3];

	leftFrame->width = picture->width;
	leftFrame->height = picture->height / 2;
	leftFrame->format = picture->format;

	rightFrame->width = picture->width;
	rightFrame->height = picture->height / 2;
	rightFrame->format = picture->format;

	av_frame_get_buffer(leftFrame, 64);
	av_frame_get_buffer(rightFrame, 64);



	// TODO Hardcoded i!!!
	for (int i = 0; i < 3; ++i){
		int height = picture->height;
		if(i != 0){
			height /= 2;
		}
		int size = picture->linesize[i]*height;

		int j;
		for (j = 0; j < size; ++j)
		{
			if(j < size / 2){
				leftFrame->data[i][j] = picture->data[i][j];
			}else{
				rightFrame->data[i][j -( size / 2)] = picture->data[i][j];
			}
		}
	}
	frameCallback(leftFrame, rightFrame);
}