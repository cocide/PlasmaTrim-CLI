all: ptrim

CC       ?= gcc
CFLAGS   ?= -Wall -g

CXX      ?= g++
CXXFLAGS ?= -Wall -g

COBJS     = hid-libusb.o
CCOBJS    = ptrim.o
OBJS      = $(COBJS) $(CCOBJS)
LIBS      = `pkg-config libusb-1.0 libudev --libs`
INCLUDES ?= -I../ `pkg-config libusb-1.0 --cflags`


ptrim: $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ $(LIBS) -o ptrim

$(COBJS): %.o: %.c
	$(CC) $(CFLAGS) -c $(INCLUDES) $< -o $@

$(CCOBJS): %.o: ../%.cc
	$(CXX) $(CXXFLAGS) -c $(INCLUDES) -Wno-write-strings $< -o $@

clean:
	rm -f $(OBJS) ptrim

.PHONY: clean
