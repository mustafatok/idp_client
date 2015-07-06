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

const char* TARGET_IP = "127.0.0.1";
const uint16_t TARGET_PORT = 2525;


int main(int argc, char* argv[])
{
	/*      Concept:

			 1) UdpSocket listens for input data.
			 2) Data is passed to H264Decoder which than parses
				the NALU files and passes everything to ffmpeg.
				The decoded data is than passed to the SdlViewer instance;
			 3) The SdlViewer uses SDL with OpenGL acceleration to show the received frames.
	 */

	UdpSocket socket;
	// H264Decoder decoder;
	MultiH264Decoder decoder("verticalConcat");
	// MultiH264Decoder decoder("leftResized");
	// MultiH264Decoder decoder("leftBlurred");
	// MultiH264Decoder decoder("rightBlurred");
	SdlViewer *viewer = nullptr;
	
	bool fullscreen = false;
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

	

	socket.setInputObserver(0, &decoder);
	decoder.setDecoderObserver(0, viewer);
	socket.initClient(TARGET_IP, TARGET_PORT);
	socket.send(PROTOCOL_TYPE_INIT, nullptr, 0);
	viewer->show(fullscreen);
	socket.send(PROTOCOL_TYPE_CLOSE, nullptr, 0);
	socket.close();
	
	return 0;
}
