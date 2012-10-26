CC           ?= gcc
CFLAGS       += -Wall -g

CXX          ?= g++
CXXFLAGS     += -Wall -g

CCOBJS        = ptrim-lib.o


UNAME        := $(shell uname)

ifeq ($(UNAME), Linux)
	COBJS     = hid-libusb.o
	LIBS      = `pkg-config libusb-1.0 libudev --libs`
	INCLUDES += -I hidapi/ -I hidapi/linux/ -I src/ `pkg-config libusb-1.0 --cflags`
endif
ifeq ($(UNAME), Darwin)
	COBJS     = hid.o
	LIBS      = -framework IOKit -framework CoreFoundation
	INCLUDES += -I hidapi/ -I hidapi/mac/ -I src/
endif


all: ptrim

ptrim: $(COBJS) $(CCOBJS) ptrim.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ $(LIBS) -o ptrim

hid.o: hidapi/mac/hid.c
	$(CC) $(CFLAGS) -c $(INCLUDES) $< -o $@

hid-libusb.o: hidapi/linux/hid-libusb.c
	$(CC) $(CFLAGS) -c $(INCLUDES) $< -o $@

%.o: src/%.cc src/*.h
	$(CXX) $(CXXFLAGS) -c $(INCLUDES) -Wno-write-strings $< -o $@

clean:
	rm -f *.o ptrim

.PHONY: clean
