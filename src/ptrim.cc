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
				getSerial(handle[number_devices]);;

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
		if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0) {
			// derp, thats easy
			ptrim_lib_version();
			ptrim_version();
			return 0;
		} else if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
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
					getSerial(handle[number_devices]);;
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
								getSerial(handle[number_devices]);;

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
					getSerial(handle[number_devices]);
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
					getSerial(handle[number_devices]);;

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

		if (argc > 2 && (strcmp(argv[2], "download") != 0 && strcmp(argv[2], "full_download") != 0)) {
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
		if (strcmp(args -> argv[2], "name") == 0) {
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
	ptrim_lib_version();
	ptrim_version();
	format_print(0, "");
	format_print(0, "SYNOPSIS");
	format_print(1, "ptrim");
	format_print(1, "ptrim --help");
	format_print(1, "ptrim --version");
	format_print(0, "");
	format_print(1, "ptrim <identifier>");
	format_print(0, "");
	format_print(1, "ptrim <identifier> start");
	format_print(1, "ptrim <identifier> stop");
	format_print(0, "");
	format_print(1, "ptrim <identifier> name [new-name]");
	format_print(1, "ptrim <identifier> brightness [new-brightness]");
	format_print(0, "");
	format_print(1, "ptrim <identifier> color [new-color [temp-brightness]]");
	format_print(1, "ptrim <identifier> fade <new-color> <fade-time> [temp-brightness]");
	format_print(0, "");
	format_print(1, "ptrim <identifier> download [sequence-file]");
	format_print(1, "ptrim <identifier> full_download [sequence-file]");
	format_print(0, "");
	format_print(1, "ptrim <identifier> upload [sequence-file]");
	format_print(1, "ptrim <identifier> full_upload [sequence-file]");
	format_print(1, "ptrim <identifier> stream <sequence-file> [temp-brightness]");
	format_print(0, "");
	format_print(0, "DESCRIPTION");
	format_print(1, "Use ptrim to control, program, read from, and write to one or more PlasmaTrims");
	format_print(0, "");
	format_print(0, "USAGE");
	format_print(1, "ptrim");
	format_print(2, "List all connected PlasmaTrims and some basic information about them");
	format_print(0, "");
	format_print(1, "ptrim --help");
	format_print(2, "Display the help document along with version information");
	format_print(0, "");
	format_print(1, "ptrim --version");
	format_print(2, "Display the version number");
	format_print(0, "");
	format_print(1, "ptrim <identifier>");
	format_print(2, "Print the information about PlasmaTrim(s) at identifier");
	format_print(0, "");
	format_print(1, "ptrim <identifier> start");
	format_print(2, "Start the last uploaded sequence on PlasmaTrim(s) at identifier");
	format_print(0, "");
	format_print(1, "ptrim <identifier> stop");
	format_print(2, "Stop the last uploaded sequence on PlasmaTrim(s) at identifier");
	format_print(0, "");
	format_print(1, "ptrim <identifier> name [new-name]");
	format_print(2, "Show the name of PlasmaTrim(s) at identifier and optionally save it as new-name");
	format_print(2, "WARNING: Saving the name as a new value will write to non-volatile memory, do not run this hundreds of thousands of times!");
	format_print(0, "");
	format_print(1, "ptrim <identifier> brightness [new-brightness]");
	format_print(2, "Show the brightness of PlasmaTrim(s) at identifier and optionally save it as new-brightness");
	format_print(2, "WARNING: Saving the brightness as a new value will write to non-volatile memory, do not run this hundreds of thousands of times!");
	format_print(0, "");
	format_print(1, "ptrim <identifier> color [new-color [temp-brightness]]");
	format_print(2, "Show the color(s) that PlasmaTrim(s) at identifier are displaying and optionally set it to new-color with either the saved brightness level or temp-brightness");
	format_print(0, "");
	format_print(1, "ptrim <identifier> fade <new-color> <fade-time> [temp-brightness]");
	format_print(2, "Fade PlasmaTrim(s) at identifier to new-color over fade-time with either the saved brightness level or temp-brightness");
	format_print(0, "");
	format_print(1, "ptrim <identifier> download [sequence-file]");
	format_print(2, "Download the current programmed sequence from PlasmaTrim(s) at identifier either to stdout or sequence-file.");
	format_print(2, "Once the last active slot is reached report all remaining slots as black to speed up this process.");
	format_print(0, "");
	format_print(1, "ptrim <identifier> full_download [sequence-file]");
	format_print(2, "Download the current programmed sequence from PlasmaTrim(s) at identifier either to stdout or sequence-file.");
	format_print(2, "Do not stop at the last active slot, get all the data from the device.");
	format_print(0, "");
	format_print(1, "ptrim <identifier> upload [sequence-file]");
	format_print(2, "Upload the sequence file sequence-file or stdin to PlasmaTrim(s) at identifier");
	format_print(2, "Once the last active slot is reached stop uploading and do not black out unused slots to make the upload faster.");
	format_print(2, "WARNING: Uploading sequences write to non-volatile memory, do not run this hundreds of thousands of times!");
	format_print(0, "");
	format_print(1, "ptrim <identifier> full_upload [sequence-file]");
	format_print(2, "Upload the sequence file sequence-file or stdin to PlasmaTrim(s) at identifier");
	format_print(2, "Once the last active slot is reached black out all unused slots at the cost of a longer upload time.");
	format_print(2, "WARNING: Uploading sequences write to non-volatile memory, do not run this hundreds of thousands of times!");
	format_print(0, "");
	format_print(1, "ptrim <identifier> stream <sequence-file> [temp-brightness]");
	format_print(2, "Stream the sequence file sequence-file to PlasmaTrim(s) at identifier either with the saved brightness or temp-brightness");
	format_print(2, "The program will not exit until it is killed or ctrl-c is pressed.");
	format_print(2, "This can be used to stream to multiple devices which will keep all devices synchronized.");
	format_print(0, "");
	format_print(0, "OPTIONS");
	format_print(1, "identifier");
	format_print(2, "The PlasmaTrim(s) to select. This can be a single serial number, multiple serial numbers, or the keyword \"all\". Examples:");
	format_print(2, "00B12345");
	format_print(2, "\"00B12345 00B12346\"");
	format_print(2, "all");
	format_print(0, "");
	format_print(1, "new-name");
	format_print(2, "The new name to save into the PlasmaTrim. All special characters must be escaped.");
	format_print(0, "");
	format_print(1, "new-brightness");
	format_print(2, "The new brightness value to store in memory. This value will be remembered after a power cycle and should be between 1 and 100.");
	format_print(0, "");
	format_print(1, "temp-brightness");
	format_print(2, "The temporary brightness value to be used for this command only. This value will not be remembered after a power cycle and should be between 1 and 100.");
	format_print(0, "");
	format_print(1, "new-color");
	format_print(2, "A hex color code to use for this command. It can be in 3 or 6 digit format, either one color for every LED or one color for all the LEDs, upper or lower case, space separation is allowed if it is wrapped in quotes. Valid examples for setting every LED to red would be:");
	format_print(2, "F00");
	format_print(2, "FF0000");
	format_print(2, "F00F00F00F00F00F00F00F00");
	format_print(2, "\"F00 F00 F00 F00 F00 F00 F00 F00\"");
	format_print(2, "FF0000FF0000FF0000FF0000FF0000FF0000FF0000FF0000");
	format_print(2, "\"FF0000 FF0000 FF0000 FF0000 FF0000 FF0000 FF0000 FF0000\"");
	format_print(0, "");
	format_print(1, "fade-time");
	format_print(2, "How much time to take while making a fade. A single hex character is used and it conforms to the same timing scheme as the sequence timing.");
	format_print(0, "");
	format_print(1, "sequence-file");
	format_print(2, "The path to a .ptSeq file. Both Simple Sequence Format and Multiple Sequence Formats are accepted. If no file is given the for upload stdin will be used, if no file is given for download stdout will be use.");

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