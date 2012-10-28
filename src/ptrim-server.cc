#include "ptrim-server.h"

int main(int argc, char* argv[]) {
	struct socket_struct socket_data[MAX_CLIENTS];
	int sockfd, port, i;
	int optval = 1; // set SO_REUSEADDR on a socket to true (1):
	socklen_t clilen;
	struct sockaddr_in serv_addr, cli_addr;
	code = new char [255];

	if ( argc < 2) {
		help();
		return 1;
	} else if (argc == 2) {
		strcpy(code, argv[1]);
		port = DEFAULT_PORT;
	} else {
		strcpy(code, argv[2]);
		port = atoi(argv[1]);
	}
	if (port < 1) {
		port = DEFAULT_PORT;
		fprintf(stderr,"Port number not valid, using the default (%d)\r\n", port);
	}

	for (i=0; i< MAX_CLIENTS; i++) {
		socket_id[i] = -2;
	}
	


	pthread_t device_thread, notifications_thread, client_thread[MAX_CLIENTS];
	numberClients = 0;

	pthread_create(&device_thread, NULL, monitorDevices, (void*) NULL );
	pthread_create(&notifications_thread, NULL, sendNotifications, (void*) NULL );

	//set up the socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (sockfd < 0) {
		fprintf(stderr, "Could not opening socket\r\n");
		return 1;
	}

	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);

	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(port);
	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		fprintf(stderr, "Could not bind to port %d\r\n", port);
		return 1;
	}
	// set the curser to be hidden
	//initscr();
	//curs_set(0);
	printf("Listening on %d for code '%s'\r\n", port, code);


	listen(sockfd, 5);
	clilen = sizeof(cli_addr);

	while (1) {
		for (i=0; i < MAX_CLIENTS; i++) {
			if (socket_id[i] == -1) {
				pthread_join(client_thread[i], NULL);
				socket_id[i] = -2;
				i = MAX_CLIENTS;
			}
		}
		for (i=0; i < MAX_CLIENTS; i++) {
			if (socket_id[i] == -2) {
				// sometimes accept causes a crash, this needs to be fixed
				socket_id[i] = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
				if (socket_id[i] < 0) { // an error occured, move on
					//socket_id[i] == -2;
					printf("Error: accept(%s)\r\n", strerror(errno));
				} else {
					if (numberClients >= MAX_CLIENTS-1) { // drop the extra clients
						close(socket_id[i]);
						socket_id[i] = -2;
					}
					socket_data[i].socket = i;
					pthread_create(&client_thread[i], NULL, manageConnection, (void*) &socket_data[i] );
				}
				i = MAX_CLIENTS;
			}
		}
	}
	close(sockfd);

	pthread_join(notifications_thread, NULL);
	pthread_join(device_thread, NULL);
	// unhide the curser
	//curs_set(1);
	return 0;
}

void *monitorDevices(void *arguements) {
	unsigned char new_device_number, i, j;
	bool open;
	char *currentDevices[MAX_DEVICES];
	int tempNumberDevices;

	for (i=0; i < MAX_DEVICES; i++) {
		openDevices[i] = new char[32];
		memset(openDevices[i],0x00,32);

		serial[i] = new char[10];
		memset(serial[i],0x00,10);

		name[i] = new char[27];
		memset(name[i],0x00,27);

		brightness[i] = 0;
		tempBrightness[i] = 0;
		status[i] = DISCONNECT;
		notify[i] = 0;
		lock[i] = -1;

		// and the temp list of current devices (to determine new/removed)
		currentDevices[i] = new char[32];
		memset(currentDevices[i],0x00,32);
	}

	while (1) {
		new_device_number = 0;
		// on a Mac we do not get new devices with this enumerate, this bug needs to be fixed somehow at another point in time
		// hid_exit(); will cause the next enumerate to get a full list, but it will also close off handles probably
		devs = hid_enumerate(0x26f3, 0x1000);
		cur_dev = devs;	
		while (cur_dev && new_device_number < MAX_DEVICES) {
			memset(currentDevices[new_device_number],0x00,32);
			strcpy(currentDevices[new_device_number], cur_dev->path);
			new_device_number++;

			cur_dev = cur_dev->next;
		}
		hid_free_enumeration(devs);

		//remove devices that are no longer around
		for (i=0; i < MAX_DEVICES; i++) {
			if (openDevices[i][0] != 0x00) {
				open=false;
				for (j=0; j < new_device_number; j++) {
					if (strcmp(openDevices[i], currentDevices[j]) == 0) {
						//this device is open so remove it from our new list
						open=true;
						memset(currentDevices[j],0x00,32);
					}
				}
				if (!open) {
					// the device is no longer available
					status[i] = DISCONNECT;
					notify[i] = 0;
					lock[i] = -1;
					for (j=0; j < MAX_CLIENTS; j++) {
						if (socket_id[j] > 0) {
							send (socket_id[j], "[removed ", 9, 0); //MSG_MORE);
							send (socket_id[j], serial[i], 8, 0);
							send (socket_id[j], "]\r\n", 3, 0);
							send (socket_id[j], 0x00, 1, MSG_EOR);
						}
					}
					hid_close(handle[i]);
					memset(openDevices[i],0x00,32);
					memset(serial[i],0x00,10);
					memset(name[i],0x00,27);
					brightness[i] = 0;
					tempBrightness[i] = 0;
				}
			} 
		}

		//add devices that are new
		for (i=0; i < MAX_DEVICES; i++) {
			open=false;
			if (openDevices[i][0] == 0x00) {
				for (j=0; j < new_device_number; j++) {
					if (currentDevices[j][0] != 0x00 && !open) {
						//this device is new, so open it
						handle[i] = hid_open_path(currentDevices[j]);
							if (handle[i]) {
								start_comm(handle[i]);;

								strcpy(openDevices[i], currentDevices[j]);
								memset(currentDevices[j],0x00,32);

								memset(serial[i],0x00,10);
								strcpy(serial[i], getSerial(handle[i]));

								memset(name[i],0x00,27);
								strcpy(name[i], getName(handle[i]));

								brightness[i] = getBrightness(handle[i]);
								tempBrightness[i] = brightness[i];
								start(handle[i]);
								status[i] = PLAY;
								notify[i] = 0;
								lock[i] = -1;

								for (j=0; j < MAX_CLIENTS; j++) {
									if (socket_id[j] > 0) {
										send (socket_id[j], "[added]\r\n", 9, 0); //MSG_MORE);
										deviceInfo( socket_id[j], i);
										send (socket_id[j], "[/added]\r\n", 10, 0);
										send (socket_id[j], 0x00, 1, MSG_EOR);
									}
								}

								open = true;
							}

					}
				}
			}
		}

		tempNumberDevices = 0;
		for (i=0; i < MAX_DEVICES; i++) {
			if (openDevices[i][0] != 0x00) {
				tempNumberDevices++;
			}
		}
		numberDevices = tempNumberDevices;

		printf("\r%02d/%02d Devices - %02d/%02d Clients", numberDevices, MAX_DEVICES, numberClients, MAX_CLIENTS-1 ); // compensate for the extra thread slot
		fflush(stdout);
		delay(1000); // refresh the devices every second because thats probably well more than fast enough
	}

	hid_exit();
	return NULL;
}


void help() {
	printf("PlsamaTrim Utilities by Cocide v0.3.0 - Oct 28 2012\r\n");
	printf("Codename: \"Got network?\"\r\n");
	printf("ptrim-lib v0.2.2, ptrim-server v0.1.0\r\n");
	printf("Usage: ptrim-server [port] (security code)\r\n");
}

void clientWelcome( int fd ) {
	char* max_devices_string = new char[3];
	char* max_clients_string = new char[3];

	snprintf(max_devices_string, 3, "%02d", MAX_DEVICES);
	snprintf(max_clients_string, 3, "%02d", MAX_CLIENTS-1); // compensate for the extra thread slot

	send (fd, "[about]\r\n", 9, 0); //MSG_MORE);
	send (fd, "PlasmaTrim Network Server\r\n", 27, 0); //MSG_MORE);
	send (fd, "version 0.1.0\r\n", 15, 0); //MSG_MORE);

	send (fd, "max devices ", 12, 0); //MSG_MORE);
	send (fd, max_devices_string, 2, 0); //MSG_MORE);
	send (fd, "\r\n", 2, 0); //MSG_MORE);

	send (fd, "max clients ", 12, 0); //MSG_MORE);
	send (fd, max_clients_string, 2, 0); //MSG_MORE);
	send (fd, "\r\n", 2, 0); //MSG_MORE);

	send (fd, "[/about]\r\n", 10, 0);
	send (fd, 0x00, 1, MSG_EOR);
}

void connectedDevices( int fd) {
	int i;
	bool provideSeperator = false;

	send (fd, "[devices]\r\n", 11, 0); //MSG_MORE);
	for (i=0; i< MAX_DEVICES; i++) {
		if ( status[i] != DISCONNECT ) {
			//we only print a seperator when more than one device is listed
			if (provideSeperator) {
				send (fd, "---\r\n", 5, 0); //MSG_MORE);
			} else {
				provideSeperator = true;
			}
			deviceInfo(fd, i);
		}
	}
	send (fd, "[/devices]\r\n", 13, 0);
	send (fd, 0x00, 1, MSG_EOR);
}

void *sendNotifications( void* ) {
	int i, j;
	char* brightness_string = new char[4];

	while (1) {
		for (i=0; i < MAX_DEVICES; i++) {
			if (status[i] != DISCONNECT && notify[i] != 0) {
				for (j=0; j < MAX_CLIENTS; j++) {
					if (socket_id[j] > 0) {
						send (socket_id[j], "[update]\r\n", 10, 0); //MSG_MORE);
							send (socket_id[j], "Serial: ", 8, 0); //MSG_MORE);
							send (socket_id[j], serial[i], 8, 0); //MSG_MORE);
								send (socket_id[j], "\r\n", 2, 0); //MSG_MORE);

							if ((notify[i] & NAME) > 0) {
								send (socket_id[j], "Name: ", 6, 0); //MSG_MORE);
								send (socket_id[j], name[i], strlen(name[i]), 0); //MSG_MORE);
								send (socket_id[j], "\r\n", 2, 0); //MSG_MORE);
							}
							if ((notify[i] & BRIGHTNESS) > 0) {
								memset(brightness_string, 0x00, 4);
								send (socket_id[j], "Saved Brightness: ", 18, 0); //MSG_MORE);
								snprintf(brightness_string, 4, "%03d", brightness[i]);
								send (socket_id[j], brightness_string, 3, 0); //MSG_MORE);
								send (socket_id[j], "%", 1, 0); //MSG_MORE);
								send (socket_id[j], "\r\n", 2, 0); //MSG_MORE);
							}
							if ((notify[i] & TEMP_BRIGHTNESS) > 0) {
								memset(brightness_string, 0x00, 4);
								send (socket_id[j], "Temp Brightness: ", 17, 0); //MSG_MORE);
								snprintf(brightness_string, 4, "%03d", tempBrightness[i]);
								send (socket_id[j], brightness_string, 3, 0); //MSG_MORE);
								send (socket_id[j], "%", 1, 0); //MSG_MORE);
								send (socket_id[j], "\r\n", 2, 0); //MSG_MORE);
							}
							if ((notify[i] & STATUS) > 0) {
								statusInfo(socket_id[j], i);
							}
						send (socket_id[j], "[/update]\r\n", 11, 0);
						send (socket_id[j], 0x00, 1, MSG_EOR);
					}
				}
				notify[i]=0; // clean the notification
			}
		}
		delay(100);
	}
	return NULL;
}

void deviceInfo( int fd, int id) {
	char* brightness_string = new char[4];

	if ( serial[id][0] != 0x00 ) {
		send (fd, "Serial: ", 8, 0); //MSG_MORE);
		send (fd, serial[id], 8, 0); //MSG_MORE);
		send (fd, "\r\nName: ", 8, 0); //MSG_MORE);
		send (fd, name[id], strlen(name[id]), 0); //MSG_MORE);
		send (fd, "\r\n", 2, 0); //MSG_MORE);

		if (status[id] > DISCONNECT) {
			send (fd, "Saved Brightness: ", 18, 0); //MSG_MORE);
			snprintf(brightness_string, 4, "%03d", brightness[id]);
			send (fd, brightness_string, 3, 0); //MSG_MORE);
			send (fd, "%\r\n", 3, 0); //MSG_MORE);
			send (fd, "Temp Brightness: ", 17, 0); //MSG_MORE);
			snprintf(brightness_string, 4, "%03d", tempBrightness[id]);
			send (fd, brightness_string, 3, 0); //MSG_MORE);
			send (fd, "%\r\n", 3, 0); //MSG_MORE);
			statusInfo(fd, id);
		}
	}
}

void statusInfo(int fd, int id) {
	send (fd, "Status: ", 8, 0); //MSG_MORE);
	switch	(status[id]) {
		case PLAY:
			send (fd, "Playing", 7, 0); //MSG_MORE);
			break;
		case IDLE:
			send (fd, "Stopped", 7, 0); //MSG_MORE);
			break;
		case STREAM:
			send (fd, "Software Controlled", 19, 0); //MSG_MORE);
			break;
		case LOCK:
			send (fd, "Locked to Client", 16, 0); //MSG_MORE);
			break;
	}
	send (fd, "\r\n", 2, 0); //MSG_MORE);
}

void runCommand(char** commands, int output) {
	int deviceIdNum[MAX_DEVICES];

	int i=0, j=0 , offset=0, selectedDevices=0;
	char* returnString = new char[2049];
	memset(returnString, 0x00, 2049);


	char* serialNumber = new char [10];

	//deviceIdNum[0] = findDeviceNumber(commands[0]);

	for (i=0; i < MAX_DEVICES; i++) {
		deviceIdNum[i] = -1;
	}


	// grab all the device id numbers for the given serials
	i=0;
	while (commands[0][i] > 0x19) {
		memset(serialNumber, 0x00, 10);
		while (commands[0][i] > 0x20) {
			serialNumber[i-offset] = commands[0][i];
			i++;
		}
		while (commands[0][i] > 0x00 && commands[0][i] < 0x30) {
			i++;
		}
		offset = i;
		deviceIdNum[selectedDevices] = findDeviceNumber(serialNumber);
		selectedDevices++;
	}

	if (deviceIdNum[0] < 0) {
		send (output, "[device not found]\r\n", 20, 0); //MSG_MORE);
		return;
	}
	pthread_t thread[selectedDevices];
	struct instance_struct instance[selectedDevices];


	//we have some list of IDs, lets use them
	j=0;
	for (i=0; i < MAX_DEVICES; i++) {
		if (deviceIdNum[i] >= 0) {
			// since the struct is a reffrence we need to make an array of them and have them be unique to avoid stomping on one another
			instance[j].commands = (char**) commands;
			instance[j].handleID = deviceIdNum[i];
			instance[j].handle = handle[deviceIdNum[i]];
			instance[j].id = j;
			instance[j].selectedDevices = selectedDevices;
			instance[j].output = output;
			j++;
		}
	}
	for (i=0; i < selectedDevices; i++) {
		pthread_create(&thread[i], NULL, runThread, (void *) &instance[i]);
	}
	for (i=0; i < selectedDevices; i++) {
		pthread_join(thread[i], NULL);
	}
}

void *runThread(void *arguments) {
	// parse the user input and run the needed things
	struct instance_struct *instance = (struct instance_struct *)arguments;
	int i, j;

	if (status[instance -> handleID] != LOCK || (status[instance -> handleID] == LOCK && lock[instance -> handleID] == instance -> output)) { // if we are not locked or we own the lock


		if (strcmp(instance -> commands[1], "lock") == 0) {
			if (status[instance -> handleID] == LOCK) {
				send (instance -> output, "[", 1, 0); //MSG_MORE);
				send (instance -> output, serial[instance -> handleID], 10, 0); //MSG_MORE);
				send (instance -> output, " already locked]\r\n", 18, 0);
				send (instance -> output, 0x00, 1, MSG_EOR);
			} else {
				status[instance -> handleID] = LOCK;
				lock[instance -> handleID] = instance -> output;
				notify[instance -> handleID] |= STATUS;
			}
		} else if (strcmp(instance -> commands[1], "unlock") == 0) {
			if (status[instance -> handleID] == LOCK) {
				// the deice is actually locked
				stop(instance -> handle);
				status[instance -> handleID] = IDLE;
				notify[instance -> handleID] |= STATUS;
				lock[instance -> handleID] = -1;
			} else { // the device was not locked
				send (instance -> output, "[", 1, 0); //MSG_MORE);
				send (instance -> output, serial[instance -> handleID], 10, 0); //MSG_MORE);
				send (instance -> output, " not locked]\r\n", 14, 0);
				send (instance -> output, 0x00, 1, MSG_EOR);
			}
		} else if (strcmp(instance -> commands[1], "start") == 0) {
			start(instance -> handle);
			status[instance -> handleID] = PLAY;
			notify[instance -> handleID] |= STATUS;
		} else if (strcmp(instance -> commands[1], "stop") == 0) {
			stop(instance -> handle);
			status[instance -> handleID] = IDLE;
			notify[instance -> handleID] |= STATUS;
		} else if (strcmp(instance -> commands[1], "name") == 0) {
			setName(instance -> handle, instance -> commands[2]);
			name[instance -> handleID] = getName(instance -> handle);
			notify[instance -> handleID] |= NAME;
		} else if (strcmp(instance -> commands[1], "brightness") == 0) {
			if (strcmp(instance -> commands[2], "temp") == 0) { //we have a temp brightness
				if ( makeBrightness(atoi(instance -> commands[3])) != tempBrightness[instance -> handleID] ) { // and its new
					tempBrightness[instance -> handleID] = makeBrightness(atoi(instance -> commands[3]));
					notify[instance -> handleID] |= TEMP_BRIGHTNESS;
					if (status[instance -> handleID] == IDLE) { //if its just idle update it
						setColor(instance -> handle, makeColor(getColor(instance -> handle), true), tempBrightness[instance -> handleID]);
					}
				}
			} else if (strcmp(instance -> commands[2], "reset") == 0) { //  we want to revert to saved
				if (tempBrightness[instance -> handleID] != brightness[instance -> handleID]) { // and its different
					tempBrightness[instance -> handleID] = brightness[instance -> handleID];
					notify[instance -> handleID] |= TEMP_BRIGHTNESS;
				}
			} else if (strcmp(instance -> commands[2], "save") == 0) { // we want to save the temp
				if (tempBrightness[instance -> handleID] != brightness[instance -> handleID]) { // and its different
					brightness[instance -> handleID] = tempBrightness[instance -> handleID];
					setBrightness(instance -> handle, brightness[instance -> handleID]);
					notify[instance -> handleID] |= BRIGHTNESS;
				}
			} else if (instance -> commands[3][0] < 0x20){ // no thrid command was given so it must be wanting to just set the saved brightness
				if (makeBrightness(atoi(instance -> commands[2])) != brightness[instance -> handleID]) { // and its new
					setBrightness(instance -> handle, makeBrightness(atoi(instance -> commands[2])));
					brightness[instance -> handleID] = getBrightness(instance -> handle);
					notify[instance -> handleID] |= BRIGHTNESS;
					if ( brightness[instance -> handleID] != tempBrightness[instance -> handleID]) { // the temp brightness needs to change too
						notify[instance -> handleID] |= TEMP_BRIGHTNESS;
						tempBrightness[instance -> handleID] = brightness[instance -> handleID];
					}
					if (status[instance -> handleID] == IDLE) { //if its just idle update it
						setColor(instance -> handle, makeColor(getColor(instance -> handle), true), tempBrightness[instance -> handleID]);
					}
				}
			}
		} else if (strcmp(instance -> commands[1], "color") == 0) {
			if (instance -> commands[2][0] < 0x20) { // we only have the color command
				setColor(instance -> handle, makeColor( instance -> commands[2], true), tempBrightness[instance -> handleID]);
				send (instance -> output, "[", 1, 0); //MSG_MORE);
				send (instance -> output, serial[instance -> handleID], 10, 0); //MSG_MORE);
				send (instance -> output, " color ",7, 0); //MSG_MORE);
				send (instance -> output, makeHumanColor(getColor(instance -> handle)), 55, 0); //MSG_MORE);
				send (instance -> output, "]\r\n", 3, 0);
				send (instance -> output, 0x00, 1, MSG_EOR);
				if (status[instance -> handleID] == PLAY) { // it was playing, so restart it
					start(instance -> handle);
				}
			} else if (instance -> commands[3][0] != 0x00 ) { // we have a new temp brightness too
				if (makeBrightness(atoi(instance -> commands[3])) != tempBrightness[instance -> handleID]) { // which is different
					tempBrightness[instance -> handleID] = makeBrightness(atoi(instance -> commands[3]));
					notify[instance -> handleID] |= TEMP_BRIGHTNESS;
				}
			}
			if (status[instance -> handleID] != IDLE) {
				status[instance -> handleID] = IDLE;
				notify[instance -> handleID] |= STATUS;
			}
			setColor(instance -> handle, makeColor( instance -> commands[2], true), tempBrightness[instance -> handleID]);
		} else if (strcmp(instance -> commands[1], "fade") == 0) {
			if (instance -> commands[4][0] != 0x00 && makeBrightness(atoi(instance -> commands[4])) != tempBrightness[instance -> handleID] ) { // we have a new temp brightness too
				tempBrightness[instance -> handleID] = makeBrightness(atoi(instance -> commands[4]));
				notify[instance -> handleID] |= TEMP_BRIGHTNESS;
			}
			if (status[instance -> handleID] != IDLE) {
				status[instance -> handleID] = IDLE;
				notify[instance -> handleID] |= STATUS;
			}
			fade(instance -> handle,(char*) makeColor( instance -> commands[2], true), instance -> commands[3], tempBrightness[instance -> handleID]);
		} else if (strcmp(instance -> commands[1], "upload") == 0 || strcmp(instance -> commands[1], "full upload") == 0 || strcmp(instance -> commands[1], "full_upload") == 0) {
			int lineSize = 100 + (27 * MAX_DEVICES) ; // 27 characters per color set for however many devices (for uploading)
			int numberLines = 1024;

			char* fileData[numberLines];

			for (j=0; j < numberLines; j++) {
				fileData[j]=new char[lineSize];
			}

			i=2;
			while (instance -> commands[i][0] != 0x0A && instance -> commands[i][0] != 0x0D && instance -> commands[i][0] != 0x00) {
				strcpy(fileData[i-2], instance -> commands[i]);
				i++;
			}
			if (strcmp(instance -> commands[1], "full upload") == 0 || strcmp(instance -> commands[1], "full_upload") == 0) {
				i=upload(instance -> handle, fileData, false, instance -> id, instance -> selectedDevices, false);
			} else {
				i=upload(instance -> handle, fileData, true, instance -> id, instance -> selectedDevices, false);
			}
			send (instance -> output, "[", 1, 0); //MSG_MORE);
			send (instance -> output, serial[instance -> handleID], 8, 0); //MSG_MORE);
			if (i < 0) {
				send (instance -> output, " upload error]\r\n", 16, 0);
				send (instance -> output, 0x00, 1, MSG_EOR);
			} else {
				send (instance -> output, " upload success]\r\n", 18, 0);
				send (instance -> output, 0x00, 1, MSG_EOR);
			}

		} else if (strcmp(instance -> commands[1], "download") == 0 || strcmp(instance -> commands[1], "full download") == 0 || strcmp(instance -> commands[1], "full_download") == 0) {
			int lineSize = 100 + (27 * MAX_DEVICES) ; // 27 characters per color set for however many devices (for uploading)
			int numberLines = 1024;

			char* fileData[numberLines];

			for (j=0; j < numberLines; j++) {
				fileData[j]=new char[lineSize];
			}

			if (strcmp(instance -> commands[1], "full upload") == 0 || strcmp(instance -> commands[1], "full_upload") == 0) {
				download(instance -> handle, fileData, lineSize, numberLines, false);
			} else {
				download(instance -> handle, fileData, lineSize, numberLines, true);
			}
			j=0;
			while (fileData[j][0] != 0x0D && fileData[j][0] != 0x0A) {
				send (instance -> output, "[", 1, 0); //MSG_MORE);
				send (instance -> output, serial[instance -> handleID], 8, 0); //MSG_MORE);
				send (instance -> output, " ", 1, 0); //MSG_MORE);
				send (instance -> output, fileData[j], strlen(fileData[j])-2, 0);
				send (instance -> output, "]\r\n", 3, 0);
				send (instance -> output, 0x00, 1, MSG_EOR);
				delay (10);
				j++;
			}

		} else {
			send (instance -> output, "[unknown command ", 17, 0); //MSG_MORE);
			send (instance -> output, instance -> commands[1], strlen(instance -> commands[1]), 0); //MSG_MORE);
			send (instance -> output, "]\r\n", 3, 0);
			send (instance -> output, 0x00, 1, MSG_EOR);
		}
	} else if (strcmp(instance -> commands[1], "force unlock") == 0) { // the device was locked by someone else
		if (status[instance -> handleID] == LOCK) {
			// the deice is actually locked
			stop(instance -> handle);
			status[instance -> handleID] = IDLE;
			notify[instance -> handleID] |= STATUS;
			lock[instance -> handleID] = -1;
		} else { // the device was not locked
			send (instance -> output, "[", 1, 0); //MSG_MORE);
			send (instance -> output, serial[instance -> handleID], 10, 0); //MSG_MORE);
			send (instance -> output, " not locked]\r\n", 14, 0);
			send (instance -> output, 0x00, 1, MSG_EOR);
			lock[instance -> handleID] = -1;
		}
	} else {
		send (instance -> output, "[", 1, 0); //MSG_MORE);
		send (instance -> output, serial[instance -> handleID], 10, 0); //MSG_MORE);
		send (instance -> output, " permission denied]\r\n", 21, 0);
		send (instance -> output, 0x00, 1, MSG_EOR);

	}


	//deviceInfo(instance -> output, instance -> handleID);
	return NULL;

}

int findDeviceNumber(char* serialNumber) {
	int i;
	for (i=0; i < MAX_DEVICES; i++) {
		if (openDevices[i][0] != 0x00) {
			if (strcmp(serial[i], serialNumber) == 0) {
				return i;
			}
		}
	}

	return -1;

}

void *manageConnection(void *arguements) {
	struct socket_struct *socket_data = (struct socket_struct *)arguements;
	int lineSize = 100 + (27 * MAX_DEVICES) ; // 27 characters per color set for however many devices (for uploading)
	int numberLines = 1024;

	char buffer[lineSize];
	char* commands[numberLines];
	int read_result, i, j;

	fd_set socks;
	FD_ZERO(&socks);
	FD_SET(socket_id[socket_data -> socket],&socks);

	struct timeval timeout;
	timeout.tv_sec = 5;
	timeout.tv_usec = 0;

	for (i=0; i<numberLines; i++) {
		commands[i]=new char[lineSize];
	}

	if (socket_id[socket_data -> socket] < 0) {
		//fprintf(stderr, "Could not accept connection\r\n");
		socket_id[socket_data -> socket] = -1;
		return NULL;
	}
	read_result = select(socket_id[socket_data -> socket]+1, &socks, (fd_set *) 0, (fd_set *) 0, &timeout);
	if (read_result < 0) {
		return NULL;
	} else if (read_result > 0) {
		memset(buffer,0x00,lineSize);
		read_result = read(socket_id[socket_data -> socket],buffer,lineSize-1);

		// clean up the security code to remove things like \r\n
		i=0;
		while (buffer[i] != 0x00) {
			if (buffer[i] < 0x20) {
				buffer[i] = 0x00;
			}
			i++;
		}
		if (strcmp(buffer, code) == 0) {
			numberClients++;
			clientWelcome(socket_id[socket_data -> socket]);
			delay(100); // delay between messages
			connectedDevices(socket_id[socket_data -> socket]);
			do {
				// get the commands

				delay(100); // delay between messages
				send (socket_id[socket_data -> socket], "[ready]\r\n", 9, 0);
				send (socket_id[socket_data -> socket], 0x00, 1, MSG_EOR);
				i=0; // command arguement counter
				while (buffer[0] != 0x0D && buffer[0] != 0x0A && buffer[0] != 0x00 && buffer[0] != '!' && buffer[0] != '.' && i < numberLines) { // while we dont have a line ending, a quit/timeout, and were under our max number of lines

					memset(buffer,0x00,lineSize);
					memset(commands[i], 0x00, lineSize);

					timeout.tv_sec = 300;
					timeout.tv_usec = 0;

					FD_ZERO(&socks);
					FD_SET(socket_id[socket_data -> socket],&socks);

					read_result = 0;
					read_result = select(socket_id[socket_data -> socket]+1, &socks, (fd_set *) 0, (fd_set *) 0, &timeout);

					if (read_result < 0) {
						//fprintf(stderr, "Timeout\r\n");
						return NULL;
					} else if (read_result > 0) {
						read_result = read(socket_id[socket_data -> socket],buffer,lineSize-1);
						j=0;
						while (buffer[j] > 0x19) {
							commands[i][j] = buffer[j];
							j++;
						}
						
						if (read_result < 0) {
							//fprintf(stderr, "Could not send to socket\r\n", 0); //MSG_MORE);
							commands[0][0] = '!';
							buffer[0] = '!';
						}
						i++;
					} else {
						//fprintf(stderr, "Could not read from socket\r\n");
						commands[0][0] = '!';
						buffer[0] = '!';
					}
				}
				if (commands[0][0] != '!' && commands[0][0] != 0x00 && buffer[0] != '.') {
					delay(100); // delay between messages
					send (socket_id[socket_data -> socket], "[accepted]\r\n", 12, 0);
					send (socket_id[socket_data -> socket], 0x00, 1, MSG_EOR);
					runCommand(commands, socket_id[socket_data -> socket]);
				}

				memset(buffer,0x01,lineSize); // clear the buffer so we dont loop threw the commands again, 0x01 is just someting not in the list of things to flag
			} while (commands[0][0] != '!' && commands[0][0] != 0x00);

			numberClients--;
			
		} else {
			// wrong password
			send (socket_id[socket_data -> socket], "[incorrect code]\r\n", 18, 0);
			send (socket_id[socket_data -> socket], 0x00, 1, MSG_EOR);
		}
	} else {
		//fprintf(stderr, "Could not read from socket\r\n");
	}

	for (i=0; i < MAX_DEVICES; i++) {
		if (lock[i] == socket_id[socket_data -> socket]) { // this device was locked to this terminal, but we are now gone
			lock[i] = -1;
			stop(handle[i]);
			status[i] = IDLE;
			notify[i] |= STATUS;
		}
	}

	close(socket_id[socket_data -> socket]);
	socket_id[socket_data -> socket] = -1;
	return NULL;
}
