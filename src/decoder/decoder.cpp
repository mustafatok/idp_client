#include "decoder.h"

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

#if LIBAVUTIL_VERSION_INT < AV_VERSION_INT(53, 5, 0)
int av_frame_copy(AVFrame *dst, const AVFrame *src);
#endif


H264Decoder::H264Decoder()
{
        avcodec_register_all();
}

H264Decoder::~H264Decoder()
{
        if (codecContext != nullptr) {
                avcodec_close(codecContext);
                codecContext = nullptr;
        }
}

void H264Decoder::decode(uint8_t type, uint8_t* data, int size)
{
        if (size == 0 || !IS_NALU(type)) {
                return;
        }

        if (type == PROTOCOL_TYPE_HEADER) {
                decodeHeader(data, size);
        } else if (type == PROTOCOL_TYPE_FRAME) {
                decodeFrame(data, size);
        }
}

void H264Decoder::decodeHeader(uint8_t* data, int size)
{
        picture = av_frame_alloc();
        if (!picture) {
                cerr << "Could not allocate target picture." << endl;
                return;
        }

        codecContext = avcodec_alloc_context3(codec);

        codec = avcodec_find_decoder(AV_CODEC_ID_H264);
        if (!codec) {
                cerr << "Could not find codec." << endl;
                return;
        }

        if (avcodec_open2(codecContext, codec, nullptr) < 0) {
                cerr << "Could not open codec." << endl;
                return;
        }

        av_init_packet(&packet);

        // don't send header information atm, as currently we include this information in the I-Frames
        /*int gotPicture = 0;
        packet.data = data;
        packet.size = size;
        int res = avcodec_decode_video2(codecContext, picture, &gotPicture, &packet);
        if (res < 0) {
                cerr << "Could not decode header information!" << endl;
        }*/
}

void H264Decoder::decodeFrame(uint8_t* data, int size)
{
        if (codec == nullptr || codecContext == nullptr) {
                cerr << "Decoder not initialized!" << endl;
                return;
        }

        // decode frame
        int gotPicture = 0;

        packet.data = data;
        packet.size = size;
        int res = avcodec_decode_video2(codecContext, picture, &gotPicture, &packet);
        if (res < 0) {
                cerr << "Could not decode frame data!" << endl;
                return;
        }

        // free memory (this is not handled by the UdpSocket class!)
        delete[] data;

        if (gotPicture) {
                // copy the frame data, it will be used asynchronously and the original
                // memory will already be overwritten by the next decoded frame 

                AVFrame *newFrame = av_frame_alloc();

                newFrame->linesize[0] = picture->linesize[0];
                newFrame->linesize[1] = picture->linesize[1];
                newFrame->linesize[2] = picture->linesize[2];
                newFrame->linesize[3] = picture->linesize[3];

                newFrame->width = picture->width;
                newFrame->height = picture->height;

                newFrame->format = picture->format;

                av_frame_get_buffer(newFrame, 64);
                av_frame_copy(newFrame, picture);

                frameCallback(newFrame);
        }
}

#if LIBAVUTIL_VERSION_INT < AV_VERSION_INT(53, 5, 0)
static int frame_copy_video(AVFrame *dst, const AVFrame *src)
{
        const uint8_t *src_data[4];
        int i, planes;

        if (dst->width  != src->width ||
                dst->height != src->height)
                return AVERROR(EINVAL);

        planes = av_pix_fmt_count_planes(static_cast<AVPixelFormat>(dst->format));
        for (i = 0; i < planes; i++)
                if (!dst->data[i] || !src->data[i])
                        return AVERROR(EINVAL);

        memcpy(src_data, src->data, sizeof(src_data));
        av_image_copy(dst->data, dst->linesize,
                        src_data, src->linesize,
                        static_cast<AVPixelFormat>(dst->format), dst->width, dst->height);

        return 0;
}

static int frame_copy_audio(AVFrame *dst, const AVFrame *src)
{
        int planar   = av_sample_fmt_is_planar(static_cast<AVSampleFormat>(dst->format));
        int channels = av_get_channel_layout_nb_channels(dst->channel_layout);
        int planes   = planar ? channels : 1;
        int i;

        if (dst->nb_samples     != src->nb_samples ||
                dst->channel_layout != src->channel_layout)
                return AVERROR(EINVAL);

        for (i = 0; i < planes; i++)
                if (!dst->extended_data[i] || !src->extended_data[i])
                        return AVERROR(EINVAL);

        av_samples_copy(dst->extended_data, src->extended_data, 0, 0,
                dst->nb_samples, channels, static_cast<AVSampleFormat>(dst->format));

        return 0;
}

int av_frame_copy(AVFrame *dst, const AVFrame *src)
{
        if (dst->format != src->format || dst->format < 0)
                return AVERROR(EINVAL);

        if (dst->width > 0 && dst->height > 0)
                return frame_copy_video(dst, src);
        else if (dst->nb_samples > 0 && dst->channel_layout)
                return frame_copy_audio(dst, src);

        return AVERROR(EINVAL);
}
#endif
