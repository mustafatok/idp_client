#ifndef __SDLVIEWER_H
#define __SDLVIEWER_H

#include <SDL.h>
#include <mutex>

extern "C" {
#include <libavutil/frame.h>
}

class SdlViewer {
public:
        explicit SdlViewer(int width, int height);
        virtual ~SdlViewer();

        void showFrame(AVFrame *frame);

        bool show(bool fullscreen = false);

private:
        void renderFrame(AVFrame *frame);

        SDL_Window *window       = nullptr;
        SDL_Renderer *renderer   = nullptr;
        SDL_Surface *bitmap      = nullptr;
        SDL_Texture *texture     = nullptr;
        SDL_Texture *frameTexture = nullptr;

        bool stopped = false;

        std::mutex frameLock;
        AVFrame *currentFrame = nullptr;
		int width;
		int height;
};

#endif // __SDLVIEWER_H
