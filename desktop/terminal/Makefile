CC=g++
CXXFLAGS=-std=c++17 -I/usr/include/freetype2 -g
LINKFLAGS=-lutil -lX11 -lXft
OBJ=main.o \
	terminal.o \
	escapes.o \
	output/useless.o \
	output/xlib.o

%.o: %.cpp
	$(CC) -c -o $@ $< $(CXXFLAGS)

all: $(OBJ)
	$(CC) -o terminal $^ $(LINKFLAGS) $(CXXFLAGS)

clean:
	$(RM) $(OBJ)
	$(RM) terminal

