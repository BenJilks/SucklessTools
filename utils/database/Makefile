CC=c++
CXXFLAGS=-std=c++17 -g
OBJ=main.o \
    database.o \
    table.o \
    column.o \
    row.o \
    entry.o

%.o: %.cpp
	$(CC) -c -o $@ $< $(CXXFLAGS)

all: $(OBJ)
	$(CC) -o database $^ $(CXXFLAGS)

clean:
	$(RM) $(OBJ) database
