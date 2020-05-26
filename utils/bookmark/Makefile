CC=clang++
CXXFLAGS=-std=c++17 -I/usr/local/include -g
LINKFLAGS=-L/usr/local/lib -ljson
OBJ=bookmark.o

ifeq ($(PREFIX),)
	PREFIX := /usr/local
endif

%.o: %.cpp
	$(CC) -c -o $@ $< $(CXXFLAGS)

all: $(OBJ)
	$(CC) -o bookmark $^ $(CXXFLAGS) $(LINKFLAGS)

clean:
	$(RM) $(OBJ)
	$(RM) bookmark bookmark.core

install:
	install -m 755 bookmark $(PREFIX)/bin/
