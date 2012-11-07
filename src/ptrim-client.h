/*******************************************************
PlasmaTrim Network client v0.1.1
Andrew Toy
Started: Oct 25 2012

Credit where credit is due:
	Signal 11: http:// www.signal11.us/oss/hidapi/
		Provided the HID API this software is using, some slight modifications needed to be made to hid.c/hid-libusb.c to allow 0x00 commands, but other than that this is 100% based off their API.
	Glohawk: http:// www.thephotonfactory.com/forum/viewtopic.php?f=5&t=104#p168
		Filled in the gaps in the HID code table.

This code may be used under the GPLv3 license
********************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include "ptrim-lib.h"

#define DEFAULT_PORT 26163

void sendFile(char* filename, int sockfd); // read the data from either stdin or a file into the socket
void help();
