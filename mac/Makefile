all: ptrim

CC=gcc
CXX=g++
COBJS=hid.o
CCOBJS=ptrim.o
OBJS=$(COBJS) $(CCOBJS)
CFLAGS+=-I../ -Wall -g -c 
LIBS=-framework IOKit -framework CoreFoundation


ptrim: $(OBJS)
	g++ -Wall -g $^ $(LIBS) -o ptrim

$(COBJS): %.o: %.c
	$(CC) $(CFLAGS) $< -o $@

$(CCOBJS): %.o: ../%.cc
	$(CXX) $(CFLAGS) -Wno-write-strings $< -o $@

clean:
	rm -f *.o ptrim $(CCOBJS)

.PHONY: clean
