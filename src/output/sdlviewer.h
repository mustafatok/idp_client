#ifndef __SDLVIEWER_H
#define __SDLVIEWER_H

#include <SDL.h>
#include <mutex>
#include "../observer/decoderobserver.h"
extern "C" {
#include <libavutil/frame.h>
}

class SdlViewer : public DecoderObserver{
public:
	explicit SdlViewer(int width, int height);
	virtual ~SdlViewer();

	virtual void showFrame(AVFrame *lFrame, AVFrame *rFrame = nullptr);

	virtual bool show(bool fullscreen = false);

	// Observer Functions
	virtual void onDecodeFrameSuccess(int id, AVFrame *frame){
		showFrame(frame, nullptr);
	}
	virtual void onDecodeFrameSuccess(int id, AVFrame *lFrame, AVFrame *rFrame) {
		showFrame(lFrame, rFrame);
	}

protected:

	SDL_Window *window       = nullptr;
	SDL_Renderer *renderer   = nullptr;
	SDL_Surface *bitmap      = nullptr;
	SDL_Texture *texture     = nullptr;
	SDL_Texture *lFrameTexture = nullptr;
	SDL_Texture *rFrameTexture = nullptr;

	bool stopped = false;

	std::mutex frameLock;
	AVFrame *currentLFrame = nullptr;
	AVFrame *currentRFrame = nullptr;

	virtual void renderFrame(AVFrame *lFrame, AVFrame *rFrame);
	int width;
	int height;

};

#endif // __SDLVIEWER_H
