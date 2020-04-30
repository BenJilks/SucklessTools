CC=g++
CXXFLAGS=-std=c++17 -pedantic -pedantic-errors -Werror \
    -Wno-unused-label \
	-Wno-unused-function \
	-Wno-unused-parameter \
	-Wno-unused-variable \

OBJ=shell.o \
	lexer.o \
	command.o \
	modules/autocomplete.o \
	main.o

%.cpp: %.o
	$(CC) -c -o $@ $< $(CXXFLAGS)

all: $(OBJ)
	$(CC) $^ -o shell

clean:
	$(RM) $(OBJ)
	$(RM) shell
