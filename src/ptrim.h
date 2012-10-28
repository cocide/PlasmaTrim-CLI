/*******************************************************
PlasmaTrim CLI goodness v0.2.2
Andrew Toy
Started: Oct 23 2012

Credit where credit is due:
	Signal 11: http:// www.signal11.us/oss/hidapi/
		Provided the HID API this software is using, some slight modifications needed to be made to hid.c/hid-libusb.c to allow 0x00 commands, but other than that this is 100% based off their API.
	Glohawk: http:// www.thephotonfactory.com/forum/viewtopic.php?f=5&t=104#p168
		Filled in the gaps in the HID code table.

This code may be used under the GPLv3 license
********************************************************/

#include <stdio.h>
#include <wchar.h>
#include <string.h>
#include <stdlib.h>
#include "hidapi.h"
#include "ptrim-lib.h"


// Headers needed for sleeping and therads
#ifdef WIN32
	#include <windows.h>
#else
	#include <unistd.h>
	#include <pthread.h>
#endif


#ifdef WIN32
	UNREFERENCED_PARAMETER(argc);
	UNREFERENCED_PARAMETER(argv);
#endif


struct arg_struct {
	int argc;
	char** argv;
	hid_device *handle;
	unsigned char id;
	unsigned char totalDevices;
};

struct hid_device_info *devs, *cur_dev;

void *runCommand(void *arguments);
void help();


void upload(hid_device *handle, char* filename, bool blank, unsigned char id, unsigned char totalDevices); // read the data from either stdin or a file into the ptrim-lip upload command
void download(hid_device *handle, char *filename, bool blank); // get the data from the device and put it into a file