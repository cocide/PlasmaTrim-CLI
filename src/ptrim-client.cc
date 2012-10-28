#include "ptrim-client.h"

int main(int argc, char *argv[]) {
	int sockfd, portno, writeStatus;
	struct sockaddr_in serv_addr;
	struct hostent *server;
	int i, j, offset=0, saveto = 0;

	char buffer[4096];
	char devices[4096];
	char about[1024];

	bool showResult = true;

	if (argc < 3) {
		help();
		return 0;
	}
	portno = atoi(argv[2]);

	if (portno == 0) {
		portno = DEFAULT_PORT;
		offset = 1;
	}
	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (sockfd < 0) {
		fprintf(stderr,"Could not open socket.\r\n");
		return -1;
	}
	server = gethostbyname(argv[1]);

	if (server == NULL) {
		fprintf(stderr,"Could not find host.\r\n");
		return -1;
	}

	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(portno);
	if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
		fprintf(stderr,"Could not connect to host.\r\n");
		return -1;
	}

	usleep(5 * 1000);
	writeStatus = write(sockfd,argv[3-offset],strlen(argv[3-offset]));
	if (writeStatus < 0) {
		fprintf(stderr, "Could not write to socket\r\n");
		return -1;
	}

	memset(buffer, 0x00, 4096);
	memset(devices, 0x00, 4096);
	memset(about, 0x00, 1024);

	while (strcmp(buffer, "[ready]\r\n") != 0 && strcmp(buffer, "[incorrect code]\r\n") != 0) {
		if (buffer[1] == 'a') { //quick and dirty check for [about]...
			saveto=1;
		} else if (buffer[1] == 'd') { // quick and dirty check for [devices]...
			saveto=2;
		} else if (buffer[1] == '/') {
			saveto = 0;
		}
		if (saveto == 1) {
			strcat(about, buffer);
		} else if (saveto == 2) {
			strcat(devices, buffer);
		}
		memset(buffer, 0x00, 4096);
		writeStatus = read(sockfd,buffer,4095);
		if (writeStatus < 0) {
			fprintf(stderr, "ERROR reading from socket\r\n");
			return -1;
		}
	}

	// parse the commands
	if (argc == 4-offset || argc == 5-offset) {
		printf("%s\n", about);
		printf("%s\n", devices);
	} else {
		if (strcmp(argv[5-offset], "upload") == 0 || strcmp(argv[5-offset], "full_upload") == 0) {
			for (i=4-offset; i < argc-1; i++) {
				writeStatus = write(sockfd,argv[i],strlen(argv[i])+1);
				if (writeStatus < 0) {
					fprintf(stderr, "Could not write to socket\r\n");
					return -1;
				}
				usleep(25 * 1000);
			}

			if (argc+offset == 7) {
				// we have a filename
				sendFile(argv[i], sockfd);
			} else {
				// we are using stdin
				writeStatus = write(sockfd,argv[i],strlen(argv[i])+1);
				if (writeStatus < 0) {
					fprintf(stderr, "Could not write to socket\r\n");
					return -1;
				}
				usleep(25 * 1000);
				sendFile("stdin", sockfd);
			}

			writeStatus = write(sockfd, "\r\n", 3);

			memset(buffer, 0x00, 4096);
			writeStatus = read(sockfd,buffer,4095);
			if (writeStatus < 0) {
				fprintf(stderr, "ERROR reading from socket\r\n");
				return -1;
			}
			printf("%s",buffer);

			memset(buffer, 0x00, 4096);
			writeStatus = read(sockfd,buffer,4095);
			if (writeStatus < 0) {
				fprintf(stderr, "ERROR reading from socket\r\n");
				return -1;
			}
			printf("%s",buffer);

		} else if (strcmp(argv[5-offset], "download") == 0 || strcmp(argv[5-offset], "full_download") == 0) {
			fd_set socks;
			FD_ZERO(&socks);
			FD_SET(sockfd, &socks);

			struct timeval timeout;
			timeout.tv_sec = 5;
			timeout.tv_usec = 0;

			for (i=4-offset; i < argc-1; i++) {
				writeStatus = write(sockfd,argv[i],strlen(argv[i])+1);
				if (writeStatus < 0) {
					fprintf(stderr, "Could not write to socket\r\n");
					return -1;
				}
				usleep(25 * 1000);
			}

			if (argc+offset == 7) {
				// we have a filename
				//sendFile(argv[i], sockfd);
			} else {
				// we are using stdin
				writeStatus = write(sockfd,argv[i],strlen(argv[i])+1);
				if (writeStatus < 0) {
					fprintf(stderr, "Could not write to socket\r\n");
					return -1;
				}
				usleep(25 * 1000);
				//sendFile("stdin", sockfd);
			}

			writeStatus = write(sockfd, "\r\n", 3);

			do {
				j = select(sockfd+1, &socks, (fd_set *) 0, (fd_set *) 0, &timeout);
				if (j < 0) {
					return -1;
				} else if (j > 0) {
					memset(buffer, 0x00, 4096);
					writeStatus = read(sockfd,buffer,4095);
					if (writeStatus < 0) {
						fprintf(stderr, "ERROR reading from socket\r\n");
						return -1;
					}
					if (strcmp(buffer, "[ready]\r\n") != 0) {
						printf("%s",buffer);
					}
				}
			} while (j > 0 && strcmp(buffer, "[ready]\r\n") != 0);
			showResult = false;

		} else {
			for (i=4-offset; i < argc; i++) {
				writeStatus = write(sockfd,argv[i],strlen(argv[i])+1);
				if (writeStatus < 0) {
					fprintf(stderr, "Could not write to socket\r\n");
					return -1;
				}
				usleep(25 * 1000);
			}
			writeStatus = write(sockfd, "\r\n", 3);
		}
		if (writeStatus < 0) {
			fprintf(stderr, "Could not write to socket\r\n");
			return -1;
		}
		usleep(25 * 1000);

		if (showResult) {
			memset(buffer, 0x00, 4096);
			writeStatus = read(sockfd,buffer,4095);
			if (writeStatus < 0) {
				fprintf(stderr, "ERROR reading from socket\r\n");
				return -1;
			}
			printf("%s",buffer);
		}
	}
	usleep(25 * 1000);

	writeStatus = write(sockfd, "!\r\n", 3);
	if (writeStatus < 0) {
		fprintf(stderr, "Could not write to socket\r\n");
		return -1;
	}

	close(sockfd);
	return 0;
}



void sendFile(char *filename, int sockfd) {
	FILE *file;
	int lineSize = 1024, writeStatus;

	if (strcmp(filename, "stdin") != 0) {
		file = fopen ( filename, "r" );
	} else {
		file = stdin;
	}

	if ( file != NULL ) {
		char line [lineSize];
		while (fgets ( line, sizeof line, file ) != NULL ) { // copy the file into the socket

			writeStatus = write(sockfd,line,strlen(line)+1);
			if (writeStatus < 0) {
				fprintf(stderr, "Could not write to socket\r\n");
				return;
			}
			usleep(5 * 1000);

		}

		if (strcmp(filename, "stdin") != 0) {
			fclose ( file );
		}
	} else {
		printf("%s did not open.\r\n", filename );
	}
}

void help() {
	printf("PlsamaTrim Utilities by Cocide v0.3.0 - Oct 28 2012\r\n");
	printf("Codename: \"Got network?\"\r\n");
	printf("ptrim-lib v0.2.2, ptrim-client v0.1.0\r\n");
	printf("\r\n");
	printf("Some help for you:\r\n");
	printf("When calling multiple devices you must have the serials space separated and wrapped in quotes\r\n");
	printf("	ie \"00B12345 00B12346\" will work\r\n");
	printf("\r\n");
	printf("Usage:\r\n");
	printf("	ptrim-client host pin commands\r\n");
	printf("	ptrim-client host port pin commands\r\n");
	printf("\r\n");
	printf("Command Overview:\r\n");
	printf("\r\n");
	printf("	list the PlasmaTrims that are hooked up to the remote system along with its about string.\r\n");
	printf("serial name (new name)\r\n");
	printf("	change the device name\r\n");
	printf("	*NOTE* this will write to the non-volatile memory, do not run this hundreds of thousands of times\r\n");
	printf("\r\n");
	printf("serial brightness (new brightness)\r\n");
	printf("	 set the brightness\r\n");
	printf("	*NOTE* this will write to the non-volatile memory, do not run this hundreds of thousands of times\r\n");
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
	printf("	this output needs to be cleaned up a lot, currently it is not very usable\r\n");
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
}