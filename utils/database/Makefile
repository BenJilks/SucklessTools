CC=c++
CXXFLAGS=-std=c++17 -g
OBJ=main.o \
    database.o \
    column.o

%.o: %.cpp
	$(CC) -c -o $@ $< $(CXXFLAGS)

all: $(OBJ)
	$(CC) -o database $^ $(CXXFLAGS)

clean:
	$(RM) $(OBJ) database
