CC=clang++
AR=ar
CXXFLAGS=-std=c++17 -g
OBJ=libjson.o main.o
INCLUDE=libjson.hpp

ifeq ($(PREFIX),)
    PREFIX := /usr/local
endif

%.o: %.cpp
	$(CC) -c -o $@ $< $(CXXFLAGS)

all: $(OBJ)
#	$(CC) -o test $^ $(CXXFLAGS)
	$(AR) rs libjson.a $(OBJ)

clean:
	$(RM) $(OBJ)
	$(RM) libjson.a

install:
	install -m 644 libjson.a $(PREFIX)/lib/
	install -m 644 libjson.hpp $(PREFIX)/include/
