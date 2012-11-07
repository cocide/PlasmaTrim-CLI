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

	if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0) {
		// derp, thats easy
		ptrim_lib_version();
		ptrim_client_version();
		return 0;
	} else if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
		// derp, thats easy
		help();
		return 0;
	} else if (argc < 3) {
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
	ptrim_lib_version();
	ptrim_client_version();
	format_print(0, "");
	format_print(0, "SYNOPSIS");
	format_print(1, "ptrim-client");
	format_print(1, "ptrim-client --help");
	format_print(1, "ptrim-client --version");
	format_print(0, "");
	format_print(1, "ptrim-client <host> [port] <security-code> <identifier> start");
	format_print(1, "ptrim-client <host> [port] <security-code> <identifier> stop");
	format_print(0, "");
	format_print(1, "ptrim-client <host> [port] <security-code> <identifier> name [new-name]");
	format_print(1, "ptrim-client <host> [port] <security-code> <identifier> brightness [new-brightness]");
	format_print(0, "");
	format_print(1, "ptrim-client <host> [port] <security-code> <identifier> color [new-color [temp-brightness]]");
	format_print(1, "ptrim-client <host> [port] <security-code> <identifier> fade <new-color> <fade-time> [temp-brightness]");
	format_print(0, "");
	format_print(1, "ptrim-client <host> [port] <security-code> <identifier> download [sequence-file]");
	format_print(1, "ptrim-client <host> [port] <security-code> <identifier> full_download [sequence-file]");
	format_print(0, "");
	format_print(1, "ptrim-client <host> [port] <security-code> <identifier> upload [sequence-file]");
	format_print(1, "ptrim-client <host> [port] <security-code> <identifier> full_upload [sequence-file]");
	format_print(0, "");
	format_print(0, "DESCRIPTION");
	format_print(1, "Use ptrim to control, program, read from, and write to one or more PlasmaTrims");
	format_print(0, "");
	format_print(0, "USAGE");
	format_print(1, "ptrim-client --help");
	format_print(2, "Display the help document along with version information");
	format_print(0, "");
	format_print(1, "ptrim-client --version");
	format_print(2, "Display the version number");
	format_print(0, "");
	format_print(1, "ptrim-client <host> [port] <security-code> <identifier> start");
	format_print(2, "Start the last uploaded sequence on PlasmaTrim(s) at identifier");
	format_print(0, "");
	format_print(1, "ptrim-client <host> [port] <security-code> <identifier> stop");
	format_print(2, "Stop the last uploaded sequence on PlasmaTrim(s) at identifier");
	format_print(0, "");
	format_print(1, "ptrim-client <host> [port] <security-code> <identifier> name [new-name]");
	format_print(2, "Show the name of PlasmaTrim(s) at identifier and optionally save it as new-name");
	format_print(2, "WARNING: Saving the name as a new value will write to non-volatile memory, do not run this hundreds of thousands of times!");
	format_print(0, "");
	format_print(1, "ptrim-client <host> [port] <security-code> <identifier> brightness [new-brightness]");
	format_print(2, "Show the brightness of PlasmaTrim(s) at identifier and optionally save it as new-brightness");
	format_print(2, "WARNING: Saving the brightness as a new value will write to non-volatile memory, do not run this hundreds of thousands of times!");
	format_print(0, "");
	format_print(1, "ptrim-client <host> [port] <security-code> <identifier> color [new-color [temp-brightness]]");
	format_print(2, "Show the color(s) that PlasmaTrim(s) at identifier are displaying and optionally set it to new-color with either the saved brightness level or temp-brightness");
	format_print(0, "");
	format_print(1, "ptrim-client <host> [port] <security-code> <identifier> fade <new-color> <fade-time> [temp-brightness]");
	format_print(2, "Fade PlasmaTrim(s) at identifier to new-color over fade-time with either the saved brightness level or temp-brightness");
	format_print(0, "");
	format_print(1, "ptrim-client <host> [port] <security-code> <identifier> download [sequence-file]");
	format_print(2, "Download the current programmed sequence from PlasmaTrim(s) at identifier either to stdout or sequence-file.");
	format_print(2, "Currently this is not very clean, it can not generate pure sequence files yet.");
	format_print(2, "Once the last active slot is reached report all remaining slots as black to speed up this process.");
	format_print(0, "");
	format_print(1, "ptrim-client <host> [port] <security-code> <identifier> full_download [sequence-file]");
	format_print(2, "Download the current programmed sequence from PlasmaTrim(s) at identifier either to stdout or sequence-file.");
	format_print(2, "Currently this is not very clean, it can not generate pure sequence files yet.");
	format_print(2, "Do not stop at the last active slot, get all the data from the device.");
	format_print(0, "");
	format_print(1, "ptrim-client <host> [port] <security-code> <identifier> upload [sequence-file]");
	format_print(2, "Upload the sequence file sequence-file or stdin to PlasmaTrim(s) at identifier");
	format_print(2, "Once the last active slot is reached stop uploading and do not black out unused slots to make the upload faster.");
	format_print(2, "WARNING: Uploading sequences write to non-volatile memory, do not run this hundreds of thousands of times!");
	format_print(0, "");
	format_print(1, "ptrim-client <host> [port] <security-code> <identifier> full_upload [sequence-file]");
	format_print(2, "Upload the sequence file sequence-file or stdin to PlasmaTrim(s) at identifier");
	format_print(2, "Once the last active slot is reached black out all unused slots at the cost of a longer upload time.");
	format_print(2, "WARNING: Uploading sequences write to non-volatile memory, do not run this hundreds of thousands of times!");
	format_print(0, "");
	format_print(0, "OPTIONS");
	format_print(1, "host   The host to find ptrim-server on.");
	format_print(0, "");
	format_print(1, "port   The port to connect over. Default 26163");
	format_print(0, "");
	format_print(1, "security-code");
	format_print(2, "A simple password or pin that is used to authenticate to the server. Special characters must be escaped.");
	format_print(0, "");
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