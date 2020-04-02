
SUDO=sudo
FLAGS=LDFLAGS="-L/usr/local/lib" CFLAGS="-I/usr/local/include"

all:
	gmake -C dwm
	gmake -C dmenu
	gmake -C st
	gmake -C slstatus
	gmake -C pamixer
	gmake -C feh
	gmake -C printscreen
	${FLAGS} gmake -C sxhkd

install:
	${SUDO} gmake -C dwm install
	${SUDO} gmake -C dmenu install
	${SUDO} gmake -C st install
	${SUDO} gmake -C slstatus install
	${SUDO} gmake -C pamixer install
	${SUDO} gmake -C feh install
	${SUDO} gmake -C printscreen install
	${SUDO} ${FLAGS} gmake -C sxhkd install
	cp -r config ~/.config/dwm

clean:	
	gmake -C dwm clean
	gmake -C dmenu clean
	gmake -C st clean
	gmake -C pamixer clean
	gmake -C slstatus clean
	${SUDO} rm -rf feh
	${SUDO} rm -rf sxhkd
	${SUDO} rm -rf xclip
	${SUDO} rm -rf printscreen	
	${SUDO} rm dwm/config.h
	${SUDO} rm st/config.h
	${SUDO} rm dmenu/config.h
	${SUDO} rm slstatus/config.h

