 #include "sdlviewer.h"

/*
 * Code based on https://github.com/Twinklebear/TwinklebearDev-Lessons/blob/master/Lesson1/src/main.cpp
 * which was written by Will Usher
 */

#include <iostream>
#include <SDL.h>
#include "../tools/timer.h"
#include "../global.h"

using namespace std;

#define WINDOW_TITLE    "LMT Video Player"

int isMouseEvent(void *, SDL_Event* event)
{
        if (event->type == SDL_USEREVENT 
                || event->type == SDL_QUIT 
                || event->type == SDL_KEYUP) {
                return 1;
        }
        return 0;
}

SdlViewer::SdlViewer(int width, int height)
{
	this->height = height;
	this->width = width;
}

SdlViewer::~SdlViewer()
{
        if (frameTexture != nullptr) { SDL_DestroyTexture(frameTexture); }
        if (texture != nullptr) { SDL_DestroyTexture(texture); }
        if (renderer != nullptr) { SDL_DestroyRenderer(renderer); }
        if (window != nullptr) { SDL_DestroyWindow(window); }
        SDL_Quit();
}

bool SdlViewer::show(bool fullscreen)
{
        if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
                cerr << "SDL_Init Error: " << SDL_GetError() << endl;
                return false;
        }

        SDL_SetEventFilter(&isMouseEvent, nullptr);

        if (!fullscreen) {
                window = SDL_CreateWindow(WINDOW_TITLE, 100, 100, width, height, SDL_WINDOW_SHOWN|SDL_WINDOW_OPENGL);
        } else {
                window = SDL_CreateWindow(WINDOW_TITLE, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_FULLSCREEN|SDL_WINDOW_SHOWN|SDL_WINDOW_OPENGL);
        }
        if (window == nullptr) {
                cerr << "SDL_CreateWindow Error: " << SDL_GetError() << endl;
                return false; 
        }

        // -1 means it automatically searches the gpu
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        if (renderer == nullptr) {
                cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << endl;
                return false;
        }

        bitmap = SDL_LoadBMP("resources/wait.bmp");
        if (bitmap == nullptr) {
                cerr << "SDL_LoadBMP Error: " << SDL_GetError() << endl;
                return false;
        }

        texture = SDL_CreateTextureFromSurface(renderer, bitmap);
        if (texture == nullptr) {
                cerr << "SDL_CreateTextureFromSurface Error: " << SDL_GetError() << endl;
                return false;
        }
        SDL_FreeSurface(bitmap);

        frameTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_YV12, SDL_TEXTUREACCESS_STATIC, width, height);
        if (frameTexture == nullptr) {
                cerr << "SDL_CreateTexture Error: " << SDL_GetError() << endl;
                return false;
        }

        SDL_Rect pos;
        pos.x = (width - bitmap->w) / 2;
        pos.y = (height - bitmap->h) / 2;
        pos.w = bitmap->w;
        pos.h = bitmap->h;

        SDL_RenderClear(renderer); // replace everything with the drawing color (default: #000000);
        SDL_RenderCopy(renderer, texture, nullptr, &pos); // copy picture onto texture
        SDL_RenderPresent(renderer); // show window*/

	Timer t, tf;
	t.remember();
	int64_t frameCounter = 0;
        
        while (!stopped) {
                SDL_Event event;

                if ((SDL_PollEvent(&event) > 0) && 
                        ((event.type == SDL_QUIT) || 
                                ((event.type == SDL_KEYUP) && (event.key.keysym.sym == SDLK_ESCAPE)))) {
                        stopped = true;
                } else {

                        frameLock.lock();
                        AVFrame *f = currentFrame;
                        currentFrame = nullptr;
                        frameLock.unlock();
                        if (f != nullptr) { 
                                renderFrame(f);
				frameCounter++;
                                av_frame_free(&f);
                        } else{
                                // don't use up 100% of cpu if there is no event in the queue
                                SDL_Delay(1);
                        }
                }
		t.remember();
                SDL_RenderCopy(renderer, frameTexture, nullptr, nullptr);
                SDL_RenderPresent(renderer);
        }

	int64_t decoding = t.diff_ys();
	cout << "frameCount: " << frameCounter << ", time till exit: " << decoding << endl;

        return true;
}

void SdlViewer::showFrame(AVFrame *frame)
{

        frameLock.lock();
        if (currentFrame != nullptr) {
                av_frame_free(&currentFrame);
        }
        currentFrame = frame;
        frameLock.unlock();
}

void SdlViewer::renderFrame(AVFrame *frame)
{
		//if (SDL_UpdateTexture(frameTexture, nullptr, &frame->data, width * SDL_BYTESPERPIXEL(SDL_PIXELFORMAT_YV12)) == -1) {
        if (SDL_UpdateYUVTexture(frameTexture, nullptr, frame->data[0], frame->linesize[0], frame->data[1],
                                frame->linesize[1], frame->data[2], frame->linesize[2]) == -1) {
                cerr << "SDL_UpdateYUVTexture Error: " << SDL_GetError() << endl;
                return;
        }
}
