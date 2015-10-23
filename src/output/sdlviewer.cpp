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

#define WINDOW_TITLE    "Stereo Video Player"

int isMouseEvent(void *, SDL_Event* event)
{
	if (event->type == SDL_USEREVENT || 
		event->type == SDL_QUIT || 
		event->type == SDL_KEYUP) {
			return 1;
	}
	return 0;
}

SdlViewer::SdlViewer(int width, int height)
{
	this->height = height;
	this->width = width;
	this->_lHeight = this->_rHeight = height;
	this->_lWidth = this->_rWidth = width;
}

SdlViewer::~SdlViewer()
{
	if (lFrameTexture != nullptr) { SDL_DestroyTexture(lFrameTexture); }
	if (rFrameTexture != nullptr) { SDL_DestroyTexture(rFrameTexture); }
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
		window = SDL_CreateWindow(WINDOW_TITLE, 100, 100, width * 2, height, SDL_WINDOW_SHOWN|SDL_WINDOW_OPENGL);
	} else {
		window = SDL_CreateWindow(WINDOW_TITLE, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width * 2, height, SDL_WINDOW_FULLSCREEN|SDL_WINDOW_SHOWN|SDL_WINDOW_OPENGL);
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

	lFrameTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_YV12, SDL_TEXTUREACCESS_STATIC, _lWidth, _lHeight);
	if (lFrameTexture == nullptr) {
		cerr << "SDL_CreateTexture Error: " << SDL_GetError() << endl;
		return false;
	}
	rFrameTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_YV12, SDL_TEXTUREACCESS_STATIC, _rWidth, _rHeight);
	if (rFrameTexture == nullptr) {
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

	SDL_Rect leftRect;
	SDL_Rect rightRect;
	leftRect.x = leftRect.y = 0;
	leftRect.w = rightRect.w = width;
	leftRect.h = rightRect.h = height;
	rightRect.x = width;
	rightRect.y = 0;
	Timer t, tf;
	t.remember();
	int64_t frameCounter = 0;
	
	while (!stopped) {
		SDL_Event event;

		if ((SDL_PollEvent(&event) > 0)){

			if((event.type == SDL_QUIT) || 
			 ((event.type == SDL_KEYUP) && 
			 (event.key.keysym.sym == SDLK_ESCAPE))) {

				stopped = true;
			}else if(event.key.keysym.sym == SDLK_LEFT){
				std::cout << "LEFTPRESSED" << std::endl;
				inputPositionsCallback(0, 0);
			}
		} else {

			frameLock.lock();
			AVFrame *lf = currentLFrame;
			AVFrame *rf = currentRFrame;
			currentLFrame = nullptr;
			currentRFrame = nullptr;
			frameLock.unlock();
			
			if(lf == nullptr && rf == nullptr){
				SDL_Delay(1);
			}else{
				renderFrame(lf, rf);
				frameCounter++;
			}
			if(lf != nullptr) av_frame_free(&lf);
			if(rf != nullptr) av_frame_free(&rf);
		}
		t.remember();
		// TODO change it for single input
		SDL_RenderCopy(renderer, lFrameTexture, nullptr, &leftRect);
		SDL_RenderCopy(renderer, rFrameTexture, nullptr, &rightRect);
		SDL_RenderPresent(renderer);
	}

	int64_t decoding = t.diff_ys();
	cout << "frameCount: " << frameCounter << ", time till exit: " << decoding << endl;

	return true;
}

void SdlViewer::showFrame(AVFrame *lFrame, AVFrame *rFrame)
{
	frameLock.lock();
	if (currentLFrame != nullptr) {
		av_frame_free(&currentLFrame);
	}
	if (currentRFrame != nullptr) {
		av_frame_free(&currentRFrame);
	}
	currentLFrame = lFrame;
	currentRFrame = rFrame;
	frameLock.unlock();
}

void SdlViewer::renderFrame(AVFrame *lFrame, AVFrame *rFrame)
{
	if(lFrame != nullptr){
		if (SDL_UpdateYUVTexture(lFrameTexture, nullptr, lFrame->data[0], lFrame->linesize[0], lFrame->data[1],
								lFrame->linesize[1], lFrame->data[2], lFrame->linesize[2]) == -1) {
			cerr << "SDL_UpdateYUVTexture Error: " << SDL_GetError() << endl;
			return;
		}
	}
	if(rFrame != nullptr){
		if (SDL_UpdateYUVTexture(rFrameTexture, nullptr, rFrame->data[0], rFrame->linesize[0], rFrame->data[1],
								rFrame->linesize[1], rFrame->data[2], rFrame->linesize[2]) == -1) {
			cerr << "SDL_UpdateYUVTexture Error: " << SDL_GetError() << endl;
			return;
		}
	}

}

void SdlViewer::updateSize(int lWidth, int lHeight, int rWidth, int rHeight){
	_lWidth = lWidth;
	_lHeight = lHeight;
	_rWidth = rWidth;
	_rHeight = rHeight;

	if (lFrameTexture != nullptr) { SDL_DestroyTexture(lFrameTexture); }
	if (rFrameTexture != nullptr) { SDL_DestroyTexture(rFrameTexture); }
	
	lFrameTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_YV12, SDL_TEXTUREACCESS_STATIC, _lWidth, _lHeight);
	if (lFrameTexture == nullptr) {
		cerr << "LSDL_CreateTexture Error: " << SDL_GetError() << endl;
		return;
	}

	rFrameTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_YV12, SDL_TEXTUREACCESS_STATIC, _rWidth, _rHeight);
	if (rFrameTexture == nullptr) {
		cerr << "RSDL_CreateTexture Error: " << SDL_GetError() << endl;
		return;
	}

	SDL_Rect leftRect;
	SDL_Rect rightRect;
	leftRect.x = leftRect.y = 0;
	leftRect.w = rightRect.w = width;
	leftRect.h = rightRect.h = height;
	rightRect.x = width;
	rightRect.y = 0;
	SDL_RenderCopy(renderer, lFrameTexture, nullptr, &leftRect);
	SDL_RenderCopy(renderer, rFrameTexture, nullptr, &rightRect);


}