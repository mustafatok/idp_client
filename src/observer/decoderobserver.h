#ifndef __DECODEROBSERVER_H
#define __DECODEROBSERVER_H

#include <stdint.h>
extern "C" {
#include <libavutil/frame.h>
}
class DecoderObserver {
public:
	virtual void onDecodeFrameSuccess(int id, AVFrame *frame) = 0;
	virtual void onDecodeFrameSuccess(int id, AVFrame *lFrame, AVFrame *rFrame) = 0;
};

#endif // __DECODEROBSERVER_H
