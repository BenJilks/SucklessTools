CC=c++
CXXFLAGS=-std=c++17 -I/usr/local/include -g
LINKFLAGS=-L/usr/local/lib -ljson -lwebinterface -lpthread -ldatabase

# Enable if on ARM
# LINKFLAGS+=-lstdc++fs

OBJ=	cron.o \
	timer.o \
	main.o

ifeq ($(PREFIX),)
    PREFIX := /usr/local
endif

%.o: %.cpp
	$(CC) -c -o $@ $< $(CXXFLAGS)

all: $(OBJ)
	$(CC) -o crond $^ $(LINKFLAGS) $(CXXFLAGS)

clean:
	$(RM) $(OBJ)
	$(RM) crond crond.core

install:
	mkdir -p $(PREFIX)/share/crond/web
	install -m 644 web/log.html $(PREFIX)/share/crond/web/
	install -m 644 web/log_entry.html $(PREFIX)/share/crond/web/
	install -m 644 web/style.css $(PREFIX)/share/crond/web/
	install -m 7111 crond $(PREFIX)/bin/

uninstall:
	$(RM) -r $(PREFIX)/share/crond/web
	$(RM) $(PRIFIX)/bin/crond
