CC           ?= gcc
CFLAGS       += -Wall -g

CXX          ?= g++
CXXFLAGS     += -Wall -g

CCOBJS        = ptrim-lib.o

LIBS          = -l curses
INCLUDES     += -I hidapi/ -I src/

UNAME        := $(shell uname)

ifeq ($(UNAME), Linux)
	COBJS     = hid-libusb.o
	LIBS     += `pkg-config libusb-1.0 libudev --libs`
	INCLUDES += -I hidapi/linux/ `pkg-config libusb-1.0 --cflags`
	OS        = linux
endif
ifeq ($(UNAME), Darwin)
	COBJS     = hid.o
	LIBS     += -framework IOKit -framework CoreFoundation
	INCLUDES += -I hidapi/mac/
	OS        = mac
endif

ROOT_TEST     = if test `whoami` != "root"; then echo "You need to be root to install, try using \`sudo make install\`" >&2; exit 1; fi
MAN1_PAGES    = man/man1/ptrim.1.bz2 man/man1/ptrim-server.1.bz2 man/man1/ptrim-client.1.bz2
MAN_PAGES     = $(MAN1_PAGES)

VERSION       = 0.3.1
DIRECTORY = PlasmaTrim_CLI-$(OS)-v$(VERSION)



all: ptrim ptrim-server ptrim-client

install: all man
	$(ROOT_TEST)
	install ptrim ptrim-server ptrim-client /usr/bin/

install-zip: all $(MAN_PAGES)
	mkdir -p $(DIRECTORY)
	echo '#!/bin/bash' > $(DIRECTORY)/install.sh
	echo '$(ROOT_TEST)' >> $(DIRECTORY)/install.sh
	echo 'install $(MAN1_PAGES) /usr/share/man/man1/' >> $(DIRECTORY)/install.sh
	echo 'install ptrim ptrim-server ptrim-client /usr/bin/' >> $(DIRECTORY)/install.sh
	chmod 755 $(DIRECTORY)/install.sh
	cp -a ptrim ptrim-server ptrim-client $(DIRECTORY)
	mkdir -p $(DIRECTORY)/man/man1
	cp -a $(MAN1_PAGES) $(DIRECTORY)/man/man1
	zip -r $(DIRECTORY).zip $(DIRECTORY)

ptrim: $(COBJS) $(CCOBJS) ptrim.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ $(LIBS) -o ptrim

ptrim-server: $(COBJS) $(CCOBJS) ptrim-server.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ $(LIBS) -o ptrim-server

ptrim-client: $(COBJS) $(CCOBJS) ptrim-client.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ $(LIBS) -o ptrim-client

man: $(MAN_PAGES)
	$(ROOT_TEST)
	install $(MAN1_PAGES) /usr/share/man/man1/

man/%.bz2: man/%
	bzip2 -kf $?

hid.o: hidapi/mac/hid.c
	$(CC) $(CFLAGS) -c $(INCLUDES) $< -o $@

hid-libusb.o: hidapi/linux/hid-libusb.c
	$(CC) $(CFLAGS) -c $(INCLUDES) $< -o $@

%.o: src/%.cc src/*.h
	$(CXX) $(CXXFLAGS) -c $(INCLUDES) -Wno-write-strings $< -o $@

clean:
	rm -f *.o ptrim ptrim-server ptrim-client man/*/*bz2
	rm -rf PlasmaTrim_CLI-*

.PHONY: clean install man
