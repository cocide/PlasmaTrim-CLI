/*******************************************************
PlasmaTrim HID communication stuffs v0.2.3
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
#include "../hidapi/hidapi.h"


// Headers needed for sleeping.
#ifdef WIN32
	#include <windows.h>
#else
	#include <unistd.h>
	#include <sys/ioctl.h>
#endif

#define MAX_DEVICES 32

#if __cplusplus
extern "C" {
#endif

void showInfo(hid_device *handle);

char *getSerial(hid_device *handle);
char *getName(hid_device *handle);
void setName(hid_device *handle, char *name);

unsigned char getBrightness(hid_device *handle); // recturns the saved brightness as a decimal perecentage (1-100)
unsigned char makeBrightness(int brightness);    // take some user input and make sure its between 1-100 then return it
void setBrightness(hid_device *handle, unsigned char brightness); // save the brightness on the device


void start(hid_device *handle);
void stop(hid_device *handle);


char *getColor(hid_device *handle); // grab the color from the device, returns as a ASCII string '000000111111222222333333444444555555666666777777'
char *makeHumanColor(char* data); // basically just add spaces to the brightness to seperate the LEDs '000000 111111 222222 333333 444444 555555 666666 777777'
unsigned char *makeColor(char *color, bool full_length); // turn an ASCII string with hex values in upper or lower case that can be 3/6 characters total or per LED into the binary data needed to program the device
                                                         // example inputs '00f' '00F' '00ff00' '000111222333444555666777' '000 111 222 333 444 555 666 777' and '000000 111111 222222 333333 444444 555555 666666 777777'
                                                         // depending on what function you are calling you might need 1 or 2 hex values per LED, full_length = true is 2 hex values per LED
void setColor(hid_device *handle, unsigned char *color, unsigned char brightness); // temporarly set the color of a device using a full length color string

void download(hid_device *handle, char** fileData, int lineSize, int numberLines, bool blank); // download data from the device and put it in fileData which has numberLines lines that are lineSize long
int upload(hid_device *handle, char** fileData, bool blank, unsigned char id, unsigned char totalDevices, bool warn); // upload the data from a file to a device, the colors used in this format are not full length. warn=true makes it send notices to stderr. returns 0 on success -1 on failure
void streamData(hid_device *handle, char *filename, unsigned char brightness, unsigned char id, unsigned char totalDevices); // play a sequence on the device. you can not currently use standard in because the threadding process requires the file to be read once per PlasmaTrim

unsigned char getActiveSlots(hid_device *handle); // return the decimal value for the active slots on a device (1-76)
void setActiveSlots(hid_device *handle, unsigned char slots); // set the number of active slots (1-76)

void fade(hid_device *handle, char *userColor, char *fadeTime, unsigned char brightness); // fade to a full length color over some time (0-F), the timing is very approximate and uses the same numbering as the sequence files 
void hold(char *holdTime); // hold for some time (0-F), same numbering as the sequence files

void readData(hid_device *handle); // just used to get the data off the buffer
void delay(unsigned int ms); // usleep, but multi platform
void start_comm(hid_device *handle); // start up the hid stuff and read the first chunk of data to remove any un-read data from the buffer just to make sure we are good to go


void format_print(int tab, char *string);
void ptrim_lib_version();
void ptrim_version();
void ptrim_server_version();
void ptrim_client_version();

#if __cplusplus
}
#endif

/*
note:
man pages converted to c help statements using:
export MANWIDTH=9999; man $1 | tail -n +8 | head -n -10 | sed -e 's/"/\\"/g' -e 's/       / /g' -e 's/\x1B\[[0-9]*m//g' -e 's/\x1B\[[0-9]*m//g' -e 's/^\([^ ].*\)/format_print(0, "\1");/g' -e 's/^ \([^ ].*\)/format_print(1, "\1");/g' -e 's/^  \([^ ].*\)/format_print(2, "\1");/g' -e 's/^$/format_print(0, "");/g' -e 's/^/\t/g' > output
*/