#ifndef __DECODER_H
#define __DECODER_H

#include <functional>
#include <queue>

extern "C" {
#include <x264.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/frame.h>
}

using FrameFunction = std::function<void(AVFrame*)>;

class H264Decoder {
public:
        explicit H264Decoder();
        virtual ~H264Decoder();

        void decode(uint8_t type, uint8_t* data, int size);

        template <typename ObjectType>
        void setFrameCallback(ObjectType *instance, 
                void (ObjectType::*callback)(AVFrame*))
        {
                frameCallback = [=](AVFrame* f) {
                        (instance->*callback)(f);
                };
        }

private:
        void decodeHeader(uint8_t *data, int size);
        void decodeFrame(uint8_t *data, int size);

        FrameFunction frameCallback;

        AVCodec *codec = nullptr;
        AVCodecContext *codecContext = nullptr;
        AVFrame *picture = nullptr;
        AVPacket packet;
};

#endif // __DECODER_H