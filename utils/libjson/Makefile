CC=c++
AR=ar
CXXFLAGS=-std=c++17 -g
OBJ=libjson.o \
    allocator.o

ifeq ($(PREFIX),)
    PREFIX := /usr/local
endif

%.o: %.cpp
	$(CC) -c -o $@ $< $(CXXFLAGS)

all: $(OBJ)
	$(CC) -o test $^ $(CXXFLAGS) main.cpp
#	$(AR) rs libjson.a $(OBJ)

clean:
	$(RM) $(OBJ)
	$(RM) libjson.a test

install:
	mkdir -p $(PREFIX)/include/libjson
	install -m 644 libjson.a $(PREFIX)/lib/
	install -m 644 libjson.hpp $(PREFIX)/include/libjson/
	install -m 644 allocator.hpp $(PREFIX)/include/libjson/
	install -m 644 forward.hpp $(PREFIX)/include/libjson/

uninstall:
	$(RM) -r $(PREFIX)/include/libjson
	$(RM) $(PREFIX)/lib/libjson.a
