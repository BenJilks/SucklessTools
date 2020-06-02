CC=clang++
CXXFLAGS=-std=c++17 -I/usr/local/include -g
LINKFLAGS=-L/usr/local/lib -ljson -lwebinterface -lpthread
OBJ=	cron.o \
	timer.o \
	main.o

%.o: %.cpp
	$(CC) -c -o $@ $< $(CXXFLAGS)

all: $(OBJ)
	$(CC) -o crond $^ $(LINKFLAGS) $(CXXFLAGS)

clean:
	$(RM) $(OBJ)
	$(RM) crond crond.core
