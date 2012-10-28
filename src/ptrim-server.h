/*******************************************************
PlasmaTrim Network goodness v0.1.0
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
#include <wchar.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
#include <ncurses.h>
#include <errno.h>


#include "hidapi.h"
#include "ptrim-lib.h"


#define DEFAULT_PORT 26163
#define MAX_CLIENTS 21 // always put one extra in so we dont fill up all the threads; bad things happen then
#define MAX_BUFFER_SIZE 4096



// status codes for the PlasmaTrims:
#define DISCONNECT 0 	// disconnect
#define PLAY 1 			// playing saved sequence
#define IDLE 2 			// idle/static display
#define STREAM 3 		// playing temporary sequence
#define LOCK 4 			// locked to a client

// uptate notification codes
#define NAME 0x01
#define BRIGHTNESS 0x02
#define TEMP_BRIGHTNESS 0x04
#define STATUS 0x08


struct hid_device_info *devs, *cur_dev;

int numberDevices;
int numberClients;
hid_device *handle[MAX_DEVICES];
char *openDevices[MAX_DEVICES];
char *serial[MAX_DEVICES];
char *name[MAX_DEVICES];
int brightness[MAX_DEVICES];
int tempBrightness[MAX_DEVICES];
int status[MAX_DEVICES];
int notify[MAX_DEVICES];
int lock[MAX_DEVICES];

char *code;
int socket_id[MAX_CLIENTS];

struct socket_struct {
	int socket;
};


struct instance_struct {
	char** commands;
	hid_device *handle;
	int handleID;
	int id;
	int selectedDevices;
	int output;
};


void help();
void clientWelcome(int fd);
void *monitorDevices(void *arguements);
void runCommand(char** commands, int output, int socketID);
void *runThread(void *arguments);
int findDeviceNumber(char* serialNumber);
void *manageConnection(void *arguements);
void connectedDevices( int fd);
void deviceInfo( int fd, int id);
void statusInfo(int fd, int id);
void *sendNotifications(void* );


/*

when you connect you have 5 seconds to enter the security code chosen by the user or you will be disconnected
invalid security codes will return:
	[incorrect code]
you will then be given info about the server along with a list of devices
example:
	[about]
	PlasmaTrim Network Server
	version 2.1.0
	max devices 32
	max clients 20
	[/about]
	[devices]
	Serial: 00B856B2
	Name: PlasmaTrim RGB-8 0xB856B2
	Saved Brightness: 001%
	Temp Brightness: 001%
	Status: Playing
	---
	Serial: 00B85776
	Name: PlasmaTrim RGB-8 0xB85776
	Saved Brightness: 001%
	Temp Brightness: 001%
	Status: Playing
	[/devices]

you may enter commands once you see [ready]
commands are entered one arguement per line. Each line must end in \r\n. The server waits to recieve one blank line before processing a command.
example:
	00B85776
	name
	Some New Name

would be a valid syntax for changing the name (note: the tabs are for ease of reading, do not send the tabs to the server)
after each command is taken by the server you will get [accepted], when the job is done it will give you [ready] again
the server will automatically disconnect clients after 5 min of inactivity. to keep the connection active send '.\r\n' (period followed by a new line). you will then get [ready] again

general command errors:
	[serial permission denied]
	[unknown command]
	[device not found]

commands and results/errors:
	serial start
		result via update
	serial stop
		result via update
	serial name new_name
		result via update
	serial brightness temp ###
		result via update
	serial brightness ###
		result via update
	serial brightness reset
		result via update
	serial brightness save
		result via update
	serial color
		result:
			[serial color 000000 111111...]
	serial color new-color
	serial color new-color new-temp-brightness
	serial fade new-color time
	serial fade new-color time new-temp-brightness

	serial upload file_contents
		result:
			[serial upload success]
		error:
			[serial upload error]
	serial full_upload file_contents
		result:
			[serial upload success]
		error:
			[serial upload error]

	serial download
		result:
			[serial line_of_file_contents]
	serial full_download
		result:
			[serial line_of_file_contents]

	serial lock
		error:
			[serial already locked]
	serial unlock
		error:
			[serial not locked]
	serial force unlock
		error:
			[serial not locked]


updates will be wrapped in [update], they will always show the serial and whatever element(s) changed
updates are sent to ALL the clients, the idea is that if you change the brightness of a device all the other clients need to know that in order to have accurate information
an update where everything changd would be:
	[update]
	Serial: serial
	Name: new_name
	Saved Brightness: new_saved_brightness%
	Temp Brightness: new_temp_brightness%
	Status: new_status
	[/update]

adding devices:
	[added]
	Serial: serial
	Name: name
	Saved Brightness: brightness%
	Temp Brightness: temp_brightness%
	Status: status
	[/added]

removing devices:
	[removed serial]


to disconnect from the server send '!\r\n'

*/
