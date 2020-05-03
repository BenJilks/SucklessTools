CC=g++
CXXFLAGS=-std=c++17 -pedantic -pedantic-errors -Werror \
    -Wno-unused-label \
	-Wno-unused-function \
	-Wno-unused-parameter \
	-Wno-unused-variable \

OBJ=shell.o \
	command.o \
	lexer.o \
	modules/autocomplete.o \
	modules/builtins.o \
	main.o

%.o: %.cpp
	$(CC) -c -o $@ $< $(CXXFLAGS)

all: $(OBJ)
	$(CC) $^ -o shell $(CXXFLAGS)

clean:
	$(RM) $(OBJ)
	$(RM) shell
