#include "ptrim-lib.h"

// instead of having to try to do signal processing across multiple platforms (eventually) just keep track of how many threads are going on to sync things
unsigned char running_threads;


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
	unsigned char i, cmd[32], offset=0;

	memset(cmd,0x00,32);
	cmd[0] = 0x08;

	if (length > 26) {
		// only 26 characters allowd
		length=26;
	}

	for (i=0; i<length; i++) {
		if (name[i] >= 0x20) { // skip over non printiable characters
			cmd[i+1-offset] = name[i];
		} else {
			offset++;
		}

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
	// make a brightness between 1-100, there is probably a better way to do this
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
	// make a color all space seperated so a human can easily read it
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
	// make a binary string for the color info from a unknown length string into either 12 or 24 byte lengths
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
		// we got a 3 character length only
		if (full_length) {
			// and it needs to be 8*6 characters
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
					// so just chop off the least significant bit on each color
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
			// so just chop off the least significant bit on each color
			for (i=0; i<24; i++) {
				color_standardized[i]=color[(i*2)];
			}
		}
	}

	// do some maths to take it outa ascii form
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
	// write a binary color string to the device with a brigtness
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

void download(hid_device *handle, char** fileData, int lineSize, int numberLines, bool blank) {
	// grab the sequence from the device
	int i, j, slotsToGet = 76, hold, fade;
	unsigned char activeSlots=getActiveSlots(handle);

	unsigned char cmd[32];
	char res = 0;

	int lineNumber = 0;

	if (blank) {
		slotsToGet = activeSlots;
	}
	for (i=0; i < numberLines; i++) {
		memset(fileData[i], 0x00, lineSize);
	}


	char* tempSpace[14];
	for (i=0; i<14; i++) {
		tempSpace[i]=new char[lineSize];
	}

	snprintf(fileData[0], numberLines,"PlasmaTrim RGB-8 Sequence\r\n");
	snprintf(fileData[1], numberLines,"Version: Simple Sequence Format\r\n");
	snprintf(fileData[2], numberLines,"Active Slots: %02d\r\n", activeSlots);


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

		snprintf(tempSpace[0], numberLines, "slot %02d %c %c - ", i, hold, fade);
		for (j=0; j<12; j++) {
			snprintf(tempSpace[j+1], numberLines, "%02X", cmd[j+2] );
		}
		snprintf(tempSpace[13], numberLines, "\r\n");
		strcpy(fileData[lineNumber+3], tempSpace[0]);
		for (j=0; j<13; j++) {
			strcat(fileData[lineNumber+3], tempSpace[j+1]);
		}
		lineNumber++;
	} for (i=slotsToGet; i < 76; i++) {
		snprintf(fileData[lineNumber+3], numberLines, "slot %02d 0 0 - 000000000000000000000000\r\n", i);
		lineNumber++;
	}
	snprintf(fileData[lineNumber+3], numberLines, "\r\n");

}


//void upload(hid_device *handle, char** fileData, bool blank, unsigned char id, unsigned char totalDevices) {
//}
int upload(hid_device *handle, char** fileData, bool blank, unsigned char id, unsigned char totalDevices, bool warn) {
	unsigned char i, lineNumber = 0, activeSlots, slotsToLoad = 76, activeDevices = 0, deviceNum = 1, loopStart = 0, offset = 0;
	bool knownType = true, multiDevice = false;
	unsigned char *rawColor;

	unsigned char cmd[32];

	char* color = new char[25];


	stop(handle);

	memset(color,0x00,25);
	memset(color,'0',3);
	setColor(handle, makeColor(color, true), 1); // set it to black because were nice

	while ( knownType && fileData[lineNumber][0] != 0x0D && fileData[lineNumber][0] != 0x0A && fileData[lineNumber][0] != 0x00) { // we know the file type and it is not the last line
		if (lineNumber == 1) {
			if (!( strcmp (fileData[lineNumber], "Version: Simple Sequence Format\r\n") == 0 || strcmp (fileData[lineNumber], "Version: Multiple Sequence Format\r\n") == 0 ||  strcmp (fileData[lineNumber], "Version: Simple Sequence Format") == 0 || strcmp (fileData[lineNumber], "Version: Multiple Sequence Format") == 0) ) {
				knownType = false;
				if (warn) {
					fprintf(stderr, "Unknown file format.\r\n");
				}
				return -1;
			} else if (strcmp (fileData[lineNumber], "Version: Multiple Sequence Format\r\n") == 0 || strcmp (fileData[lineNumber], "Version: Multiple Sequence Format") == 0) {
				multiDevice = true;
			}
		} else if ( lineNumber == 2 ) {
			if (multiDevice) {
				activeDevices =  ((fileData[lineNumber][35] - 48) * 10) + fileData[lineNumber][36] - 48;
				deviceNum = id + 1;
				while (deviceNum > activeDevices) {
					deviceNum-=activeDevices;
				}
				loopStart =  (((fileData[lineNumber][52] - 48) * 10) + fileData[lineNumber][53] - 48) -1;
				i = 80;
				while (fileData[lineNumber][i] > 47 && fileData[lineNumber][i] < 58) {
					activeSlots*=10;
					activeSlots+=fileData[lineNumber][i] - 48;
					i++;
				}
			} else {
				activeSlots = ((fileData[lineNumber][14] - 48) * 10) + fileData[lineNumber][15] - 48;
			}
			if (blank && (activeSlots-loopStart) < 76) {
				// dont bother blanking out the slots
				slotsToLoad = activeSlots-loopStart;
			}
			if (((activeSlots-loopStart) > 76) && id == 0) {
				// we had more than 76 slots
				if (warn) {
					fprintf(stderr, "Notice: More than 76 looping slots are in the sequence, %d are being ignored.\r\n", (activeSlots-loopStart)-76);
				}
			}


			if (activeDevices == 0) {
				activeDevices++;
			}
			if (id == activeDevices) {
				// this is one device past the number of actiev devices, notify them of wrapping
				if (warn) {
					fprintf(stderr, "Notice: This sequence is only written for %d PlasmaTrims, you have %d selected.\r\n", activeDevices, totalDevices);
					fprintf(stderr, "        The remaining devices will be treated as repeats.\r\n");
				}
			} else if (activeDevices > totalDevices && id == (totalDevices -1)) {
				// we have more sequence availabe, tell them
				if (warn) {
					fprintf(stderr, "Notice: This sequence can utilize %d PlasmaTrims, you only have %d selected.\r\n", activeDevices, totalDevices);
				}
			}

		} else if ( lineNumber > (2+loopStart) && lineNumber < 3+slotsToLoad+loopStart) {

			if (lineNumber == 103) {
				offset = 1; // once we hit slot 100 we need to account for the extra digit in the slot #
			}
			memset(color,0x00,25);
			memset(cmd,0x00,32);
			cmd[0] = 0x06;
			cmd[1] = lineNumber - 3 - loopStart;
			for (i=14; i<38; i++) {
				color[i-14] = fileData[lineNumber][i+(27*(deviceNum-1))+offset];
			}
			rawColor = makeColor(color, false);
			for (i=0; i<12; i++) {
				cmd[i+2] = rawColor[i];
			}
			cmd[14] = ((fileData[lineNumber][8+offset] - 0x30) << 4) + fileData[lineNumber][10+offset] - 0x30;

			hid_write(handle, cmd, 32);
			readData(handle);

		}
		lineNumber++;

	}
	setActiveSlots(handle, activeSlots-loopStart);
	start(handle);

	return 0;


}

void streamData(hid_device *handle, char *filename, unsigned char brightness, unsigned char id, unsigned char totalDevices) {
	// make the device(s) look like they are running a sequence, but actually controll it for consistant timing
	unsigned char i, lineNumber = 0, activeSlots = 0, activeDevices = 0, loopStart = 0, deviceNum = 1, offset = 0;
	bool knownType = true, multiDevice = false;

	running_threads++; // keep track how many threads are active so we can sync things

	FILE *file;
	file = fopen ( filename, "r" );
	if ( file != NULL ) {

		char line [ 20 + 27 * MAX_DEVICES ]; // 20 characters for the stuff before the color data (which is actually less) and 27 characters per color set for however many devices
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
		if (activeDevices == 0) {
			activeDevices++;
		}
		if (id == activeDevices) {
			// this is one device past the number of actiev devices, notify them of wrapping
			fprintf(stderr, "Notice: This sequence is only written for %d PlasmaTrims, you have %d selected.\r\n", activeDevices, totalDevices);
			fprintf(stderr, "        The remaining devices will be treated as repeats.\r\n");
		} else if (activeDevices > totalDevices && id == (totalDevices -1)) {
			// we have more sequence availabe, tell them
			fprintf(stderr, "Notice: This sequence can utilize %d PlasmaTrims, you only have %d selected.\r\n", activeDevices, totalDevices);
		}


		while ( knownType && fgets ( line, sizeof line, file ) != NULL ) {
			lineNumber++;
			if (lineNumber > 103) {
				offset = 1; // once we hit slot 100 we need to account for the extra digit in the slot #
			}
			if ( lineNumber > 3 && lineNumber < 4+activeSlots) {
				for (i=14; i<38; i++) {
					slot[lineNumber-4][i-14] = line[i+(27*(deviceNum-1))+offset];
				}
				// save the hold and fade time, but they are fading out, so move it by one
				if (lineNumber-3 == activeSlots) {
					fadeTime[0][0] = line[10+offset];
				} else {
					fadeTime[lineNumber-3][0] = line[10+offset];
				}
				holdTime[lineNumber-4][0] = line[8+offset];

			}
		}
		running_threads--; // the file read is done, lets sync the threads
		while (running_threads > 0) {
			// wait for the other threads to catch up (some variance happens due to lag in the USB transfer)
			delay(1);
		}
		delay(5); // make sure they all notice that it reached 0

		if (knownType && activeSlots > 0) {
			running_threads++; // we sync after the intro, so make them as being runnnig
			for (i=0; i < loopStart; i++) {
				fade(handle, makeHumanColor(slot[i]), fadeTime[i], brightness);
				hold(holdTime[i]);
			}
			running_threads--; // and they are incative
			while (1) {
				while (running_threads > 0) {
					// wait for the other threads to catch up (some variance happens due to lag in the USB transfer)
					delay(1);
				}
				delay(5); // make sure they all notice that it reached 0
				running_threads+=2; // and the sequence began, so mark it as active (but mark it as 2x so we sync on the loop too [useful for very quick sequences])
				for (i=loopStart; i < activeSlots; i++) {
					fade(handle, makeHumanColor(slot[i]), fadeTime[i], brightness);
					hold(holdTime[i]);
					if (i < loopStart+3 && i < activeSlots-3 && i % 5 == 0) { // sync every 5 slots just to make sure the user sees no variance - this will end up adding something like 5-50 ms to the hold depending on how far off the devices are
						running_threads--;
						while (running_threads > totalDevices) {
							// wait for the other threads to catch up (some variance happens due to lag in the USB transfer)
							delay(1);
						}
						delay(5);// make sure they all notice that it reached 0
						running_threads++;
					}
				}
				running_threads-=2; // this sequence is done
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

	if (strcmp(holdTime, "0") == 0) {
		return;
	} else if (strcmp(holdTime, "1") == 0) {
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
		delay(50000);
	} else if (strcmp(holdTime, "a") == 0 || strcmp(holdTime, "A") == 0) {
		delay(60000);
	} else if (strcmp(holdTime, "b") == 0 || strcmp(holdTime, "B") == 0) {
		delay(150000);
	} else if (strcmp(holdTime, "c") == 0 || strcmp(holdTime, "C") == 0) {
		delay(500000);
	} else if (strcmp(holdTime, "d") == 0 || strcmp(holdTime, "D") == 0) {
		delay(600000);
	} else if (strcmp(holdTime, "e") == 0 || strcmp(holdTime, "E") == 0) {
		delay(90000);
	} else if (strcmp(holdTime, "f") == 0 || strcmp(holdTime, "F") == 0) {
		delay(1800000);
	}
}




void readData(hid_device *handle) {
	// after every write we need to read the data from the buffer to prevent errors, but only sometimes we want the data, this reads the data we dont care about
	char res;
	unsigned char cmd[32];
	memset(cmd,0x00,32);

	while (res == 0) {
		res = hid_read(handle, cmd, sizeof(cmd));
		
		delay(1);
	}
}

void delay(unsigned int ms) {
	// since we use different functions on windows platforms just do the selection here once for every sleep
	#ifdef WIN32
		Sleep(ms);
	#else
		usleep(ms*1000);
	#endif
}

void start_comm(hid_device *handle) {
	getSerial(handle); // just preform a random request to clear out the buffer
}

void format_print(int tab, char *string) {
	struct winsize w;
	ioctl(0, TIOCGWINSZ, &w);
	int i, stop, offset = 0, width = (w.ws_col-(tab*3));
	int len = strlen(string);

	if (len == 0) {
		printf("\r\n");
	}

	while (offset < len) {
		for (i=0; i < tab; i++) {
			printf("   ");
		}
		if (len - offset > width) {
			stop=offset+width;
			i=offset+width;
			while (string [i] != ' ' && i > offset) {
				//while we dont have a space and its not a single word
				i--;
			}
			if (i != offset) {
				//if we found the end of the newest word
				stop=i;
			}
		} else {
			stop=len;
		}
		
		for (i=offset; i < stop; i++) {
			if (i < len) {
				//make sure we are not going over
				printf("%c", string[i]);
			}
		}
		printf("\r\n");
		offset=stop;
	}
}

void ptrim_lib_version() {
	format_print(0, "PlsamaTrim Utilities by Cocide v0.3.1 - Nov 7 2012");
	format_print(0, "Codename: \"Documentation is a must\"");
	format_print(1, "ptrim-lib v0.2.3");
}
void ptrim_version() {
	format_print(1, "ptrim v0.2.3");
}
void ptrim_client_version() {
	format_print(1, "ptrim-client v0.1.1");
}
void ptrim_server_version() {
	format_print(1, "ptrim-server v0.1.1");
}