#include <fstream>
#include <iostream>
#include <thread>

#include "decoder/decoder.h"
#include "input/network.h"
#include "output/sdlviewer.h"
#include "tools/timer.h"

#include "global.h"

using namespace std;

const int DEFAULT_WIDTH = 1280;
const int DEFAULT_HEIGHT = 720;
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

        bool fullscreen = false;
        for(int i = 1; i < argc; ++i) {
                std::string value(argv[i]);
                if (value == "--fullscreen") {
                        fullscreen = true;
                } else if (value == "--help" || value == "-h") {
                        cout << "--help         no idea what exactly this parameter does" << endl
                             << "--fullscreen   open fullscreen opengl context instead of vga window" << endl;
                        return 0;
                }
        }

        UdpSocket socket;
        H264Decoder decoder;
        SdlViewer viewer(DEFAULT_WIDTH, DEFAULT_HEIGHT);

        socket.setReadCallback(&decoder, &H264Decoder::decode);
        decoder.setFrameCallback(&viewer, &SdlViewer::showFrame);

        socket.initClient(TARGET_IP, TARGET_PORT);
        socket.send(PROTOCOL_TYPE_INIT, nullptr, 0);
        viewer.show(fullscreen);
        socket.send(PROTOCOL_TYPE_CLOSE, nullptr, 0);
        socket.close();
        
        return 0;
}
