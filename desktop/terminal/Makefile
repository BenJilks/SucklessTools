CC=g++
CXXFLAGS=-std=c++17 -I/usr/include/freetype2 -O2
LINKFLAGS=-lutil -lX11 -lXft
OBJ=main.o \
	terminal.o \
	escapes.o \
	line.o \
	color.o \
	decoder.o \
	output/output.o \
	output/xlib.o

ifeq ($(PREFIX),)
    PREFIX := /usr/local
endif

%.o: %.cpp
	$(CC) -c -o $@ $< $(CXXFLAGS)

all: $(OBJ)
	$(CC) -o terminal $^ $(LINKFLAGS) $(CXXFLAGS)

clean:
	$(RM) $(OBJ)
	$(RM) terminal

install:
	install -m 755 terminal $(PREFIX)/bin/
