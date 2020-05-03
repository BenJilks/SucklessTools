CC=g++
AR=ar
CXXFLAGS=-std=c++17
OBJ=libjson.o
INCLUDE=libjson.hpp

ifeq ($(PREFIX),)
    PREFIX := /usr/local
endif

%.o: %.cpp
	$(CC) -c -o $@ $< $(CXXFLAGS)

%.hpp: %.tinylex
	tinylex -l cpp -o $@ $<

all: lexer.hpp $(OBJ)
#	$(CC) -o test $(OBJ)
	$(AR) rs libjson.a $(OBJ)

clean:
	$(RM) $(OBJ) lexer.hpp
	$(RM) libjson.a

install:
	install -m 644 libjson.a $(PREFIX)/lib/
	install -m 644 libjson.hpp $(PREFIX)/include/
