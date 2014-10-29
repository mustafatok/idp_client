CXX    ?= g++
#CXX	= clang++ -Wno-reserved-user-defined-literal
CFLAGS  = -Wall -std=c++11 -pthread -Wno-literal-suffix
LIBS    = -L/usr/lib/x86_64-linux-gnu -lz -ldl -lbz2 -lcrypto
LIBS   += $(shell pkg-config --cflags --libs sdl2)
LIBS   += $(shell export PKG_CONFIG_PATH=`pwd`/build/stage/lib/pkgconfig/ && pkg-config --cflags libavcodec libavutil libavfilter libswscale libavformat)
OBJS    = build/stage/lib/libavformat.a build/stage/lib/libavcodec.a \
          build/stage/lib/libavdevice.a build/stage/lib/libavfilter.a \
          build/stage/lib/libavutil.a build/stage/lib/libswresample.a \
          build/stage/lib/libswscale.a
SRC     = src/

FFMPEG_SRC = git://git.videolan.org/ffmpeg.git
FFMPEG_VER = n2.2.4 #version currently on fedora 20 is 2.1.x
FFMPEG_FLAGS = --enable-static --disable-shared #--disable-pthreads

SOURCE  = $(SRC)*.cpp \
          $(SRC)input/*.cpp \
          $(SRC)tools/*.cpp \
          $(SRC)output/*.cpp \
          $(SRC)decoder/*.cpp 

BINARY  = clientd
BINARYD = debug_clientd

all:	libs client
clean:	client_clean libs_clean

client:
	$(CXX) $(SOURCE) $(OBJS) $(CFLAGS) -o build/$(BINARY) -O3 $(LIBS)

client_debug:
	$(CXX) $(SOURCE) $(OJBS) $(CFLAGS) -o build/$(BINARYD) -DDEBUG -g $(LIBS)

libs:
	mkdir -p build/stage
	mkdir -p build/src
	
	# ffmpeg
	if [ ! -f build/src/ffmpeg/configure ]; then \
		cd build/src/ && git clone $(FFMPEG_SRC) && cd ffmpeg && git checkout $(FFMPEG_VER); \
	fi
	if [ ! -f build/stage/lib/libavcodec.a ]; then \
		cd build/src/ffmpeg && ./configure --prefix=`pwd`/../../stage/ $(FFMPEG_FLAGS) && make && make install; \
	fi

libs_clean:
	rm -rf ./build

client_clean:
	rm -f build/$(BINARY) build/$(BINARYD)

