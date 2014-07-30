CXX    ?= g++
#CXX	= clang++ -Wno-reserved-user-defined-literal
CFLAGS  = -Wall -std=c++11 -pthread -Wno-literal-suffix
LIBS    = -L/usr/lib/x86_64-linux-gnu
LIBS   += $(shell pkg-config --cflags --libs libavcodec libavutil libavformat sdl2)
SOURCE  = $(SRC)*.cpp \
          $(SRC)input/*.cpp \
          $(SRC)tools/*.cpp \
          $(SRC)output/*.cpp \
          $(SRC)decoder/*.cpp 

BINARY  = clientd
BINARYD = debug_clientd

all:
	$(CXX) $(SOURCE) $(CFLAGS) -o $(BINARY) -O3 $(LIBS)
debug:
	$(CXX) $(SOURCE) $(CFLAGS) -o $(BINARYD) -DDEBUG -g $(LIBS)
clean:
	rm -f $(BINARY) $(BINARYD)
	rm -f *.o

