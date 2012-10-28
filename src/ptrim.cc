#include "ptrim.h"


int main(int argc, char* argv[]) {
	unsigned char number_devices = 0, i;
	hid_device *handle[MAX_DEVICES];
	
	if (argc == 1) {
		// no arguements were given so find/list devices and info
		devs = hid_enumerate(0x26f3, 0x1000);
		cur_dev = devs;	
		while (cur_dev && number_devices < MAX_DEVICES) {
			handle[number_devices] = hid_open_path(cur_dev->path);
			if (handle[number_devices]) {
				// Set the hid_read() function to be non-blocking.
				start_comm(handle[number_devices]);;

				showInfo(handle[number_devices]);

				number_devices++;
			}
			printf("\r\n");

			cur_dev = cur_dev->next;
		}
		hid_free_enumeration(devs);
		if (number_devices == 1) {
			// use a bit better english for only one device
			printf("1 PlasmaTrim was found.\r\n");
		} else {
			printf("%d PlasmaTrims were found.\r\n", number_devices);
		}
	} else {
		// we got some args, so figure out what the hell is going on
		if (strcmp(argv[1], "help") == 0) {
			// derp, thats easy
			help();
			return 0;
		} else if (strcmp(argv[1], "all") == 0) {
			// open all the PlasmaTrims in whatever order is easiest
			devs = hid_enumerate(0x26f3, 0x1000);
			cur_dev = devs;	
			while (cur_dev && number_devices < MAX_DEVICES) {
				handle[number_devices] = hid_open_path(cur_dev->path);
				if (handle[number_devices]) {
					// Set the hid_read() function to be non-blocking.
					start_comm(handle[number_devices]);;
					number_devices++;
				}
				cur_dev = cur_dev->next;
			}
			hid_free_enumeration(devs);
		} else if (argv[1][8] == ' ' || argv[1][12] == ' ') {
			// a few plasmatrims were listed, seperated by spaces
			bool last = false, found = false;
			unsigned char offset = 0, paths_to_open = 0;
			char* device = new char[14];
			char* path_list[MAX_DEVICES];
			memset(device,0x00,14);
			i = 0;
			while (!last && paths_to_open < MAX_DEVICES) {
				// gather all the devices and figure out if its a path or serial
				if (argv[1][i] == ' ' || argv[1][i] == 0x00) {
					// this is either a space or the last element
					if (argv[1][i] == 0x00 ) {
						// if its the last element note it so we dont keep going
						last = true;
					}
					// well thats a whole serial or path, do something with it and move the offset
					offset = i+1;


					if (device[7] != 0x00 && device[8] == 0x00) {
						// this is a serial based on the number of characters
						found = false;
						devs = hid_enumerate(0x26f3, 0x1000);
						cur_dev = devs;
						while (cur_dev && !found) {
							handle[number_devices] = hid_open_path(cur_dev->path);
							if (handle[number_devices]) {
								// Set the hid_read() function to be non-blocking.
								start_comm(handle[number_devices]);;

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
					}
					// reset the device serial/path
					memset(device,0x00,14);
				} else {
					// add to the serial or path
					device[i-offset] = argv[1][i];
				}
				i++;
			}
			for (i=0; i < paths_to_open; i++) {
				// the list needs to be by path so that we don't have to search for them, so open the paths
				handle[number_devices] = hid_open_path(path_list[i]);
				if (!handle[number_devices]) {
					fprintf(stderr, "Could not open device at %s\r\n", path_list[i]);
				} else {
					number_devices++;
					start_comm(handle[number_devices]);
				}
			}
		} else {
			// a serial number was given so find it
			devs = hid_enumerate(0x26f3, 0x1000);
			cur_dev = devs;
			bool found = false;
			while (cur_dev && !found) {
				handle[number_devices] = hid_open_path(cur_dev->path);
				if (handle[number_devices]) {
					// Set the hid_read() function to be non-blocking.
					start_comm(handle[number_devices]);;

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
			// someone needs to do Windows threadding stuffs, I do not have a Windows comp
		#else
			pthread_t thread[number_devices];
		#endif
		
		// when we call the function that parses the commands and runs the actual work we need the args and some more info
		// the easiest way to pass this is in a structure
		struct arg_struct args[number_devices];

		// since the struct is a reffrence we need to make an array of them and have them be unique to avoid stomping on one another
		for (i=0; i < number_devices; i++) {
			args[i].argc = argc;
			args[i].argv = (char**) argv;
			args[i].handle = handle[i];
			args[i].id = i;
			args[i].totalDevices = number_devices;
		}

		if (number_devices > 1 && argc == 3 && ( strcmp(argv[2], "upload") == 0 ||  strcmp(argv[2], "full_upload") == 0 ) ) {
			// they want to do an upload of multiple devics with stdin, this wont work currently so error out
			fprintf(stderr, "When loading multiple devices you must use a file, not stdin\r\n");
			return 1;
		}

		if (argc > 2 && (strcmp(argv[2], "info") != 0 && strcmp(argv[2], "download") != 0 && strcmp(argv[2], "full_download") != 0)) {
			// standard/simple commands that can just be theraded (not that they need to if its only one device, but we already have multidevice communication set up so we might as well)
			// things that have multiline output should not really go here
			for (i=0; i < number_devices; i++) {
				#ifdef WIN32
					// someone needs to do Windows threadding stuffs, I do not have a Windows comp
					runCommand(&args[i]);
				#else
					pthread_create(&thread[i], NULL, runCommand, (void *) &args[i]);
				#endif
			}
			for (i=0; i < number_devices; i++) {
				#ifdef WIN32
					// someone needs to do Windows threadding stuffs, I do not have a Windows comp
				#else
					pthread_join(thread[i], NULL);
				#endif
			}
		} else {
			// commands that do not thread well.
			// listing off the devices, getting the info, and downloading data from the devices
			for (i=0; i < number_devices; i++) {
				runCommand(&args[i]);
			}
		}
	}

	hid_exit();

	return 0;
}


void *runCommand(void *arguments) {
	// parse the user input and run the needed things
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
				upload(args -> handle, args -> argv[3], true, args -> id, args -> totalDevices);
			} else {
				upload(args -> handle, "stdin", true, args -> id, args -> totalDevices);
			}
		} else if (strcmp(args -> argv[2], "full_upload") == 0) {
			if (args -> argc > 3) {
				upload(args -> handle, args -> argv[3], false, args -> id, args -> totalDevices);
			} else {
				upload(args -> handle, "stdin", false, args -> id, args -> totalDevices);
			}
		} else if (strcmp(args -> argv[2], "stream") == 0) {
			//running_threads++; // sometimes file reads lag a few ms, so sync up for starting after all of them read
			if (args -> argc > 4) {
				streamData(args -> handle, args -> argv[3], makeBrightness(atoi(args -> argv[4])), args -> id, args -> totalDevices);
			} else {
				streamData(args -> handle, args -> argv[3], getBrightness(args -> handle), args -> id, args -> totalDevices);
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


void help() {
	printf("PlsamaTrim Utilities by Cocide v0.3.0 - Oct 28 2012\r\n");
	printf("Codename: \"Got network?\"\r\n");
	printf("ptrim-lib v0.2.2, ptrim v0.2.2\r\n");
	printf("\r\n");
	printf("Some help for you:\r\n");
	printf("When calling multiple devices you must have the serials space separated and wrapped in quotes\r\n");
	printf("	ie \"00B12345 00B12346\" will work\r\n");
	printf("	the keywork 'all' can also be used to call all devices\r\n");
	printf("\r\n");
	printf("Usage:\r\n");
	printf("	ptrim-client host pin commands\r\n");
	printf("	ptrim-client host port pin commands\r\n");
	printf("\r\n");
	printf("Command Overview:\r\n");
	printf("\r\n");
	printf("	list the PlasmaTrims that are hooked up to the system.\r\n");
	printf("serial info\r\n");
	printf("	print generic information about the device(s) with the serial (basically the same as calling it w/o arguments)\r\n");
	printf("serial serial\r\n");
	printf("	print the serial number (rather pointless if you use serial numbers to call the script)\r\n");
	printf("serial name [new name]\r\n");
	printf("	print the devices name or set it if a new name is give\r\n");
	printf("	*NOTE* this will write to the non-volatile memory when you set the name, do not run this hundreds of thousands of times\r\n");
	printf("\r\n");
	printf("serial brightness [new brightness]\r\n");
	printf("	print or set the brightness\r\n");
	printf("	*NOTE* this will write to the non-volatile memory when you set the brightness, do not run this hundreds of thousands of times\r\n");
	printf("\r\n");
	printf("serial (start|stop)\r\n");
	printf("	start or stop the sequence loaded to the device\r\n");
	printf("\r\n");
	printf("serial color [new color] [temporary brightness]\r\n");
	printf("	show or set the colors the PlasmaTrim is displaying\r\n");
	printf("	if a brightness is not given it will use the saved brightness, which could be different than the current brightness\r\n");
	printf("	the new color string must be wrapped in quotes if it has spaces.\r\n");
	printf("	the color string can be 3 or 6 characters - which will set all the LEDs to the same color (ie: 'FFF' and 'FFFFFF' are the same)\r\n");
	printf("		it can also be 8 sets of 3 or 6 characters - which will set every LED to a different color (ex: 'F00 0F0 00F....')\r\n");
	printf("	the new brightness is NOT saved to non-volatile memory\r\n");
	printf("\r\n");
	printf("serial fade (new color) (fade time) [temporary brightness]\r\n");
	printf("	fade the PlasmaTrim to the new color over some time, the time is an estimate and uses the same numbering as the windows application and ptSeq files.\r\n");
	printf("		0 => instant, almost same as color \r\n");
	printf("		1 => 1/10 sec; 2 => 1/4; 3 => 1/2; 4 => 1 sec; 5 => 2.5; 6 => 5; 7 => 10; 8 => 15; 9 => 30 sec\r\n");
	printf("		a => 1 min; b => 2.5; c => 5; d => 10; e => 15; f => 30 min\r\n");
	printf("\r\n");
	printf("serial download [filename]\r\n");
	printf("	get the currently programmed sequence from the device (it will stop at the last active element then report black with no hold or fade for the rest)\r\n");
	printf("	if no file is give it will print to stdout\r\n");
	printf("serial full_download [filename]\r\n");
	printf("	same as download except it will not stop reading at the last active slot (useful if you have stored some unfinished work after the last element)\r\n");
	printf("\r\n");
	printf("serial upload [filename]\r\n");
	printf("	program the PlasmaTrim with the sequence file, if no file is given it will use stdin (you can pipe sequence files to it)\r\n");
	printf("	if no file is given it will not overwrite the slots after the last active element (makes uploads faster)\r\n");
	printf("	*NOTE* this will write to the non-volatile memory, do not run this hundreds of thousands of times\r\n");
	printf("serial full_upload [filename]\r\n");
	printf("	same as upload except it will not stop writing at the last active slot (useful if you would like to blackout the inactive elements, but it takes longer)\r\n");
	printf("	*NOTE* this will write to the non-volatile memory, do not run this hundreds of thousands of times\r\n");
	printf("\r\n");
	printf("serial stream (filename) [temporary brightness]\r\n");
	printf("	send the sequence in the file to the device(s) without overwriting the current programming, optionally at a brightness level\r\n");
	printf("	this is slightly slower than if the sequence ran on the device from programming, and it keeps in sync by waiting for them all to finish (USB has some lag at times)\r\n");
	printf("	but the differences in timing between multiple units are minimial, you will probably never notice them since the units will always start together\r\n");
	printf("\r\n");
}

void upload(hid_device *handle, char *filename, bool blank, unsigned char id, unsigned char totalDevices) {
	// program a device
	FILE *file;


	int lineSize = 100 + (27 * MAX_DEVICES) ; // 27 characters per color set for however many devices (for uploading)
	int numberLines = 1024;

	char* fileData[numberLines];
	int i, lineNumber = 0;

	for (i=0; i<numberLines; i++) {
		fileData[i]=new char[lineSize];
	}



	if (strcmp(filename, "stdin") != 0) {
		file = fopen ( filename, "r" );
	} else {
		file = stdin;
	}

	if ( file != NULL ) {
		char line [lineSize];
		while (fgets ( line, sizeof line, file ) != NULL && lineNumber < numberLines ) { // copy the file into the array
			memset(fileData[lineNumber], 0x00, lineSize);
			strcpy(fileData[lineNumber], line);
			lineNumber++;
		}

		if (strcmp(filename, "stdin") != 0) {
			fclose ( file );
		}
		upload(handle, fileData, blank, id, totalDevices, true); // actually upload it
	} else {
		printf("%s did not open.\r\n", filename );
	}
}

void download(hid_device *handle, char *filename, bool blank) {
	int lineSize = 100 + (27 * MAX_DEVICES) ; // 27 characters per color set for however many devices (for uploading)
	int numberLines = 1024;

	char* fileData[numberLines];
	int i, lineNumber = 0;

	for (i=0; i<numberLines; i++) {
		fileData[i]=new char[lineSize];
	}

	int length=strlen(filename);
	char* extension = new char[7];
	memset(extension,0x00,7);


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
	download(handle, fileData, lineSize, numberLines, blank);
	while (fileData[lineNumber] != 0x00 && lineNumber < numberLines) {
		fprintf(file, "%s", fileData[lineNumber]);
		lineNumber++;
	}
	if (strcmp(filename, "stdout") != 0) {
		fclose ( file );
	}
}