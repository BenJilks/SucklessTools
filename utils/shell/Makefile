CC=g++
CXXFLAGS=-std=c++17 -pedantic -pedantic-errors -Werror -g\
    -Wno-unused-label \
	-Wno-unused-function \
	-Wno-unused-parameter \
	-Wno-unused-variable \

OBJ=shell.o \
	command.o \
	lexer.o \
	modules/autocomplete.o \
	modules/builtins.o \
	modules/alias.o \
	main.o

ifeq ($(PREFIX),)
    PREFIX := /usr/local
endif

%.o: %.cpp
	$(CC) -c -o $@ $< $(CXXFLAGS)

all: $(OBJ)
	$(CC) $^ -o shell $(CXXFLAGS)

clean:
	$(RM) $(OBJ)
	$(RM) shell

install:
	install -m 755 shell $(PREFIX)/bin/
