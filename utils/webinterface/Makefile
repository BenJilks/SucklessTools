CC=clang++
AR=ar
CXXFLAGS=-std=c++17 -I/usr/local/include
OBJ=	webinterface.o \
	request.o \
	template.o \
	table.o

ifeq ($(PREFIX),)
    PREFIX := /usr/local
endif

%.o: $.cpp
	$(CC) -o $@ $< $(CXXFLAGS)

all: $(OBJ)
#	$(CC) -o test test.cpp $^ $(CXXFLAGS)
	$(AR) rs libwebinterface.a $(OBJ)

clean:
	$(RM) $(OBJ) test.o
	$(RM) test test.core
	$(RM) libwebinterface.a

install:
	mkdir -p $(PREFIX)/include/webinterface
	install -m 644 libwebinterface.a $(PREFIX)/lib/
	install -m 644 webinterface.hpp $(PREFIX)/include/webinterface/
	install -m 644 request.hpp $(PREFIX)/include/webinterface/
	install -m 644 template.hpp $(PREFIX)/include/webinterface/
	install -m 644 table.hpp $(PREFIX)/include/webinterface/

uninstall:
	$(RM) -r $(PREFIX)/include/webinterface
	$(RM) $(PREFIX)/lib/libwebinterface.a
