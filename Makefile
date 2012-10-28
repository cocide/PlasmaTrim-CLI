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
endif
ifeq ($(UNAME), Darwin)
	COBJS     = hid.o
	LIBS     += -framework IOKit -framework CoreFoundation
	INCLUDES += -I hidapi/mac/
endif


all: ptrim ptrim-server ptrim-client

ptrim: $(COBJS) $(CCOBJS) ptrim.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ $(LIBS) -o ptrim

ptrim-server: $(COBJS) $(CCOBJS) ptrim-server.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ $(LIBS) -o ptrim-server

ptrim-client: $(COBJS) $(CCOBJS) ptrim-client.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ $(LIBS) -o ptrim-client

hid.o: hidapi/mac/hid.c
	$(CC) $(CFLAGS) -c $(INCLUDES) $< -o $@

hid-libusb.o: hidapi/linux/hid-libusb.c
	$(CC) $(CFLAGS) -c $(INCLUDES) $< -o $@

%.o: src/%.cc src/*.h
	$(CXX) $(CXXFLAGS) -c $(INCLUDES) -Wno-write-strings $< -o $@

clean:
	rm -f *.o ptrim ptrim-server ptrim-client

.PHONY: clean
