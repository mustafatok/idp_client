#include "oculusviewer.h"

#include <iostream>
#include <SDL.h>
#include <assert.h>
#include "../tools/timer.h"
#include "../global.h"



using namespace std;

// Some helpers
unsigned int next_pow2(unsigned int x);

OculusViewer::OculusViewer(int width, int height) : SdlViewer(width, height){
	dummyFrame = av_frame_alloc();

	dummyFrame->width = next_pow2(width);
	dummyFrame->height = next_pow2(height);
	
	dummyFrame->format = PIX_FMT_RGB24;
	av_frame_get_buffer(dummyFrame, 64);

	swslctx = sws_getContext(width, height,
                      PIX_FMT_YUV420P,
                      dummyFrame->width, dummyFrame->height,
                      PIX_FMT_RGB24,
                      SWS_BICUBIC, 0, 0, 0);

	swsrctx = sws_getContext(width, height,
                      PIX_FMT_YUV420P,
                      dummyFrame->width, dummyFrame->height,
                      PIX_FMT_RGB24,
                      SWS_BICUBIC, 0, 0, 0);
}

int OculusViewer::init(){
	int i, x, y;
	unsigned int flags;

	/* libovr must be initialized before we create the OpenGL context */
	ovr_Initialize(0);

	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);

	x = y = SDL_WINDOWPOS_UNDEFINED;
	flags = SDL_WINDOW_OPENGL;
	if(!(window = SDL_CreateWindow("press 'f' to move to the HMD", x, y, width*2, height, flags))) {
		fprintf(stderr, "failed to create window\n");
		return -1;
	}
	if(!(ctx = SDL_GL_CreateContext(window))) {
		fprintf(stderr, "failed to create OpenGL context\n");
		return -1;
	}

	glewInit();

	if(!(hmd = ovrHmd_Create(0))) {
		fprintf(stderr, "failed to open Oculus HMD, falling back to virtual debug HMD\n");
		if(!(hmd = ovrHmd_CreateDebug(ovrHmd_DK2))) {
			fprintf(stderr, "failed to create virtual debug HMD\n");
			return -1;
		}
	}
	printf("initialized HMD: %s - %s\n", hmd->Manufacturer, hmd->ProductName);

	/* resize our window to match the HMD resolution */
	SDL_SetWindowSize(window, hmd->Resolution.w, hmd->Resolution.h);
	SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

	// SDL_SetWindowPosition(window, hmd->WindowsPos.x, hmd->WindowsPos.y);
	// SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
	win_width = hmd->Resolution.w;
	win_height = hmd->Resolution.h;

	/* retrieve the optimal render target resolution for each eye */
	eyeres[0] = ovrHmd_GetFovTextureSize(hmd, ovrEye_Left, hmd->DefaultEyeFov[0], 1.0);
	eyeres[1] = ovrHmd_GetFovTextureSize(hmd, ovrEye_Right, hmd->DefaultEyeFov[1], 1.0);

	/* and create a single render target texture to encompass both eyes */
	fb_width = eyeres[0].w + eyeres[1].w;
	fb_height = eyeres[0].h > eyeres[1].h ? eyeres[0].h : eyeres[1].h;
	update_rtarg(fb_width, fb_height);

	/* fill in the ovrGLTexture structures that describe our render target texture */
	for(i=0; i<2; i++) {
		fb_ovr_tex[i].OGL.Header.API = ovrRenderAPI_OpenGL;
		fb_ovr_tex[i].OGL.Header.TextureSize.w = fb_tex_width;
		fb_ovr_tex[i].OGL.Header.TextureSize.h = fb_tex_height;
		/* this next field is the only one that differs between the two eyes */
		fb_ovr_tex[i].OGL.Header.RenderViewport.Pos.x = i == 0 ? 0 : fb_width / 2.0;
		fb_ovr_tex[i].OGL.Header.RenderViewport.Pos.y = 0;
		fb_ovr_tex[i].OGL.Header.RenderViewport.Size.w = fb_width / 2.0;
		fb_ovr_tex[i].OGL.Header.RenderViewport.Size.h = fb_height;
		fb_ovr_tex[i].OGL.TexId = fb_tex;	/* both eyes will use the same texture id */
	}

	/* fill in the ovrGLConfig structure needed by the SDK to draw our stereo pair
	 * to the actual HMD display (SDK-distortion mode)
	 */
	memset(&glcfg, 0, sizeof glcfg);
	glcfg.OGL.Header.API = ovrRenderAPI_OpenGL;
	glcfg.OGL.Header.BackBufferSize.w = win_width;
	glcfg.OGL.Header.BackBufferSize.h = win_height;
	glcfg.OGL.Header.Multisample = 1;

#ifdef OVR_OS_WIN32
	glcfg.OGL.Window = GetActiveWindow();
	glcfg.OGL.DC = wglGetCurrentDC();
#elif defined(OVR_OS_LINUX)
	glcfg.OGL.Disp = glXGetCurrentDisplay();
#endif

	if(hmd->HmdCaps & ovrHmdCap_ExtendDesktop) {
		ovrHmd_AttachToWindow(hmd, (void*)glXGetCurrentDrawable(), 0, 0);
		printf("running in \"extended desktop\" mode\n");
	} else {
		/* to sucessfully draw to the HMD display in "direct-hmd" mode, we have to
		 * call ovrHmd_AttachToWindow
		 * XXX: this doesn't work properly yet due to bugs in the oculus 0.4.1 sdk/driver
		 */
#ifdef WIN32
		ovrHmd_AttachToWindow(hmd, glcfg.OGL.Window, 0, 0);
#elif defined(OVR_OS_LINUX)
		ovrHmd_AttachToWindow(hmd, (void*)glXGetCurrentDrawable(), 0, 0);
#endif
		printf("running in \"direct-hmd\" mode\n");
	}

	/* enable low-persistence display and dynamic prediction for lattency compensation */
	hmd_caps = ovrHmdCap_LowPersistence | ovrHmdCap_DynamicPrediction;
	ovrHmd_SetEnabledCaps(hmd, hmd_caps);

	/* configure SDK-rendering and enable OLED overdrive and timewrap, which
	 * shifts the image before drawing to counter any lattency between the call
	 * to ovrHmd_GetEyePose and ovrHmd_EndFrame.
	 */
	distort_caps = ovrDistortionCap_TimeWarp | ovrDistortionCap_Overdrive;
	if(!ovrHmd_ConfigureRendering(hmd, &glcfg.Config, distort_caps, hmd->DefaultEyeFov, eye_rdesc)) {
		fprintf(stderr, "failed to configure distortion renderer\n");
	}

	if (!ovrHmd_ConfigureTracking(hmd, ovrTrackingCap_Orientation | ovrTrackingCap_Position, 0)) {
		fprintf(stderr, "failed to configure position tracker\n");
    }
	return 0;

}


bool OculusViewer::show(bool fullscreen)
{
	if(this->init() == -1) {
		return false;
	}
	Timer t, tf;
	t.remember();
	int64_t frameCounter = 0;
		
	while (!stopped) {
		SDL_Event event;
		if (SDL_PollEvent(&event) > 0) {
			if((event.type == SDL_QUIT) || 
					((event.type == SDL_KEYUP) && (event.key.keysym.sym == SDLK_ESCAPE))){
				stopped = true;
			}else if((event.type == SDL_KEYUP) && (event.key.keysym.sym == 'f')){
				this->toggle_hmd_fullscreen();
			}
					
		} else {
			ovrHmd_DismissHSWDisplay(hmd);

			frameLock.lock();
			AVFrame *lf = currentLFrame;
			AVFrame *rf = currentRFrame;
			currentLFrame = nullptr;
			currentRFrame = nullptr;
			frameLock.unlock();
			if (lf != nullptr && rf != nullptr) { 
				renderFrame(lf, rf);
				frameCounter++;
				av_frame_free(&lf);
				av_frame_free(&rf);
			} else{
					// don't use up 100% of cpu if there is no event in the queue
				SDL_Delay(1);
			}
		}
		t.remember();
	}
	return true;
}

OculusViewer::~OculusViewer()
{
	if(hmd){
		ovrHmd_Destroy(hmd);
	}
	ovr_Shutdown();
	if (window != nullptr) { SDL_DestroyWindow(window); }

}


void OculusViewer::renderFrame(AVFrame *lFrame, AVFrame *rFrame)
{
	int i;
	ovrMatrix4f proj;
	ovrPosef pose[2];

	/* the drawing starts with a call to ovrHmd_BeginFrame */
	ovrHmd_BeginFrame(hmd, 0);

	/* start drawing onto our texture render target */
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
	/* for each eye ... */
	for(i=0; i<2; i++) {
		ovrEyeType eye = hmd->EyeRenderOrder[i];

		glViewport(eye == ovrEye_Left ? 0 : fb_width / 2, 0, fb_width / 2, fb_height);

		// proj = ovrMatrix4f_Projection(hmd->DefaultEyeFov[eye], 0.0, 0.0, eye);
		proj = ovrMatrix4f_Projection(hmd->DefaultEyeFov[eye], 0.5, 500.0, 0);
		glMatrixMode(GL_PROJECTION);
		glLoadTransposeMatrixf(proj.M[0]);

		pose[eye] = ovrHmd_GetHmdPosePerEye(hmd, eye);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glTranslatef(eye_rdesc[eye].HmdToEyeViewOffset.x,
				eye_rdesc[eye].HmdToEyeViewOffset.y,
				eye_rdesc[eye].HmdToEyeViewOffset.z);

		// glTranslatef(0, -ovrHmd_GetFloat(hmd, OVR_KEY_EYE_HEIGHT, 1.65), 0);

		glRotatef(180,0,0,0);
		/* finally draw the scene for this eye */

		if (eye == ovrEye_Left){
			draw_scene(lFrame, true); 
		}else{
			draw_scene(rFrame, false); 
		}
		
		// Query the HMD for the current tracking state.
		ovrTrackingState ts  = ovrHmd_GetTrackingState(hmd, ovr_GetTimeInSeconds());

		if (ts.StatusFlags & (ovrStatus_OrientationTracked | ovrStatus_PositionTracked)) 
		{
			ovrPosef pose = ts.HeadPose.ThePose;
			std::cout << pose.Position.x << " " << pose.Position.y << " " << pose.Position.z <<std::endl;
		}


	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	ovrHmd_EndFrame(hmd, pose, &fb_ovr_tex[0].Texture);
	glUseProgram(0);
	assert(glGetError() == GL_NO_ERROR);
}

void OculusViewer::toggle_hmd_fullscreen()
{
	
	SDL_SetWindowPosition(window, hmd->WindowsPos.x, hmd->WindowsPos.y);
	SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);

	glcfg.OGL.Header.BackBufferSize.w = hmd->Resolution.h;
	glcfg.OGL.Header.BackBufferSize.h = hmd->Resolution.w;

	distort_caps &= ~ovrDistortionCap_LinuxDevFullscreen;
	ovrHmd_ConfigureRendering(hmd, &glcfg.Config, distort_caps, hmd->DefaultEyeFov, eye_rdesc);

}

void OculusViewer::draw_scene(AVFrame *frame, bool leftSign)
{
	unsigned int tex;

	if(leftSign)
		sws_scale(swslctx, frame->data, frame->linesize, 0, frame->height, dummyFrame->data, dummyFrame->linesize);
	else
		sws_scale(swsrctx, frame->data, frame->linesize, 0, frame->height, dummyFrame->data, dummyFrame->linesize);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
	glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, dummyFrame->width, dummyFrame->height, 0, GL_RGB, GL_UNSIGNED_BYTE, dummyFrame->data[0]);
	
	glEnable(GL_TEXTURE_2D);
	draw_box(30, 20, 35, -1.0);
	glDisable(GL_TEXTURE_2D);
	glDeleteTextures(1, &tex);

	glPopMatrix();

	// TODO Dummy frame i free yaparak dene
}

void OculusViewer::draw_box(float xsz, float ysz, float zsz, float norm_sign)
{
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glScalef(xsz * 0.5, ysz * 0.5, zsz * 0.5);

	if(norm_sign < 0.0) {
		glFrontFace(GL_CW);
	}

	glBegin(GL_QUADS);
	glNormal3f(0, 0, 1 * norm_sign);
	glTexCoord2f(0, 0); glVertex3f(-1, -1, 1);
	glTexCoord2f(1, 0); glVertex3f(1, -1, 1);
	glTexCoord2f(1, 1); glVertex3f(1, 1, 1);
	glTexCoord2f(0, 1); glVertex3f(-1, 1, 1);
	glNormal3f(1 * norm_sign, 0, 0);
	glTexCoord2f(0, 0); glVertex3f(1, -1, 1);
	glTexCoord2f(1, 0); glVertex3f(1, -1, -1);
	glTexCoord2f(1, 1); glVertex3f(1, 1, -1);
	glTexCoord2f(0, 1); glVertex3f(1, 1, 1);
	glNormal3f(0, 0, -1 * norm_sign);
	glTexCoord2f(0, 0); glVertex3f(1, -1, -1);
	glTexCoord2f(1, 0); glVertex3f(-1, -1, -1);
	glTexCoord2f(1, 1); glVertex3f(-1, 1, -1);
	glTexCoord2f(0, 1); glVertex3f(1, 1, -1);
	glNormal3f(-1 * norm_sign, 0, 0);
	glTexCoord2f(0, 0); glVertex3f(-1, -1, -1);
	glTexCoord2f(1, 0); glVertex3f(-1, -1, 1);
	glTexCoord2f(1, 1); glVertex3f(-1, 1, 1);
	glTexCoord2f(0, 1); glVertex3f(-1, 1, -1);
	glEnd();

	// glBegin(GL_TRIANGLE_FAN);
	// glNormal3f(0, 1 * norm_sign, 0);
	// glTexCoord2f(0.5, 0.5); glVertex3f(0, 1, 0);
	// glTexCoord2f(0, 0); glVertex3f(-1, 1, 1);
	// glTexCoord2f(1, 0); glVertex3f(1, 1, 1);
	// glTexCoord2f(1, 1); glVertex3f(1, 1, -1);
	// glTexCoord2f(0, 1); glVertex3f(-1, 1, -1);
	// glTexCoord2f(0, 0); glVertex3f(-1, 1, 1);
	// glEnd();
	// glBegin(GL_TRIANGLE_FAN);
	// glNormal3f(0, -1 * norm_sign, 0);
	// glTexCoord2f(0.5, 0.5); glVertex3f(0, -1, 0);
	// glTexCoord2f(0, 0); glVertex3f(-1, -1, -1);
	// glTexCoord2f(1, 0); glVertex3f(1, -1, -1);
	// glTexCoord2f(1, 1); glVertex3f(1, -1, 1);
	// glTexCoord2f(0, 1); glVertex3f(-1, -1, 1);
	// glTexCoord2f(0, 0); glVertex3f(-1, -1, -1);
	// glEnd();

	glFrontFace(GL_CCW);
	glPopMatrix();
}

/* update_rtarg creates (and/or resizes) the render target used to draw the two stero views */
void OculusViewer::update_rtarg(int width, int height)
{
	if(!fbo) {
		/* if fbo does not exist, then nothing does... create every opengl object */
		glGenFramebuffers(1, &fbo);
		glGenTextures(1, &fb_tex);
		glGenRenderbuffers(1, &fb_depth);

		glBindTexture(GL_TEXTURE_2D, fb_tex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	/* calculate the next power of two in both dimensions and use that as a texture size */
	fb_tex_width = next_pow2(width);
	fb_tex_height = next_pow2(height);

	/* create and attach the texture that will be used as a color buffer */
	glBindTexture(GL_TEXTURE_2D, fb_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, fb_tex_width, fb_tex_height, 0,
			GL_RGBA, GL_UNSIGNED_BYTE, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fb_tex, 0);

	/* create and attach the renderbuffer that will serve as our z-buffer */
	glBindRenderbuffer(GL_RENDERBUFFER, fb_depth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, fb_tex_width, fb_tex_height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, fb_depth);

	if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		fprintf(stderr, "incomplete framebuffer!\n");
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	printf("created render target: %dx%d (texture size: %dx%d)\n", width, height, fb_tex_width, fb_tex_height);
}


unsigned int next_pow2(unsigned int x)
{
	x -= 1;
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	return x + 1;
}

