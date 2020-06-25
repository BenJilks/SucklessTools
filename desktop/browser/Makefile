CC=c++
CXXFLAGS=-std=c++17 -g
OBJ=main.o \
    html/lexer.o \
    html/parser.o

%.o: %.cpp
	$(CC) -c -o $@ $< $(CXXFLAGS)

all: $(OBJ)
	$(CC) -o browser $^ $(CXXFLAGS)

clean:
	$(RM) $(OBJ) browser
