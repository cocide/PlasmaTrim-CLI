/*******************************************************
PlasmaTrim CLI goodness v0.1.0
Andrew Toy Oct 23 2012

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
#ifdef _WIN32
	#include <windows.h>
#else
	#include <unistd.h>
#endif


#ifdef WIN32
	UNREFERENCED_PARAMETER(argc);
	UNREFERENCED_PARAMETER(argv);
#endif




void showInfo(hid_device *handle);
char *getSerial(hid_device *handle);
char *getName(hid_device *handle);
void setName(hid_device *handle, char *name);

int getBrightness(hid_device *handle);
int makeBrightness(int brightness);
void setBrightness(hid_device *handle, int brightness);


void start(hid_device *handle);
void stop(hid_device *handle);


char *getColor(hid_device *handle);
char *makeHumanColor(char* data);
unsigned char *makeColor(char *color, bool full_length);
void setColor(hid_device *handle, unsigned char *color, int brightness);

void download(hid_device *handle, char *filename, bool blank);
void upload(hid_device *handle, char *filename, bool blank);

int getActiveSlots(hid_device *handle);
void setActiveSlots(hid_device *handle, int slots);

void fade(hid_device *handle, char *userColor, char *fadeTime);


void help();


int main(int argc, char* argv[]) {
	hid_device *handle;

	struct hid_device_info *devs, *cur_dev;
	
	if (argc == 1) {
		//no arguements were given so find/list devices and info
		int number = 0;
		devs = hid_enumerate(0x26f3, 0x1000);
		cur_dev = devs;	
		while (cur_dev) {
			handle = hid_open_path(cur_dev->path);
			if (handle) {
				number++;

				// Set the hid_read() function to be non-blocking.
				hid_set_nonblocking(handle, 1);

				showInfo(handle);
				printf("Path: %s\n", cur_dev->path);

				hid_close(handle);
			}
			printf("\n");

			cur_dev = cur_dev->next;
		}
		hid_free_enumeration(devs);
		if (number == 1) {
			//use a bit better english for only one device
			printf("1 PlasmaTrim was found.\n");
		} else {
			printf("%d PlasmaTrims were found.\n", number);
		}
		return 0;

	} else {
		if (strcmp(argv[1], "help") == 0) {
			help();
			return 0;
		} else if (argv[1][4] == ':') {
			// a path was given

			handle = hid_open_path(argv[1]);
			if (!handle) {
				printf("Could not open device at that path!\n");
				return 1;
			}
			

		} else {
			// a serial number was given;
			devs = hid_enumerate(0x26f3, 0x1000);
			cur_dev = devs;
			bool found = false;
			while (cur_dev && !found) {
				handle = hid_open_path(cur_dev->path);
				if (handle) {
					// Set the hid_read() function to be non-blocking.
					hid_set_nonblocking(handle, 1);

					if (strcmp(argv[1], getSerial(handle)) == 0) {
						found = true;
					} else {
						hid_close(handle);
					}
				}
				cur_dev = cur_dev->next;
			}
			hid_free_enumeration(devs);
			if (!found) {
				printf("PlasmaTrim not found or could not be opened.\n");
				return 1;
			}
		}

	} 
	if (handle && argc > 2) {
		if (strcmp(argv[2], "info") == 0) {
			showInfo(handle);
		} else if (strcmp(argv[2], "serial") == 0) {
			printf("%s\n", getSerial(handle));
		} else if (strcmp(argv[2], "name") == 0) {
			if (argc > 3) {
				setName(handle, argv[3]);
			} else {
				printf("%s\n", getName(handle));
			}
		} else if (strcmp(argv[2], "brightness") == 0) {
			if (argc > 3) {
				setBrightness(handle, makeBrightness(atoi(argv[3])));
			} else {
				printf("%d\n", getBrightness(handle));
			}
		} else if (strcmp(argv[2], "start") == 0) {
			start(handle);
		} else if (strcmp(argv[2], "stop") == 0) {
			stop(handle);
		} else if (strcmp(argv[2], "color") == 0) {
			if (argc > 4) {
				setColor(handle, makeColor(argv[3], true), makeBrightness(atoi(argv[4])));
			} else if (argc > 3) {
				setColor(handle, makeColor(argv[3], true), getBrightness(handle));
			} else {
				printf("%s\n", makeHumanColor(getColor(handle)));
			}
		} else if (strcmp(argv[2], "download") == 0) {
			if (argc > 3) {
				download(handle, argv[3], true);
			} else {
				download(handle, "stdout", true);
			}
		} else if (strcmp(argv[2], "full_download") == 0) {
			if (argc > 3) {
				download(handle, argv[3], false);
			} else {
				download(handle, "stdout", false);
			}
		} else if (strcmp(argv[2], "upload") == 0) {
			if (argc > 3) {
				upload(handle, argv[3], true);
			} else {
				upload(handle, "stdin", true);
			}
		} else if (strcmp(argv[2], "full_upload") == 0) {
			if (argc > 3) {
				upload(handle, argv[3], false);
			} else {
				upload(handle, "stdin", false);
			}
		} else if (strcmp(argv[2], "fade") == 0) {
			fade(handle, argv[3], argv[4]);
		} else {
			help();
		}

		hid_close(handle);
	}

	hid_exit();

	return 0;
}










void showInfo(hid_device *handle) {
	printf("Name: %s\n", getName(handle));
	printf("Serial: %s\n", getSerial(handle));
	printf("Saved Brightness: %d%%\n", getBrightness(handle));
	printf("Showing: %s\n", makeHumanColor(getColor(handle)));
}

char *getSerial(hid_device *handle) {
	unsigned char cmd[32];
	char* buf = new char[9];
	int i, res = 0;

	memset(cmd,0x00,32);
	memset(buf,0x00,9);
	cmd[0] = 0x0A;

	hid_write(handle, cmd, 32);

	while (res == 0) {
		res = hid_read(handle, cmd, sizeof(cmd));
		
		#ifdef WIN32
			Sleep(1);
		#else
			usleep(1*1000);
		#endif
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
	int i, res = 0;

	memset(cmd,0x00,32);
	cmd[0] = 0x09;

	hid_write(handle, cmd, 32);

	while (res == 0) {
		res = hid_read(handle, cmd, sizeof(cmd));
		
		#ifdef WIN32
			Sleep(1);
		#else
			usleep(1*1000);
		#endif
	}

	// shift it over by one to remove the command code
	for (i = 0; i < 26; i++) {
		buf[i] = cmd[i+1];
	}

	return buf;
}

void setName(hid_device *handle, char *name) {
	int length=strlen(name);
	int i, res = 0;
	unsigned char cmd[32];

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

	while (res == 0) {
		res = hid_read(handle, cmd, sizeof(cmd));
		
		#ifdef WIN32
			Sleep(1);
		#else
			usleep(1*1000);
		#endif
	}

}



int getBrightness(hid_device *handle) {
	unsigned char cmd[32];
	int res = 0;

	memset(cmd,0x00,32);
	cmd[0] = 0x0C;

	hid_write(handle, cmd, 32);

	while (res == 0) {
		res = hid_read(handle, cmd, sizeof(cmd));
		
		#ifdef WIN32
			Sleep(1);
		#else
			usleep(1*1000);
		#endif
	}

	return cmd[1];
}

int makeBrightness(int brightness) {
	if (brightness > 100) {
		brightness = 100;
	} else if (brightness < 1) {
		brightness = 1;
	}
	return brightness;
}

void setBrightness(hid_device *handle, int brightness) {
	unsigned char cmd[32];
	int res=0;

	memset(cmd,0x00,32);
	cmd[0] = 0x0B;
	cmd[1] = brightness;

	hid_write(handle, cmd, 32);

	while (res == 0) {
		res = hid_read(handle, cmd, sizeof(cmd));
		
		#ifdef WIN32
			Sleep(1);
		#else
			usleep(1*1000);
		#endif
	}
}



void start(hid_device *handle) {
	unsigned char cmd[32];
	int res=0;

	memset(cmd,0x00,32);
	cmd[0] = 0x02;

	hid_write(handle, cmd, 32);

	while (res == 0) {
		res = hid_read(handle, cmd, sizeof(cmd));
		
		#ifdef WIN32
			Sleep(1);
		#else
			usleep(1*1000);
		#endif
	}
}

void stop(hid_device *handle) {
	unsigned char cmd[32];
	int res=0;

	memset(cmd,0x00,32);
	cmd[0] = 0x03;

	hid_write(handle, cmd, 32);

	while (res == 0) {
		res = hid_read(handle, cmd, sizeof(cmd));
		
		#ifdef WIN32
			Sleep(1);
		#else
			usleep(1*1000);
		#endif
	}
}


char *getColor(hid_device *handle) {
	unsigned char cmd[32];
	char* buf = new char[49];
	int i, res = 0;

	memset(cmd,0x00,32);
	memset(buf,0x00,49);
	cmd[0] = 0x01;

	hid_write(handle, cmd, 32);

	while (res == 0) {
		res = hid_read(handle, cmd, sizeof(cmd));
		
		#ifdef WIN32
			Sleep(1);
		#else
			usleep(1*1000);
		#endif
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
	int i, j, s = 0;

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
	int i, j, offset = 0;
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

void setColor(hid_device *handle, unsigned char *color, int brightness) {
	unsigned char cmd[32];
	int i, res=0;

	memset(cmd,0x00,32);
	cmd[0] = 0x00;

	for (i=0; i < 24; i++) {
		cmd[i+1] = color[i];
	}
	cmd[25] = brightness;

	hid_write(handle, cmd, 32);

	while (res == 0) {
		res = hid_read(handle, cmd, sizeof(cmd));
		
		#ifdef WIN32
			Sleep(1);
		#else
			usleep(1*1000);
		#endif
	}
}

void download(hid_device *handle, char *filename, bool blank) {

	int i, j, slotsToGet = 76;
	int hold, fade;
	int activeSlots=getActiveSlots(handle);
	int length=strlen(filename);
	char* extension = new char[7];
	memset(extension,0x00,7);

	unsigned char cmd[32];
	int res = 0;

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
			
			#ifdef WIN32
				Sleep(1);
			#else
				usleep(1*1000);
			#endif
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


void upload(hid_device *handle, char *filename, bool blank) {
	int i;
	int lineNumber = 0, activeSlots, slotsToLoad = 76;
	bool knownType = true;
	unsigned char *rawColor;

	unsigned char cmd[32];
	int res=0;

	char* color = new char[25];

	FILE *file;
	if (strcmp(filename, "stdin") != 0) {
		file = fopen ( filename, "r" );
	} else {
		file = stdin;
	}
	if ( file != NULL ) {
		stop(handle);
		char line [ 256 ];
		while ( knownType && fgets ( line, sizeof line, file ) != NULL ) {
			lineNumber++;
			if (lineNumber == 2 && strcmp (line, "Version: Simple Sequence Format\r\n") != 0) {
				knownType = false;
				printf("Unknown file format.\n");
			} else if ( lineNumber == 3 ) {
				activeSlots = ((line[14] - 48) * 10) + line[15] - 48;
				if (blank) {
					slotsToLoad = activeSlots;
				}
			} else if ( lineNumber > 3 && lineNumber < 4+slotsToLoad) {
				res = 0;
				memset(color,0x00,25);
				memset(cmd,0x00,32);
				cmd[0] = 0x06;
				cmd[1] = lineNumber - 4;
				for (i=14; i<38; i++) {
					color[i-14] = line[i];
				}
				rawColor = makeColor(color, false);
				for (i=0; i<12; i++) {
					cmd[i+2] = rawColor[i];
				}
				cmd[14] = ((line[8] - 0x30) << 4) + line[10] - 0x30;

				hid_write(handle, cmd, 32);

				while (res == 0) {
					res = hid_read(handle, cmd, sizeof(cmd));
					
					#ifdef WIN32
						Sleep(1);
					#else
						usleep(1*1000);
					#endif
				}

			}
		}
		if (strcmp(filename, "stdin") != 0) {
			fclose ( file );
		}
		setActiveSlots(handle, activeSlots);
		start(handle);
	} else {
		printf("%s did not open.\n", filename );
	}
}


int getActiveSlots(hid_device *handle) {
	unsigned char cmd[32];
	int res = 0;

	memset(cmd,0x00,32);
	cmd[0] = 0x05;

	hid_write(handle, cmd, 32);

	while (res == 0) {
		res = hid_read(handle, cmd, sizeof(cmd));
		
		#ifdef WIN32
			Sleep(1);
		#else
			usleep(1*1000);
		#endif
	}

	return cmd[1];
}

void setActiveSlots(hid_device *handle, int slots) {
	unsigned char cmd[32];
	int res=0;

	if (slots > 76) {
		slots = 76;
	} else if (slots < 1) {
		slots = 1;
	}

	memset(cmd,0x00,32);
	cmd[0] = 0x04;
	cmd[1] = slots;

	hid_write(handle, cmd, 32);

	while (res == 0) {
		res = hid_read(handle, cmd, sizeof(cmd));
		
		#ifdef WIN32
			Sleep(1);
		#else
			usleep(1*1000);
		#endif
	}
}


void fade(hid_device *handle, char *userColor, char *fadeTime) {
	// the delay times are pretty rough, I cant seem to get enough accuracy with them for the 5-a, everything past c was never tested for accuracy.
	int i, j;
	unsigned char x, y;
	unsigned char* origColor = makeColor(getColor(handle), true);
	unsigned char* newColor = makeColor(userColor, true);
	int steps = 255, delay = 0;
	if (strcmp(fadeTime, "0") == 0) {
		steps = 2;
	} else if (strcmp(fadeTime, "1") == 0) {
		steps = 12;
	} else if (strcmp(fadeTime, "2") == 0) {
		steps = 30;
	} else if (strcmp(fadeTime, "3") == 0) {
		steps = 61;
	} else if (strcmp(fadeTime, "4") == 0) {
		steps = 122;
	} else if (strcmp(fadeTime, "5") == 0) {
		delay = 3;
	} else if (strcmp(fadeTime, "6") == 0) {
		delay = 11;
	} else if (strcmp(fadeTime, "7") == 0) {
		delay = 28;
	} else if (strcmp(fadeTime, "8") == 0) {
		delay = 50;
	} else if (strcmp(fadeTime, "9") == 0) {
		delay = 110;
	} else if (strcmp(fadeTime, "a") == 0 || strcmp(fadeTime, "A") == 0) {
		delay = 225;
	} else if (strcmp(fadeTime, "b") == 0 || strcmp(fadeTime, "B") == 0) {
		delay = 580;
	} else if (strcmp(fadeTime, "c") == 0 || strcmp(fadeTime, "C") == 0) {
		delay = 1200;
	} else if (strcmp(fadeTime, "d") == 0 || strcmp(fadeTime, "D") == 0) {
		delay = 2400;
	} else if (strcmp(fadeTime, "e") == 0 || strcmp(fadeTime, "E") == 0) {
		delay = 3600;
	} else if (strcmp(fadeTime, "f") == 0 || strcmp(fadeTime, "F") == 0) {
		delay = 7200;
	}
	unsigned char* colorTable[steps];
	double colorDiff[24];

	for (j=0; j<24; j++) {
		x = origColor[j];
		y = newColor[j];
		colorDiff[j] = (x-y) / steps;

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
		setColor(handle, colorTable[i], 1);
		
		#ifdef WIN32
			Sleep(delay);
		#else
			usleep(delay*1000);
		#endif
	}

}










void help() {
	printf("Some help for you:\n\n");
	printf("ptrim\n");
	printf("	list the PlasmaTrims that are hooked up to the system.\n");
	printf("ptrim (serial|path) info\n");
	printf("	print generic information about the device with the serial or at the path (basically the same as calling it w/o arguments)\n");
	printf("ptrim (serial|path) serial\n");
	printf("	print the serial number (rather pointless if you use serial numbers to call the script, but could be useful if you used the path)\n");
	printf("ptrim (serial|path) name [new name]\n");
	printf("	print the devices name or set it if a new name is give\n");
	printf("	*NOTE* this will write to the non-volatile memory when you set the name, do not run this hundreds of thousands of times\n");
	printf("\n");
	printf("ptrim (serial|path) brightness [new brightness]\n");
	printf("	print or set the brightness\n");
	printf("	*NOTE* this will write to the non-volatile memory when you set the brightness, do not run this hundreds of thousands of times\n");
	printf("\n");
	printf("ptrim (serial|path) (start|stop)\n");
	printf("	start or stop the sequence loaded to the device\n");
	printf("\n");
	printf("ptrim (serial|path) color [new color] [temporary brightness]\n");
	printf("	show or set the colors the PlasmaTrim is displaying\n");
	printf("	if a brightness is not given it will use the saved brightness, which could be different than the current brightness\n");
	printf("	the new color string must be wrapped in quotes if it has spaces.\n");
	printf("	the color string can be 3 or 6 characters - which will set all the LEDs to the same color (ie: 'FFF' and 'FFFFFF' are the same)\n");
	printf("		it can also be 8 sets of 3 or 6 characters - which will set every LED to a different color (ex: 'F00 0F0 00F....')\n");
	printf("	the new brightness is NOT saved to non-volatile memory\n");
	printf("\n");
	printf("ptrim (serial|path) fade (new color) (fade time)\n");
	printf("	fade the PlasmaTrim to the new color over some time, the time is an estimate and uses the same numbering as the windows application and ptSeq files.\n");
	printf("		0 => instant, almost same as color \n");
	printf("		1 => 1/10 sec; 2 => 1/4; 3 => 1/2; 4 => 1 sec; 5 => 2.5; 6 => 5; 7 => 10; 8 => 15; 9 => 30 sec\n");
	printf("		a => 1 min; b => 2.5; c => 5; d => 10; e => 15; f => 30 min\n");
	printf("\n");
	printf("ptrim (serial|path) download [filename]\n");
	printf("	get the currently programmed sequence from the device (it will stop at the last active element then report black with no hold or fade for the rest)\n");
	printf("	if no file is give it will print to stdout\n");
	printf("ptrim (serial|path) full_download [filename]\n");
	printf("	same as download except it will not stop reading at the last active slot (useful if you have stored some unfinished work after the last element)\n");
	printf("\n");
	printf("ptrim (serial|path) upload [filename]\n");
	printf("	program the PlasmaTrim with the sequence file, if no file is given it will use stdin (you can pipe sequence files to it)\n");
	printf("	if no file is given it will not overwrite the slots after the last active element (makes uploads faster)\n");
	printf("	*NOTE* this will write to the non-volatile memory, do not run this hundreds of thousands of times\n");
	printf("ptrim (serial|path) full_upload [filename]\n");
	printf("	same as upload except it will not stop writing at the last active slot (useful if you would like to blackout the inactive elements, but it takes longer)\n");
	printf("	*NOTE* this will write to the non-volatile memory, do not run this hundreds of thousands of times\n");
	printf("\n");
}