
SUDO=sudo
FLAGS=LDFLAGS="-L/usr/local/lib" CFLAGS="-I/usr/local/include"

MAKE=gmake

all:
	${MAKE} -C dwm
	${MAKE} -C dmenu
	${MAKE} -C st
	${MAKE} -C slstatus
	${MAKE} -C pamixer
	${MAKE} -C feh
	${MAKE} -C printscreen
	${FLAGS} ${MAKE} -C sxhkd

install:
	${SUDO} ${MAKE} -C dwm install
	${SUDO} ${MAKE} -C dmenu install
	${SUDO} ${MAKE} -C st install
	${SUDO} ${MAKE} -C slstatus install
	${SUDO} ${MAKE} -C pamixer install
	${SUDO} ${MAKE} -C feh install
	${SUDO} ${MAKE} -C printscreen install
	${SUDO} ${FLAGS} ${MAKE} -C sxhkd install
	cp -r config ~/.config/dwm
	mkdir ~/backgrounds
	${SUDO} cp config/update_background /usr/bin/

clean:	
	${MAKE} -C dwm clean
	${MAKE} -C dmenu clean
	${MAKE} -C st clean
	${MAKE} -C pamixer clean
	${MAKE} -C slstatus clean
	${SUDO} rm -rf feh
	${SUDO} rm -rf sxhkd
	${SUDO} rm -rf xclip
	${SUDO} rm -rf printscreen	
	${SUDO} rm dwm/config.h
	${SUDO} rm st/config.h
	${SUDO} rm dmenu/config.h
	${SUDO} rm slstatus/config.h

