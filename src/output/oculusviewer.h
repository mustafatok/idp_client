#ifndef __OCULUSVIEWER_H
#define __OCULUSVIEWER_H

#include "sdlviewer.h"

#include <GL/glew.h>

#include <X11/Xlib.h>
#include <GL/glx.h>

#include <OVR_CAPI.h>
#include <OVR_CAPI_GL.h>
extern "C" {
#include <libswscale/swscale.h>
}
class OculusViewer : public SdlViewer {
public:
	explicit OculusViewer(int width, int height);
	virtual ~OculusViewer();
	virtual bool show(bool fullscreen = false);
	virtual void updateSize(int lWidth, int lHeight, int rWidth, int rHeight){
		// SdlViewer::updateSize(lWidth, lHeight, rWidth, rHeight);
		if(swslctx != nullptr) sws_freeContext(swslctx);
		if(swsrctx != nullptr) sws_freeContext(swsrctx);
		swslctx = sws_getContext(lWidth, lHeight,
                  PIX_FMT_YUV420P,
                  dummyFrame->width, dummyFrame->height,
                  PIX_FMT_RGB24,
                  0, 0, 0, 0);
        swsrctx = sws_getContext(rWidth, rHeight,
                  PIX_FMT_YUV420P,
                  dummyFrame->width, dummyFrame->height,
                  PIX_FMT_RGB24,
                  0, 0, 0, 0);
	}


private:
	SDL_GLContext ctx;
	int win_width, win_height;

	unsigned int fbo, fb_tex, fb_depth;
	int fb_width, fb_height;
	int fb_tex_width, fb_tex_height;

	ovrHmd hmd;
	ovrSizei eyeres[2];
	ovrEyeRenderDesc eye_rdesc[2];
	ovrGLTexture fb_ovr_tex[2];
	union ovrGLConfig glcfg;
	unsigned int distort_caps;
	unsigned int hmd_caps;

	AVFrame *dummyFrame;
	SwsContext * swslctx = nullptr;
	SwsContext * swsrctx = nullptr;

	int init();
	void draw_scene(AVFrame *frame, bool leftSign);
	void draw_box(float xsz, float ysz, float zsz, float norm_sign);
	void update_rtarg(int width, int height);
	void toggle_hmd_fullscreen();

protected:
	virtual void renderFrame(AVFrame *lFrame, AVFrame *rFrame);
};

#endif // __OCULUSVIEWER_H

