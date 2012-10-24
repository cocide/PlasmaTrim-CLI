/*******************************************************
PlasmaTrim CLI goodness
Andrew Toy
Started: Oct 23 2012

Credit where credit is due:
	Signal 11: http://www.signal11.us/oss/hidapi/
		Provided the HID API this software is using, some slight modifications needed to be made to hid.c/hid-libusb.c to allow 0x00 commands, but other than that this is 100% based off their API.
	Glohawk: http://www.thephotonfactory.com/forum/viewtopic.php?f=5&t=104#p168
		Filled in the gaps in the HID code table.

Anyone may use, modify, distribute, use, code from this work. Just make a footnote in some comment and all is good.
********************************************************/

#include <stdio.h>
#include <wchar.h>
#include <string.h>
#include <stdlib.h>
#include "hidapi.h"


// Headers needed for sleeping.
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

// instead of having to try to do signal processing across multiple platforms (eventually) just keep track of how many threads are going on to sync things
unsigned char running_threads;



struct arg_struct {
	int argc;
	char** argv;
	hid_device *handle;
	unsigned char id;
};


void *runCommand(void *arguments);

void showInfo(hid_device *handle);
char *getSerial(hid_device *handle);
char *getName(hid_device *handle);
void setName(hid_device *handle, char *name);

unsigned char getBrightness(hid_device *handle);
unsigned char makeBrightness(int brightness);
void setBrightness(hid_device *handle, unsigned char brightness);


void start(hid_device *handle);
void stop(hid_device *handle);


char *getColor(hid_device *handle);
char *makeHumanColor(char* data);
unsigned char *makeColor(char *color, bool full_length);
void setColor(hid_device *handle, unsigned char *color, unsigned char brightness);

void download(hid_device *handle, char *filename, bool blank);
void upload(hid_device *handle, char *filename, bool blank, unsigned char id);
void streamData(hid_device *handle, char *filename, unsigned char brightness, unsigned char id);

unsigned char getActiveSlots(hid_device *handle);
void setActiveSlots(hid_device *handle, unsigned char slots);

void fade(hid_device *handle, char *userColor, char *fadeTime, unsigned char brightness);
void hold(char *holdTime);

void readData(hid_device *handle);
void delay(unsigned int ms);

void help();


int main(int argc, char* argv[]) {
	unsigned char max_devices = 32, number_devices = 0, i;
	hid_device *handle[max_devices];

	struct hid_device_info *devs, *cur_dev;
	
	if (argc == 1) {
		//no arguements were given so find/list devices and info
		devs = hid_enumerate(0x26f3, 0x1000);
		cur_dev = devs;	
		while (cur_dev && number_devices < max_devices) {
			handle[number_devices] = hid_open_path(cur_dev->path);
			if (handle[number_devices]) {
				// Set the hid_read() function to be non-blocking.
				hid_set_nonblocking(handle[number_devices], 1);

				showInfo(handle[number_devices]);
				printf("Path: %s\r\n", cur_dev->path);

				number_devices++;
			}
			printf("\r\n");

			cur_dev = cur_dev->next;
		}
		hid_free_enumeration(devs);
		if (number_devices == 1) {
			//use a bit better english for only one device
			printf("1 PlasmaTrim was found.\r\n");
		} else {
			printf("%d PlasmaTrims were found.\r\n", number_devices);
		}
	} else {
		if (strcmp(argv[1], "help") == 0) {
			help();
			return 0;
		} else if (strcmp(argv[1], "all") == 0) {
			//open all the PlasmaTrims
			devs = hid_enumerate(0x26f3, 0x1000);
			cur_dev = devs;	
			while (cur_dev && number_devices < max_devices) {
				handle[number_devices] = hid_open_path(cur_dev->path);
				if (handle[number_devices]) {
					// Set the hid_read() function to be non-blocking.
					hid_set_nonblocking(handle[number_devices], 1);
					number_devices++;
				}
				cur_dev = cur_dev->next;
			}
			hid_free_enumeration(devs);
		} else if (argv[1][8] == ' ' || argv[1][12] == ' ') {
			bool last = false, found = false;
			unsigned char offset = 0, paths_to_open = 0;
			char* device = new char[14];
			char* path_list[14];
			memset(device,0x00,14);
			i = 0;
			while (!last && paths_to_open < max_devices) {
				if (argv[1][i] == ' ' || argv[1][i] == 0x00) {
					//this is either a space or the last element
					if (argv[1][i] == 0x00 ) {
						//if its the last element note it so we dont keep going
						last = true;
					}
					//well thats a whole serial or path, do something with it and move the offset
					offset = i+1;


					if (device[7] != 0x00 && device[8] == 0x00) {
						//this is a serial
						found = false;
						devs = hid_enumerate(0x26f3, 0x1000);
						cur_dev = devs;
						while (cur_dev && !found) {
							handle[number_devices] = hid_open_path(cur_dev->path);
							if (handle[number_devices]) {
								// Set the hid_read() function to be non-blocking.
								hid_set_nonblocking(handle[number_devices], 1);

								if (strcmp(device, getSerial(handle[number_devices])) == 0) {
									found = true;
									path_list[paths_to_open] = new char[14];
									strcpy(path_list[paths_to_open], cur_dev->path);
									paths_to_open++;
								}
								hid_close(handle[number_devices]);
							}
							cur_dev = cur_dev->next;
						}
						hid_free_enumeration(devs);
						if (!found) {
							fprintf(stderr, "PlasmaTrim %s not found or could not be opened.\r\n", device);
						}
					} else if (device[11] != 0x00 && device[12] == 0x00) {
						//this is a path
						path_list[paths_to_open] = new char[14];
						strcpy(path_list[paths_to_open], device);
						paths_to_open++;

					}
					//reset the device serial/path
					memset(device,0x00,14);
				} else {
					//add to the serial or path
					device[i-offset] = argv[1][i];
				}
				i++;
			}
			for (i=0; i < paths_to_open; i++) {
				handle[number_devices] = hid_open_path(path_list[i]);
				if (!handle[number_devices]) {
					fprintf(stderr, "Could not open device at %s\r\n", path_list[i]);
				} else {
					number_devices++;
				}
			}
		} else if (argv[1][4] == ':') {
			// a path was given
			handle[number_devices] = hid_open_path(argv[1]);
			if (!handle[number_devices]) {
				fprintf(stderr, "Could not open device at that path!\r\n");
				return 1;
			} else {
				number_devices++;
			}
		} else {
			// a serial number was given;
			devs = hid_enumerate(0x26f3, 0x1000);
			cur_dev = devs;
			bool found = false;
			while (cur_dev && !found) {
				handle[number_devices] = hid_open_path(cur_dev->path);
				if (handle[number_devices]) {
					// Set the hid_read() function to be non-blocking.
					hid_set_nonblocking(handle[number_devices], 1);

					if (strcmp(argv[1], getSerial(handle[number_devices])) == 0) {
						found = true;
					} else {
						hid_close(handle[number_devices]);
					}
				}
				cur_dev = cur_dev->next;
			}
			hid_free_enumeration(devs);
			if (!found) {
				fprintf(stderr, "PlasmaTrim not found or could not be opened.\r\n");
				return 1;
			} else {
				number_devices++;
			}
		}

	} 
	if (number_devices > 0) {
		// we have opened at least one device
		#ifdef WIN32
			//someone needs to do Windows threadding stuffs, I do not have a Windows comp
		#else
			pthread_t thread[number_devices];
		#endif
		
		struct arg_struct args[number_devices];

		for (i=0; i < number_devices; i++) {
			args[i].argc = argc;
			args[i].argv = (char**) argv;
			args[i].handle = handle[i];
			args[i].id = i;
		}
		if (number_devices > 1 && argc == 3 && ( strcmp(argv[2], "upload") == 0 ||  strcmp(argv[2], "full_upload") == 0 ) ) {
			// they want to do an upload of multiple devics with stdin, this wont work currently so error out
			fprintf(stderr, "When loading multiple devices you must use a file, not stdin\r\n");
			return 1;
		}
		if (argc > 2 && (strcmp(argv[2], "info") != 0 && strcmp(argv[2], "download") != 0 && strcmp(argv[2], "full_download") != 0)) {
			// standard/simple commands that can just be theraded (not that they need to, but we already have multidevice communication set up so we might as well)
			// things that have multiline output should not really go here
			for (i=0; i < number_devices; i++) {
				#ifdef WIN32
					//someone needs to do Windows threadding stuffs, I do not have a Windows comp
					runCommand(&args[i]);
				#else
					pthread_create(&thread[i], NULL, runCommand, (void *) &args[i]);
				#endif
			}
			for (i=0; i < number_devices; i++) {
				#ifdef WIN32
					//someone needs to do Windows threadding stuffs, I do not have a Windows comp
				#else
					pthread_join(thread[i], NULL);
				#endif
			}
		} else {
			// commands that do not thread well.
			for (i=0; i < number_devices; i++) {
				runCommand(&args[i]);
			}
		}
	}

	hid_exit();

	return 0;
}







void *runCommand(void *arguments) {
	struct arg_struct *args = (struct arg_struct *)arguments;
	if (args -> argc == 2) {
		showInfo(args -> handle);
	} else if (args -> argc > 2) {
		if (strcmp(args -> argv[2], "info") == 0) {
			showInfo(args -> handle);
		} else if (strcmp(args -> argv[2], "serial") == 0) {
			printf("%s\r\n", getSerial(args -> handle));
		} else if (strcmp(args -> argv[2], "name") == 0) {
			if (args -> argc > 3) {
				setName(args -> handle, args -> argv[3]);
			} else {
				printf("%s\r\n", getName(args -> handle));
			}
		} else if (strcmp(args -> argv[2], "brightness") == 0) {
			if (args -> argc > 3) {
				setBrightness(args -> handle, makeBrightness(atoi(args -> argv[3])));
			} else {
				printf("%d\r\n", getBrightness(args -> handle));
			}
		} else if (strcmp(args -> argv[2], "start") == 0) {
			start(args -> handle);
		} else if (strcmp(args -> argv[2], "stop") == 0) {
			stop(args -> handle);
		} else if (strcmp(args -> argv[2], "color") == 0) {
			if (args -> argc > 4) {
				setColor(args -> handle, makeColor(args -> argv[3], true), makeBrightness(atoi(args -> argv[4])));
			} else if (args -> argc > 3) {
				setColor(args -> handle, makeColor(args -> argv[3], true), getBrightness(args -> handle));
			} else {
				printf("%s\r\n", makeHumanColor(getColor(args -> handle)));
			}
		} else if (strcmp(args -> argv[2], "download") == 0) {
			if (args -> argc > 3) {
				download(args -> handle, args -> argv[3], true);
			} else {
				download(args -> handle, "stdout", true);
			}
		} else if (strcmp(args -> argv[2], "full_download") == 0) {
			if (args -> argc > 3) {
				download(args -> handle, args -> argv[3], false);
			} else {
				download(args -> handle, "stdout", false);
			}
		} else if (strcmp(args -> argv[2], "upload") == 0) {
			if (args -> argc > 3) {
				upload(args -> handle, args -> argv[3], true, args -> id);
			} else {
				upload(args -> handle, "stdin", true, args -> id);
			}
		} else if (strcmp(args -> argv[2], "full_upload") == 0) {
			if (args -> argc > 3) {
				upload(args -> handle, args -> argv[3], false, args -> id);
			} else {
				upload(args -> handle, "stdin", false, args -> id);
			}
		} else if (strcmp(args -> argv[2], "stream") == 0) {
			if (args -> argc > 4) {
				streamData(args -> handle, args -> argv[3], makeBrightness(atoi(args -> argv[4])), args -> id);
			} else {
				streamData(args -> handle, args -> argv[3], getBrightness(args -> handle), args -> id);
			}
		} else if (strcmp(args -> argv[2], "fade") == 0) {
			if (args -> argc > 5) {
				fade(args -> handle, args -> argv[3], args -> argv[4], makeBrightness(atoi(args -> argv[5])));
			} else {
				fade(args -> handle, args -> argv[3], args -> argv[4], getBrightness(args -> handle));
			}
		} else {
			help();
		}
	}
	// close off the open device
	hid_close(args -> handle);
	return NULL;
}


void showInfo(hid_device *handle) {
	printf("Name: %s\r\n", getName(handle));
	printf("Serial: %s\r\n", getSerial(handle));
	printf("Saved Brightness: %d%%\r\n", getBrightness(handle));
	printf("Showing: %s\r\n", makeHumanColor(getColor(handle)));
}

char *getSerial(hid_device *handle) {
	unsigned char cmd[32];
	char* buf = new char[9];
	unsigned char i;
	char res = 0;

	memset(cmd,0x00,32);
	memset(buf,0x00,9);
	cmd[0] = 0x0A;

	hid_write(handle, cmd, 32);

	while (res == 0) {
		res = hid_read(handle, cmd, sizeof(cmd));
		
		delay(1);
	}

	// get the serial in the correct (reverse) order and convert it to ascii
	for (i = 2; i < 10; i+=2) {
		buf[9-(i)] = ( cmd[(i/2)] & 0x0F ) + 48;
		buf[9-(i+1)] = (( cmd[(i/2)] & 0xF0 ) >> 4) + 48;
		if (buf[9-(i)] > 0x39) {
			buf[9-(i)]+= 7;
		}
		if (buf[9-(i+1)] > 0x39) {
			buf[9-(i+1)]+= 7;
		}
	}

	buf[8] = 0x00;


	return buf;
}

char *getName(hid_device *handle) {
	unsigned char cmd[32];
	char* buf = new char[27];
	unsigned char i;
	char res = 0;

	memset(cmd,0x00,32);
	cmd[0] = 0x09;

	hid_write(handle, cmd, 32);

	while (res == 0) {
		res = hid_read(handle, cmd, sizeof(cmd));
		
		delay(1);
	}

	// shift it over by one to remove the command code
	for (i = 0; i < 26; i++) {
		buf[i] = cmd[i+1];
	}

	return buf;
}

void setName(hid_device *handle, char *name) {
	unsigned int length=strlen(name);
	unsigned char i, cmd[32];

	memset(cmd,0x00,32);
	cmd[0] = 0x08;

	if (length > 26) {
		//only 26 characters allowd
		length=26;
	}

	for (i=0; i<length; i++) {
		cmd[i+1] = name[i];
	}

	hid_write(handle, cmd, 32);
	readData(handle);

}



unsigned char getBrightness(hid_device *handle) {
	unsigned char cmd[32];
	char res = 0;

	memset(cmd,0x00,32);
	cmd[0] = 0x0C;

	hid_write(handle, cmd, 32);

	while (res == 0) {
		res = hid_read(handle, cmd, sizeof(cmd));
		
		delay(1);
	}

	return cmd[1];
}

unsigned char makeBrightness(int brightness) {
	if (brightness > 100) {
		brightness = 100;
	} else if (brightness < 1) {
		brightness = 1;
	}
	return brightness;
}

void setBrightness(hid_device *handle, unsigned char brightness) {
	unsigned char cmd[32];

	memset(cmd,0x00,32);
	cmd[0] = 0x0B;
	cmd[1] = brightness;

	hid_write(handle, cmd, 32);
	readData(handle);
}



void start(hid_device *handle) {
	unsigned char cmd[32];

	memset(cmd,0x00,32);
	cmd[0] = 0x02;

	hid_write(handle, cmd, 32);
	readData(handle);
}

void stop(hid_device *handle) {
	unsigned char cmd[32];

	memset(cmd,0x00,32);
	cmd[0] = 0x03;

	hid_write(handle, cmd, 32);
	readData(handle);
}


char *getColor(hid_device *handle) {
	unsigned char cmd[32];
	char* buf = new char[49];
	unsigned char i;
	char res = 0;

	memset(cmd,0x00,32);
	memset(buf,0x00,49);
	cmd[0] = 0x01;

	hid_write(handle, cmd, 32);

	while (res == 0) {
		res = hid_read(handle, cmd, sizeof(cmd));
		
		delay(1);
	}

	// get the color and convert it
	for (i = 0; i < 24; i++) {
		buf[(i*2)] = ( cmd[i+1] >> 4 ) +48;
		buf[(i*2)+1] = ( cmd[i+1] & 0x0F ) +48;

		if (buf[(i*2)] > 0x39) {
			buf[(i*2)]+= 7;
		}
		if (buf[(i*2)+1] > 0x39) {
			buf[(i*2)+1]+= 7;
		}
	}


	return buf;
}

char *makeHumanColor(char* data) {
	unsigned char i, j, s = 0;

	char* color = new char[56];
	memset(color,0x00,56);

	for (i=0; i<48; i+=6) {
		for (j=0; j<6; j++) {
			color[i+j+s] = data[i+j];
		}
		if (i+j < 48) {
			color[i+j+s] = ' ';
		}
		s++;
	}
	return color;
}

unsigned char *makeColor(char* color, bool full_length) {
	unsigned char i, j, offset = 0;
	char* color_standardized = new char[49];
	memset(color_standardized,0x00,49);

	unsigned char* buf = new unsigned char[26];
	memset(buf,0x00,26);

	unsigned int msb, lsb;

	int length=strlen(color);

	// strip out the spaces rather crudly
	for (i=0; (i+offset) <length; i++) {
		while (color[i+offset] == ' ') {
			offset++;
		}
		color[i] = color[i+offset];
	}

	length -= offset;
	if (length < 6) {
		//we got a 3 character length only
		if (full_length) {
			//and it needs to be 8*6 characters
			for (i=0; i<48; i+=6) {
				for (j=0; j<6; j+=2) {
					color_standardized[i+j]=color[j/2];
					color_standardized[i+j+1]=color[j/2];
				}
			}
		} else {
			// and it needs to be 8*3 characters
			for (i=0; i<24; i+=3) {
				for (j=0; j<3; j++) {
					color_standardized[i+j]=color[j];
				}
			}
		}
	} else if (length < 24) {
		// we got a 6 char length only
		if (full_length) {
			// and it needs to be 8*6 characters
			for (i=0; i<48; i+=6) {
				for (j=0; j<6; j++) {
					color_standardized[i+j]=color[j];
				}
			}
		} else {
			// and it needs to be 8*3 characters
			for (i=0; i<24; i+=3) {
				for (j=0; j<6; j+=2) {
					//so just chop off the least significant bit on each color
					color_standardized[i+(j/2)]=color[j];
				}
			}
		}
	} else if (length < 48) {
		// we got a 3 char length for each LED
		if (full_length) {
			// and it needs to be 8*6 characters
			for (i=0; i<24; i++) {
				color_standardized[i*2]=color[i];
				color_standardized[(i*2)+1]=color[i];
			}
		} else {
			// and it needs to be 8*3 characters ... which it already is
			for (i=0; i<24; i++) {
				color_standardized[i]=color[i];
			}
		}
	} else {
		// we got a 6 char length for each LED
		if (full_length) {
			// and it needs to be 8*6 characters ... which it already is
			for (i=0; i<48; i++) {
				color_standardized[i]=color[i];
			}
		} else {
			// and it needs to be 8*3 characters
			//so just chop off the least significant bit on each color
			for (i=0; i<24; i++) {
				color_standardized[i]=color[(i*2)];
			}
		}
	}


	for (i=0; i<50; i+=2) {

		msb = color_standardized[i];
		if (msb > 70) {
			msb-=87;
		} else if (msb > 57) {
			msb-=55;
		} else if (msb > 9) {
			msb-=48;
		}
		msb = msb << 4;

		lsb = color_standardized[i+1];
		if (lsb > 70) {
			lsb-=87;
		} else if (lsb > 57) {
			lsb-=55;
		} else if (lsb > 9) {
			lsb-=48;
		}
		buf[i/2] = msb + lsb  ;

	}
	
	return buf;
}

void setColor(hid_device *handle, unsigned char *color, unsigned char brightness) {
	unsigned char cmd[32];
	unsigned char i;

	memset(cmd,0x00,32);
	cmd[0] = 0x00;

	for (i=0; i < 24; i++) {
		cmd[i+1] = color[i];
	}
	cmd[25] = brightness;

	hid_write(handle, cmd, 32);
	readData(handle);
}

void download(hid_device *handle, char *filename, bool blank) {

	unsigned char i, j, slotsToGet = 76, hold, fade;
	unsigned char activeSlots=getActiveSlots(handle);
	int length=strlen(filename);
	char* extension = new char[7];
	memset(extension,0x00,7);

	unsigned char cmd[32];
	char res = 0;

	if (blank) {
		slotsToGet = activeSlots;
	}

	FILE *file;

	if (strcmp(filename, "stdout") != 0) {
		if (length > 6) {
			for (i=(length - 6); i < length; i++) {
				extension[i-(length-6)] = filename[i];
			}
		}
		if (strcmp(extension, ".ptSeq") == 0) {
			// the filename ended in a .ptSeq, just use it
			file = fopen(filename,"w+");
		} else {
			// append the .ptSeq extension
			file = fopen(strcat(filename, ".ptSeq"),"w+");
		}
	} else {
		file = stdout;
	}
	fprintf(file,"PlasmaTrim RGB-8 Sequence\r\nVersion: Simple Sequence Format\r\nActive Slots: %02d\r\n", activeSlots);


	for (i=0; i < slotsToGet; i++) {
		res = 0;
		memset(cmd,0x00,32);
		cmd[0] = 0x07;
		cmd[1] = i;

		hid_write(handle, cmd, 32);

		while (res == 0) {
			res = hid_read(handle, cmd, sizeof(cmd));
			
			delay(1);
		}

		hold = (cmd[14] >> 4) + 48;
		if (hold > 0x39) {
			hold+= 7;
		}

		fade = (cmd[14] & 0xF) + 48;
		if (fade > 0x39) {
			fade+= 7;
		}

		fprintf(file, "slot %02d %c %c - ", i, hold, fade);
		for (j=0; j<12; j++) {
			fprintf(file, "%02X", cmd[j+2] );
		}
		fprintf(file, "\r\n");
	} for (i=slotsToGet; i < 76; i++) {
		fprintf(file, "slot %02d 0 0 - 000000000000000000000000\r\n", i);
	}
	fprintf(file, "\r\n");
	if (strcmp(filename, "stdout") != 0) {
		fclose(file);
	}
}


void upload(hid_device *handle, char *filename, bool blank, unsigned char id) {
	unsigned char i, lineNumber = 0, activeSlots, slotsToLoad = 76, activeDevices = 0, deviceNum = 1;
	bool knownType = true, multiDevice = false;
	unsigned char *rawColor;

	unsigned char cmd[32];

	char* color = new char[25];

	FILE *file;
	if (strcmp(filename, "stdin") != 0) {
		file = fopen ( filename, "r" );
	} else {
		file = stdin;
	}
	if ( file != NULL ) {


		stop(handle);

		memset(color,0x00,25);
		memset(color,'0',3);
		setColor(handle, makeColor(color, true), 1); //set it to black because were nice

		char line [ 560 ]; //553 = 20 devices max on a line for this read
		while ( knownType && fgets ( line, sizeof line, file ) != NULL ) {
			lineNumber++;
			if (lineNumber == 2) {
				if (!( strcmp (line, "Version: Simple Sequence Format\r\n") == 0 || strcmp (line, "Version: Multiple Sequence Format\r\n") == 0)) {
					knownType = false;
					fprintf(stderr, "Unknown file format.\r\n");
					return;
				} else if (strcmp (line, "Version: Multiple Sequence Format\r\n") == 0) {
					multiDevice = true;
				}
			} else if ( lineNumber == 3 ) {
				if (multiDevice) {
					activeDevices =  ((line[35] - 48) * 10) + line[36] - 48;
					deviceNum = id + 1;
					while (deviceNum > activeDevices) {
						deviceNum-=activeDevices;
					}
				}
				activeSlots = ((line[14] - 48) * 10) + line[15] - 48;
				if (blank) {
					slotsToLoad = activeSlots;
				}
			} else if ( lineNumber > 3 && lineNumber < 4+slotsToLoad) {
				memset(color,0x00,25);
				memset(cmd,0x00,32);
				cmd[0] = 0x06;
				cmd[1] = lineNumber - 4;
				for (i=14; i<38; i++) {
					color[i-14] = line[i+(27*(deviceNum-1))];
				}
				rawColor = makeColor(color, false);
				for (i=0; i<12; i++) {
					cmd[i+2] = rawColor[i];
				}
				cmd[14] = ((line[8] - 0x30) << 4) + line[10] - 0x30;

				hid_write(handle, cmd, 32);
				readData(handle);

			}
		}
		if (strcmp(filename, "stdin") != 0) {
			fclose ( file );
		}
		setActiveSlots(handle, activeSlots);
		start(handle);
	} else {
		printf("%s did not open.\r\n", filename );
	}
}

void streamData(hid_device *handle, char *filename, unsigned char brightness, unsigned char id) {
	unsigned char i, lineNumber = 0, activeSlots = 0, activeDevices = 0, loopStart = 0, deviceNum = 1, offset = 0;
	bool knownType = true, multiDevice = false;



	FILE *file;
	file = fopen ( filename, "r" );
	if ( file != NULL ) {

		char line [ 560 ]; //553 = 20 devices max on a line for this read
		while ( activeSlots == 0 && knownType && fgets ( line, sizeof line, file ) != NULL ) {
			lineNumber++;
			if (lineNumber == 2) {
				if (!( strcmp (line, "Version: Simple Sequence Format\r\n") == 0 || strcmp (line, "Version: Multiple Sequence Format\r\n") == 0)) {
					knownType = false;
					fprintf(stderr, "Unknown file format.\r\n");
					return;
				} else if (strcmp (line, "Version: Multiple Sequence Format\r\n") == 0) {
					multiDevice = true;
				}
			} else if ( lineNumber == 3 ) {
				if (multiDevice) {
					i = 80;
					while (line[i] > 47 && line[i] < 58) {
						activeSlots*=10;
						activeSlots+=line[i] - 48;
						i++;
					}
					activeDevices =  ((line[35] - 48) * 10) + line[36] - 48;
					deviceNum = id + 1;
					while (deviceNum > activeDevices) {
						deviceNum-=activeDevices;
					}
					loopStart =  (((line[52] - 48) * 10) + line[53] - 48) -1;

				} else {
					activeSlots = ((line[14] - 48) * 10) + line[15] - 48;
				}
			}
		}

		char* fadeTime[activeSlots];
		char* holdTime[activeSlots];
		char* slot[activeSlots];

		for (i=0; i < activeSlots; i++) {
			slot[i] = new char[25];
			memset(slot[i],0x00,25);

			holdTime[i] = new char[2];
			memset(holdTime[i],0x00,2);

			fadeTime[i] = new char[2];
			memset(fadeTime[i],0x00,2);
		}


		while ( knownType && fgets ( line, sizeof line, file ) != NULL ) {
			lineNumber++;
			if (lineNumber == 104) {
				offset = 1; //once we hit slot 100 we need to account for the extra digit in the slot #
			}
			if ( lineNumber > 3 && lineNumber < 4+activeSlots) {
				for (i=14; i<38; i++) {
					slot[lineNumber-4][i-14] = line[i+(27*(deviceNum-1))+offset];
				}
				//save the hold and fade time, but they are fading out and holding after, so move it by one
				if (lineNumber-3 == activeSlots) {
					fadeTime[0][0] = line[10+offset];
					holdTime[0][0] = line[8+offset];
				} else {
					fadeTime[lineNumber-3][0] = line[10+offset];
					holdTime[lineNumber-3][0] = line[8+offset];
				}

			}
		}
		
		if (knownType && activeSlots > 0) {
			for (i=0; i < loopStart; i++) {
				hold(holdTime[i]);
				fade(handle, makeHumanColor(slot[i]), fadeTime[i], brightness);
			}
			while (1) {
				running_threads++;
				for (i=loopStart; i < activeSlots; i++) {
					hold(holdTime[i]);
					fade(handle, makeHumanColor(slot[i]), fadeTime[i], brightness);
				}
				running_threads--;
				while (running_threads > 0) {
					// wait for the other threads to catch up (some variance happens due to lag in the USB transfer)
					delay(1);
				}
				// make sure they all notice that it reached 0
				delay(5);
			}
		}

		fclose ( file );

	} else {
		printf("%s did not open.\r\n", filename );
	}
}


unsigned char getActiveSlots(hid_device *handle) {
	unsigned char cmd[32];
	char res = 0;

	memset(cmd,0x00,32);
	cmd[0] = 0x05;

	hid_write(handle, cmd, 32);

	while (res == 0) {
		res = hid_read(handle, cmd, sizeof(cmd));
		
		delay(1);
	}

	return cmd[1];
}

void setActiveSlots(hid_device *handle, unsigned char slots) {
	unsigned char cmd[32];

	if (slots > 76) {
		slots = 76;
	} else if (slots < 1) {
		slots = 1;
	}

	memset(cmd,0x00,32);
	cmd[0] = 0x04;
	cmd[1] = slots;

	hid_write(handle, cmd, 32);
	readData(handle);
}


void fade(hid_device *handle, char *userColor, char *fadeTime, unsigned char brightness) {
	// the delay times are pretty rough, I cant seem to get enough accuracy with them for the 5-a, everything past c was never tested for accuracy.
	unsigned char i, j, x, y;
	unsigned char* origColor = makeColor(getColor(handle), true);
	unsigned char* newColor = makeColor(userColor, true);
	unsigned char steps = 255;
	unsigned int delayTime = 0;

	if (strcmp(fadeTime, "0") == 0) {
		steps = 1;
	} else if (strcmp(fadeTime, "1") == 0) {
		steps = 10;
	} else if (strcmp(fadeTime, "2") == 0) {
		steps = 30;
	} else if (strcmp(fadeTime, "3") == 0) {
		steps = 61;
	} else if (strcmp(fadeTime, "4") == 0) {
		steps = 122;
	} else if (strcmp(fadeTime, "5") == 0) {
		delayTime = 3;
	} else if (strcmp(fadeTime, "6") == 0) {
		delayTime = 11;
	} else if (strcmp(fadeTime, "7") == 0) {
		delayTime = 28;
	} else if (strcmp(fadeTime, "8") == 0) {
		delayTime = 50;
	} else if (strcmp(fadeTime, "9") == 0) {
		delayTime = 110;
	} else if (strcmp(fadeTime, "a") == 0 || strcmp(fadeTime, "A") == 0) {
		delayTime = 225;
	} else if (strcmp(fadeTime, "b") == 0 || strcmp(fadeTime, "B") == 0) {
		delayTime = 580;
	} else if (strcmp(fadeTime, "c") == 0 || strcmp(fadeTime, "C") == 0) {
		delayTime = 1200;
	} else if (strcmp(fadeTime, "d") == 0 || strcmp(fadeTime, "D") == 0) {
		delayTime = 2400;
	} else if (strcmp(fadeTime, "e") == 0 || strcmp(fadeTime, "E") == 0) {
		delayTime = 3600;
	} else if (strcmp(fadeTime, "f") == 0 || strcmp(fadeTime, "F") == 0) {
		delayTime = 7200;
	}
	unsigned char* colorTable[steps];
	double colorDiff[24];

	for (j=0; j<24; j++) {
		x = origColor[j];
		y = newColor[j];
		colorDiff[j] = (double) (x-y) / steps;

	}
	for (i=0; i<steps-1; i++) {
		colorTable[i] = new unsigned char[25];
		for (j=0; j<24; j++) {
			colorTable[i][j] = origColor[j] - (colorDiff[j] * (i+1));
		}
	}
	colorTable[steps-1] = new unsigned char[25];
	for (j=0; j<24; j++) {
		colorTable[i][j] = newColor[j];
	}
	for (i=0; i<steps; i++) {
		setColor(handle, colorTable[i], brightness);
		
		delay(delayTime);
	}
}

void hold(char *holdTime) {

	if (strcmp(holdTime, "1") == 0) {
		delay(100);
	} else if (strcmp(holdTime, "2") == 0) {
		delay(250);
	} else if (strcmp(holdTime, "3") == 0) {
		delay(500);
	} else if (strcmp(holdTime, "4") == 0) {
		delay(1000);
	} else if (strcmp(holdTime, "5") == 0) {
		delay(2500);
	} else if (strcmp(holdTime, "6") == 0) {
		delay(5000);
	} else if (strcmp(holdTime, "7") == 0) {
		delay(19000);
	} else if (strcmp(holdTime, "8") == 0) {
		delay(15000);
	} else if (strcmp(holdTime, "9") == 0) {
		delay(30000);
	} else if (strcmp(holdTime, "a") == 0 || strcmp(holdTime, "A") == 0) {
		delay(60000);
	} else if (strcmp(holdTime, "b") == 0 || strcmp(holdTime, "B") == 0) {
		delay(150000);
	} else if (strcmp(holdTime, "c") == 0 || strcmp(holdTime, "C") == 0) {
		delay(300000);
	} else if (strcmp(holdTime, "d") == 0 || strcmp(holdTime, "D") == 0) {
		delay(600000);
	} else if (strcmp(holdTime, "e") == 0 || strcmp(holdTime, "E") == 0) {
		delay(90000);
	} else if (strcmp(holdTime, "f") == 0 || strcmp(holdTime, "F") == 0) {
		delay(1800000);
	}
}





void readData(hid_device *handle) {
	//after every write we need to read the data from the buffer to prevent errors, but only sometimes we want the data, this reads the data we dont care about
	char res;
	unsigned char cmd[32];
	memset(cmd,0x00,32);

	while (res == 0) {
		res = hid_read(handle, cmd, sizeof(cmd));
		
		delay(1);
	}
}

void delay(unsigned int ms) {
	#ifdef WIN32
		Sleep(ms);
	#else
		usleep(ms*1000);
	#endif
}


void help() {
	printf("Version: 0.2.0\t From: Oct 24 2012\r\n");
	printf("Codename: \"And then there were two\"\r\n");
	printf("\r\n");
	printf("Some help for you:\r\n");
	printf("When calling multiple devices you must have either the serials or paths space separated and wrapped in quotes\r\n");
	printf("	ie \"00B12345 00B12346\" will work\r\n");
	printf("	the keywork 'all' can also be used to call all devices\r\n");
	printf("\r\n");
	printf("\r\n");
	printf("Command Overview:\r\n");
	printf("\r\n");
	printf("ptrim\r\n");
	printf("	list the PlasmaTrims that are hooked up to the system.\r\n");
	printf("ptrim (serial|path|multiple) info\r\n");
	printf("	print generic information about the device with the serial or at the path (basically the same as calling it w/o arguments)\r\n");
	printf("ptrim (serial|path|multiple) serial\r\n");
	printf("	print the serial number (rather pointless if you use serial numbers to call the script, but could be useful if you used the path)\r\n");
	printf("ptrim (serial|path|multiple) name [new name]\r\n");
	printf("	print the devices name or set it if a new name is give\r\n");
	printf("	*NOTE* this will write to the non-volatile memory when you set the name, do not run this hundreds of thousands of times\r\n");
	printf("\r\n");
	printf("ptrim (serial|path|multiple) brightness [new brightness]\r\n");
	printf("	print or set the brightness\r\n");
	printf("	*NOTE* this will write to the non-volatile memory when you set the brightness, do not run this hundreds of thousands of times\r\n");
	printf("\r\n");
	printf("ptrim (serial|path|multiple) (start|stop)\r\n");
	printf("	start or stop the sequence loaded to the device\r\n");
	printf("\r\n");
	printf("ptrim (serial|path|multiple) color [new color] [temporary brightness]\r\n");
	printf("	show or set the colors the PlasmaTrim is displaying\r\n");
	printf("	if a brightness is not given it will use the saved brightness, which could be different than the current brightness\r\n");
	printf("	the new color string must be wrapped in quotes if it has spaces.\r\n");
	printf("	the color string can be 3 or 6 characters - which will set all the LEDs to the same color (ie: 'FFF' and 'FFFFFF' are the same)\r\n");
	printf("		it can also be 8 sets of 3 or 6 characters - which will set every LED to a different color (ex: 'F00 0F0 00F....')\r\n");
	printf("	the new brightness is NOT saved to non-volatile memory\r\n");
	printf("\r\n");
	printf("ptrim (serial|path|multiple) fade (new color) (fade time) temporary [brightness]\r\n");
	printf("	fade the PlasmaTrim to the new color over some time, the time is an estimate and uses the same numbering as the windows application and ptSeq files.\r\n");
	printf("		0 => instant, almost same as color \r\n");
	printf("		1 => 1/10 sec; 2 => 1/4; 3 => 1/2; 4 => 1 sec; 5 => 2.5; 6 => 5; 7 => 10; 8 => 15; 9 => 30 sec\r\n");
	printf("		a => 1 min; b => 2.5; c => 5; d => 10; e => 15; f => 30 min\r\n");
	printf("\r\n");
	printf("ptrim (serial|path|multiple) download [filename]\r\n");
	printf("	get the currently programmed sequence from the device (it will stop at the last active element then report black with no hold or fade for the rest)\r\n");
	printf("	if no file is give it will print to stdout\r\n");
	printf("ptrim (serial|path|multiple) full_download [filename]\r\n");
	printf("	same as download except it will not stop reading at the last active slot (useful if you have stored some unfinished work after the last element)\r\n");
	printf("\r\n");
	printf("ptrim (serial|path|multiple) upload [filename]\r\n");
	printf("	program the PlasmaTrim with the sequence file, if no file is given it will use stdin (you can pipe sequence files to it)\r\n");
	printf("	if no file is given it will not overwrite the slots after the last active element (makes uploads faster)\r\n");
	printf("	*NOTE* this will write to the non-volatile memory, do not run this hundreds of thousands of times\r\n");
	printf("ptrim (serial|path|multiple) full_upload [filename]\r\n");
	printf("	same as upload except it will not stop writing at the last active slot (useful if you would like to blackout the inactive elements, but it takes longer)\r\n");
	printf("	*NOTE* this will write to the non-volatile memory, do not run this hundreds of thousands of times\r\n");
	printf("\r\n");
	printf("ptrim (serial|path|multiple) stream (filename) [temporary brightness]\r\n");
	printf("	send the sequence in the file to the device(s) without overwriting the current programming, optionally at a brightness level\r\n");
	printf("	this is slightly slower than if the sequence ran on the device from programming, and it keeps in sync by waiting for them all to finish (USB has some lag at times)\r\n");
	printf("	but the differences in timing between multiple units are minimial, you will probably never notice them since the units will always start together\r\n");
	printf("\r\n");
}