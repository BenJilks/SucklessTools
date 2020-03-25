
all:
	gmake -C dwm
	gmake -C dmenu
	gmake -C st
	gmake -C pamixer

install:
	gmake -C dwm install
	gmake -C dmenu install
	gmake -C st install
	gmake -C pamixer install

clean:	
	gmake -C dwm clean
	gmake -C dmenu clean
	gmake -C st clean
	gmake -C pamixer clean
	rm dwm/config.h
	rm st/config.h
	rm dmenu/config.h

