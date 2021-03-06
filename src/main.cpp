#include <fstream>
#include <iostream>
#include <thread>

#include "decoder/decoder.h"
#include "decoder/multidecoder.h"
#include "input/network.h"
#include "output/sdlviewer.h"
#include "output/oculusviewer.h"
#include "tools/configuration.h"
#include "tools/timer.h"

#include "global.h"

using namespace std;

ConfigFile cfg("build/configuration.cfg");
//common setup
int DEFAULT_WIDTH = cfg.getValueOfKey<int>("WIDTH");
int DEFAULT_HEIGHT = cfg.getValueOfKey<int>("HEIGHT");
double DEFAULT_FPS = cfg.getValueOfKey<double>("FPS");
double EXPOSURE = cfg.getValueOfKey<double>("EXPOSURE");
bool FULLSCREEN = cfg.getValueOfKey<bool>("FULLSCREEN");
const uint16_t TARGET_PORT = 2525;


UdpSocket input;
// MultiH264Decoder *decoder;
MultiH264Decoder *decoder;

SdlViewer *viewer = nullptr;
bool fullscreen = false;
bool stopped = false;
float _x = 100;  float _y =  100 ;  float _z  = 100;

void inputCallback(int x, int y){
	input.sendInputParams(x, y);
}

void positionCallback( float x,  float y,  float z){
	if(!(_x == 100 || _y == 100 || _z == 100)){
		input.sendPositionParams(_x - x, _y - y, _z - z);
	}
	_x = x;
	_y = y;
	_z = z;
}


void init(int mode, int lWidth, int lHeigth, int rWidth, int rHeight){
	if(decoder != nullptr) decoder->releaseObserver();

	cout << "LW : " << lWidth << endl;
	cout << "LH : " << lHeigth << endl;
	cout << "RW : " << rWidth << endl;
	cout << "RH : " << rHeight << endl;
	if(mode == (int) MODE_VERTICALCONCAT){
		cout << "MODE_VERTICALCONCAT" << endl;
	}else if(mode == (int) MODE_LEFTRESIZED){
		cout << "MODE_LEFTRESIZED" << endl;
	}else if(mode == (int) MODE_RIGHTRESIZED){
		cout << "MODE_RIGHTRESIZED" << endl;
	}else if(mode == (int) MODE_LEFTBLURRED){
		cout << "MODE_LEFTBLURRED" << endl;
	}else if(mode == (int) MODE_RIGHTBLURRED){
		cout << "MODE_RIGHTBLURRED" << endl;
	}else if(mode == (int) MODE_SINGLE){
		cout << "MODE_SINGLE" << endl;
	}else if(mode == (int) MODE_INTERLEAVING){
		cout << "MODE_INTERLEAVING" << endl;
	}else{
		return;
	}
	cout << "Hoop1" << endl;
	cout << "Vw" << viewer << endl;
	viewer->updateSize(lWidth, lHeigth, rWidth, rHeight);
	cout << "Hoop2" << endl;

	// TODO: DELETE OLD DECODER
	decoder = new MultiH264Decoder(mode);
	cout << "Hoop3" << endl;

	decoder->setDecoderObserver(0, viewer);
	cout << "Hoop4" << endl;

	input.setInputObserver(0, decoder);

	cout << "Hoop5" << endl;
}

int main(int argc, char* argv[])
{
	/*      Concept:

			 1) UdpSocket listens for input data.
			 2) Data is passed to H264Decoder which than parses
				the NALU files and passes everything to ffmpeg.
				The decoded data is than passed to the SdlViewer instance;
			 3) The SdlViewer uses SDL with OpenGL acceleration to show the received frames.
	 */
	
	for(int i = 1; i < argc; ++i) {
		std::string value(argv[i]);
		if (value == "--fullscreen") {
			fullscreen = true;
		}else if(value == "--oculus"){
			viewer = new OculusViewer(DEFAULT_WIDTH, DEFAULT_HEIGHT);
		}else if (value == "--help" || value == "-h") {
			cout << "--help           no idea what exactly this parameter does" << endl
				 << "--fullscreen     open fullscreen opengl context instead of vga window" << endl
				 << "--oculus   adjust screen window for Oculus Rift DK2" << endl;
			return 0;
		}
	}
	if(viewer == nullptr){
		viewer = new SdlViewer(DEFAULT_WIDTH, DEFAULT_HEIGHT);
	}

	input.initClient(cfg.getValueOfKey<string>("TARGET_IP").c_str(), TARGET_PORT);
	input.setInitCallback(&init);
	input.send(PROTOCOL_TYPE_INIT, nullptr, 0);
	viewer->setInputCallback(&inputCallback);
	viewer->setPositionCallback(&positionCallback);
	viewer->show(fullscreen);
	input.send(PROTOCOL_TYPE_CLOSE, nullptr, 0);
	input.close();

	return 0;
}
