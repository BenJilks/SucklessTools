CC=g++
LINKFLAGS=-ljson
OBJ=bookmark.o

ifeq ($(PREFIX),)
    PREFIX := /usr/local
endif

%.o: %.cpp
	$(CC) -c -o $@ $<

all: $(OBJ)
	$(CC) -o bookmark $^ $(LINKFLAGS)

clean:
	$(RM) $(OBJ)
	$(RM) bookmark

install:
	install -m 755 bookmark $(PREFIX)/bin/
