//Jesse A. Jones
//15 Dec, 2022
//CS 360
//Final Project
//V: 1.14

//Inclusions and function descriptions 
// can be found in the header file included below.
#include"myftp.h"

int forkErrorCheck(int retVal){
	int errnoTemp;

	//Error checks passed in return value.
	if (retVal == -1){
		errnoTemp = errno;
		fprintf(stderr, "Server Child %d Suffered Fork Error: %s\n", getpid(), strerror(errnoTemp));
		fflush(stderr);
		return errnoTemp;
	}

	return 0;
}

int dupErrorCheck(int retVal){
	int errnoTemp;

	//Error checks passed in return value.
	if (retVal == -1){
		errnoTemp = errno;
		fprintf(stderr, "Server Child %d Suffered Dup Error: %s\n", getpid(), strerror(errnoTemp));
		fflush(stderr);
		return errnoTemp;
	}

	return 0;
}

int serverSocket(const char *portNum){
	int listenfd, bindresult, errnoTemp, sockOptRetVal;

	listenfd = socket(AF_INET, SOCK_STREAM, 0);

	//Error checks creation of control line socket.
	if (listenfd == -1){
		errnoTemp = errno;
		fprintf(stderr, "Server Child %d Suffered Socket Error: %s\n", getpid(), strerror(errnoTemp));
		fflush(stderr);
		return -1;
	}

	//Makes it so socket can be cleared 
	// and made available if the server quits.
	sockOptRetVal = setsockopt(listenfd, 
		SOL_SOCKET, 
		SO_REUSEADDR, 
		&(int){1}, 
		sizeof(int));

	if (sockOptRetVal < 0){
		errnoTemp = errno;
		fprintf(stderr, "Server Child %d Suffered Sock Option Set Error: %s\n", getpid(), strerror(errnoTemp));
		return -1;
	}

	//Sets up server address structure and its members.
	struct sockaddr_in serverAddress;
	memset(&serverAddress, 0, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(atoi(portNum));
	serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);

	bindresult = bind(listenfd, 
		(struct sockaddr *)&serverAddress, 
		sizeof(serverAddress));

	//Error checks binding of address to port.
	if (bindresult < 0){
		perror("bind");
		return -2;
	}

	return listenfd;

}

void writeErrorToClient(const char *errorString, int controlfd){
	write(controlfd, "E", 1);
	write(controlfd, errorString, strlen(errorString));
	write(controlfd, "\n", 1);
}

int changeServerDir(char *path, int controlfd){
	int errnoTemp, cdRetVal;

	cdRetVal = chdir(path);

	//Error checks changing directories.
	if (cdRetVal != 0){
		errnoTemp = errno;
		fprintf(stderr, "Server Child %d Change Dir Error for %s: %s\n", getpid(), path, strerror(errnoTemp));
		fflush(stderr);
		writeErrorToClient(strerror(errnoTemp), controlfd);
		return errnoTemp;
	}else{
		fprintf(stdout, "Server Child %d successfully changed to directory: %s\n", getpid(), path);
		fflush(stdout);
		write(controlfd, "A\n", 2);
		return 0;
	}
	return 0;
}

int rlsServerLsFunction(int datafd){
	int errnoTemp, funcCallResult, forkRes, waitResult;

	forkRes = fork();

	//Forks and error checks.
	errnoTemp = forkErrorCheck(forkRes);
	if (errnoTemp != 0) return errnoTemp;

	if (forkRes){
		wait(&waitResult);
		close(datafd);
		return waitResult;
	}else{
		//Makes it so ls writes to data pipe.
		errnoTemp = dupErrorCheck(dup2(datafd, 1));
		if (errnoTemp != 0) exit(errnoTemp);

		//String array used in execvp to execute "ls -l".
		char *lsCmd[3] = {NULL, NULL, NULL};
		lsCmd[0] = "ls";
		lsCmd[1] = "-l";

		funcCallResult = execvp(lsCmd[0], lsCmd);

		//Error checks execution of "more -20".
		if (funcCallResult == -1){
			errnoTemp = errno;
			fprintf(stderr, "Exec Error: %s\n", strerror(errnoTemp));
			fflush(stderr);
			exit(errnoTemp);
		}
	}
}

void getServerFunction(int readfd, int datafd){
	char dataBuf[DATA_BUF_SIZE];
	int readSize, errnoTemp, writeRet;

	while((readSize = read(readfd, dataBuf, DATA_BUF_SIZE)) > 0){
		writeRet = write(datafd, dataBuf, readSize);
		if (writeRet == -1){
			errnoTemp = errno;
			fprintf(stderr, "Warning for Server Child %d: %s\n", getpid(), strerror(errnoTemp));
			fflush(stderr);
			break;
		}
	}
}

int isValidRegFile(char *path, int controlfd){
	struct stat statMem, *st = &statMem;
	int canParse, isRegFile, isReadable, errnoTemp;	

	//Checks existence of file.
	canParse = (stat(path, st)) == 0;
	if (!(canParse)){
		errnoTemp = errno;
		fprintf(stderr, "Server Child %d Suffered File Error: %s\n", getpid(), strerror(errnoTemp));
		fflush(stderr);
		writeErrorToClient(strerror(errnoTemp), controlfd);
		return errnoTemp;
	}

	//Checks file regularity.
	isRegFile = S_ISREG(st->st_mode);
	if (!isRegFile){
		fprintf(stderr, "Server Child %d Suffered File Error: %s\n", getpid(), "Requested file is not a regular file.");
		fflush(stderr);
		writeErrorToClient("Requested file is not a regular file.", controlfd);
		return 1;
	}

	//Checks file readability
	isReadable = (access(path, R_OK)) == 0;
	if (!isReadable){
		errnoTemp = errno;
		fprintf(stderr, "Server Child %d Suffered File Error: %s\n", getpid(), strerror(errnoTemp));
		fflush(stderr);
		writeErrorToClient(strerror(errnoTemp), controlfd);
		return errnoTemp;
	}

	return 0;
}

int readUntilNewline(int fd, char *bigBuf, int bufSize){
	char littleBuf[2] = {'\0'};
	int commandLength = 0;
	int characterWasRead;

	//Reads one character at a time to passed in buffer 
	// until newline is hit or maximum command size is hit.
	while (commandLength < bufSize){
		characterWasRead = read(fd, littleBuf, 1);

		//Detects if EOF was hit.
		if (!characterWasRead){
			return 0;
		}

		//Replaces newline with null terminator if newline is hit. 
		// Length is returned.
		if (littleBuf[0] == '\n'){
			bigBuf[commandLength] = '\0';
			return commandLength + 1;
		}
		bigBuf[commandLength] = littleBuf[0];
		commandLength++;
	}
	bigBuf[commandLength - 1] = '\0';
	return commandLength;
}

void putServerFunction(int writefd, int datafd){
	char dataBuf[DATA_BUF_SIZE];
	int readSize;

	//Reads from datafd to buffer, then writes from buffer to file.
	while((readSize = read(datafd, dataBuf, DATA_BUF_SIZE)) > 0){
		write(writefd, dataBuf, readSize);
	}
}

char *extractNameFromPath(char *path){
	//Plus one makes room for null terminator.
	char name[PATH_MAX + 1];
	char *current, *next;

	next = strtok(path, "/");

	//If null is returned this early, 
	// the path is just a relative file name.
	if ((next == NULL) || (strcmp(next, "") == 0)){
		return path;
	}

	//Searches through path until end path name has been found.
	while((next != NULL) && (strcmp(next, "") != 0)){
		current = next;
		next = strtok(NULL, "/");
	}

	return current;
}

void main(int argc, char const *argv[]){
	const char *portNum;
	int listenfd, errnoTemp, funcCallResult;

	//Sets port number based on if one was given.
	if (argc < 2){
		portNum = "49999";
	}else{
		portNum = argv[1];
	}

	//Checks to make sure port number given isn't invalid.
	if ((atoi(portNum) == 0) && (strcmp(portNum, "0") != 0)){
		fprintf(stderr, 
			"Fatal Format Error: %s\n", 
			"Invalid port number given.");
		fflush(stderr);
		exit(-1);
	}

	listenfd = serverSocket(portNum);

	//Error checks creation of server socket.
	if (listenfd == -1){
		exit(-1);
	}

	pid_t forkResult;
	
	//Creates connection queue for server of size 4.
	funcCallResult = listen(listenfd, 4);

	//Error checks listen.
	if (funcCallResult == -1){
		errnoTemp = errno;
		fprintf(stderr, "Fatal Error: %s\n", strerror(errnoTemp));
		fflush(stderr);
		exit(errnoTemp);
	}

	//Loop accepts client connections and handles them.
	while(1){		
		int controlfd, dataListenfd, dataLineFd;
		unsigned int length = sizeof(struct sockaddr_in);
		struct sockaddr_in clientControlAddress;

		//Connects a given client's control pipe.
		controlfd = accept(listenfd, 
			(struct sockaddr *)&clientControlAddress, 
			&length);

		//Error checks establishing client control connection.
		if (controlfd == -1){
			errnoTemp = errno;
			fprintf(stderr, "Accept Error: %s\n", strerror(errnoTemp));
			fflush(stderr);
			
		}else{
			forkResult = fork();

			//Error checks forking.
			if (forkResult == -1){
				errnoTemp = errno;
				fprintf(stderr, "Fatal Fork Error: %s\n", strerror(errnoTemp));
				fflush(stderr);
				
				writeErrorToClient(strerror(errnoTemp), controlfd);
				close(controlfd);
			}

			//Gets address info and purges zombies.
			if (forkResult > 0){
				char clientHostName[NI_MAXHOST];
				char *host;
				int hostNameGetResult;

				hostNameGetResult = getnameinfo((struct sockaddr *)&clientControlAddress,
					sizeof(clientHostName), 
					clientHostName,
					sizeof(clientHostName),
					NULL,
					0,
					NI_NUMERICSERV);

				//Error checks aquisition of client host name.
				if (hostNameGetResult != 0){
					host = "[Unknown Host]";
					errnoTemp = hostNameGetResult;
					fprintf(stderr, "Client Hostname Fetch Error: %s\n", gai_strerror(errnoTemp));
					fflush(stderr);
				}else{
					host = clientHostName;
				}

				fprintf(stdout, "Client: %d successfully connected from %s.\n", forkResult, host);
				fflush(stdout);

				//Cleans up zombies.
				while((waitpid(-1, NULL, WNOHANG)) > 0);
				close(controlfd);

			}

			//Interacts with the given client.
			if (forkResult == 0){
				//Buffer created to hold a command character, 
				// file pathway, and newline terminator.
				int serverCmdMax = 1 + PATH_MAX + 1;
				char cmdBuf[serverCmdMax];

				struct sockaddr_in serverAddress;
				memset(&serverAddress, 0, sizeof(serverAddress));
				length = sizeof(serverAddress);
				int socknameGetRes;

				//Number 6 chosen because port can only get up 
				// to 65535, a five digit integer. 
				// The sixth byte is for the null terminator.
				char dataPortNum[6];

				int portNumInt, controlStringLength, clientCommandLength;
				int dataLineExists = BOOL_FALSE;

				//Reads commands from client and performs them accordingly.
				while(1){
					clientCommandLength = readUntilNewline(controlfd, cmdBuf, serverCmdMax);

					//Accounts for if client unexpectedly died.
					if (clientCommandLength == 0){
						fprintf(stderr, "Fatal Error: Client %d %s\n", getpid(), "exited unexpectedly, exiting child process...");
						fflush(stderr);
						if (dataLineExists){
							close(dataLineFd);
						}
						close(controlfd);
						exit(-1);
					}

					//Prints command sent to server form client. 
					// Handles if command has path or not.
					fprintf(stdout, "\nClient %d Initiated Command %c", getpid(), cmdBuf[0]);
					if (clientCommandLength > 2){
						fprintf(stdout, " With Argument %s\n", cmdBuf + 1);
						fflush(stdout);
					}else{
						fprintf(stdout, "\n");
						fflush(stdout);
					}

					//Exit command expressed here.
					if (cmdBuf[0] == 'Q'){
						write(controlfd, "A\n", 2);
						fprintf(stdout, "Client: %d exited successfully.\n", getpid());
						fflush(stdout);
						close(controlfd);
						exit(0);
					}

					//This command establishes a data pipe to 
					// be used by rls, get, put, and show.
					else if (cmdBuf[0] == 'D'){
						dataListenfd = serverSocket("0");
						int hasErr = BOOL_FALSE;

						//Error checks creation of data line socket.
						if (dataListenfd == -1){
							errnoTemp = errno;
							writeErrorToClient(strerror(errnoTemp), controlfd);
							hasErr = BOOL_TRUE;
						}

						socknameGetRes = getsockname(dataListenfd, (struct sockaddr *)&serverAddress, &length);

						//Error checks filling out the address struct.
						if (socknameGetRes == -1){
							errnoTemp = errno;
							writeErrorToClient(strerror(errnoTemp), controlfd);
							hasErr = BOOL_TRUE;
						}

						if (!hasErr){
							//Gets port number in host byte order 
							// and turns it to a string.
							portNumInt = ntohs(serverAddress.sin_port);
							sprintf(dataPortNum, "%d", portNumInt);

							//Writes the acknowledgement string 
							// for the D command with the layout: A 2 3 4 2 \n 
							// (2342 being an example port).
							write(controlfd, "A", 1);
							write(controlfd, dataPortNum, strlen(dataPortNum));
							write(controlfd, "\n", 1);

							//Listens for and accepts connection from client.
							listen(dataListenfd, 1);
							dataLineFd = accept(dataListenfd, 
								(struct sockaddr *)&serverAddress, 
								&length);

							//Error checks connection failure.
							if (dataLineFd == -1){
								errnoTemp = errno;
								writeErrorToClient(strerror(errnoTemp), controlfd);
							}else{
								fprintf(stdout, "Client %d %s\n", getpid(), "D command successfully processed.");
								fflush(stdout);
								dataLineExists = BOOL_TRUE;
							}
						}
					}

					//Covers rls command.
					else if (cmdBuf[0] == 'L'){
						if (dataLineExists){
							errnoTemp = rlsServerLsFunction(dataLineFd);

							//Error checks server end of rls.
							if (errnoTemp != 0){
								fprintf(stderr, "Client %d Suffered rls Error: %s",getpid(), strerror(errnoTemp));
								writeErrorToClient(strerror(errnoTemp), controlfd);
							}else{
								fprintf(stdout, "Client %d %s\n", getpid(), "L command successfully processed.");
								fflush(stdout);

								write(controlfd, "A\n", 2);
								dataLineExists = BOOL_FALSE;
							}
						}else{
							writeErrorToClient("Use Error: Client needs to make data line before L command is sent.", controlfd);
						}
					}

					//Handles rcd command.
					else if (cmdBuf[0] == 'C'){
						changeServerDir(cmdBuf + 1, controlfd);
					}

					//Handles get command.
					else if (cmdBuf[0] == 'G'){ 
						if (dataLineExists){
							int readfd;

							//Checks if path leads to readable regular file.
							funcCallResult = isValidRegFile(cmdBuf + 1, controlfd);

							//Proceeds if provided file path is a valid regular file, 
							// otherwise writes error to client and closes data line.
							if (funcCallResult == 0){
								fprintf(stdout, "Server Child %d Reading File %s\n", getpid(), cmdBuf + 1);
								fflush(stdout);

								readfd = open(cmdBuf + 1, O_RDONLY, 0); 

								//Error checks opening file to read data.
								if (readfd == -1){
									errnoTemp = errno;
									writeErrorToClient(strerror(errnoTemp), controlfd);
									close(dataLineFd);
									dataLineExists = BOOL_FALSE;
								}else{
									fprintf(stdout, "Server Child %d Transferring File %s to Client\n",getpid(), cmdBuf + 1);
									fflush(stdout);									

									//Writes acknowledgement and closes necessary descriptors.
									write(controlfd, "A\n", 2);
									getServerFunction(readfd, dataLineFd);
									close(dataLineFd);
									close(readfd);
									dataLineExists = BOOL_FALSE;

									fprintf(stdout, "Client %d %s\n", getpid(), "G command successfully processed.");
									fflush(stdout);
								}
							}else{
								close(dataLineFd);
								dataLineExists = BOOL_FALSE;
							}
						}else{
							writeErrorToClient("Use Error: Client needs to make data line before G command is sent.", controlfd);
						}
					}

					//Handles put command.
					else if (cmdBuf[0] == 'P'){
						if (dataLineExists){
							char *fileName;
							int writefd;

							fileName = extractNameFromPath(cmdBuf + 1);

							if (fileName != NULL){
								fprintf(stdout, "Server Child %d Writing File %s\n", getpid(), fileName);
								fflush(stdout);

								writefd = open(fileName, O_CREAT | O_EXCL | O_WRONLY, 0744);

								//Error checks file creation.
								if (writefd == -1){
									errnoTemp = errno;
									writeErrorToClient(strerror(errnoTemp), controlfd);
									close(dataLineFd);
									dataLineExists = BOOL_FALSE;
								}
								else{
									fprintf(stdout, "Server Child %d Receiving File %s from Client\n", getpid(), fileName);
									fflush(stdout);

									write(controlfd, "A\n", 2);
									putServerFunction(writefd, dataLineFd);
									close(dataLineFd);
									close(writefd);
									dataLineExists = BOOL_FALSE;

									fprintf(stdout, "Client %d %s\n", getpid(), "P command successfully processed.");
									fflush(stdout);
								}
							}else{
								writeErrorToClient("Input Error: Name could not be extracted from path.", controlfd);
								close(dataLineFd);
								dataLineExists = BOOL_FALSE;
							}
						}else{
							writeErrorToClient("Use Error: Client needs to make data line before P command is sent.", controlfd);
						}
					}

					//Handles case of invalid server command.
					else{
						fprintf(stderr, "Server Child %d %s\n", getpid(), "Received Invalid Command");
						fflush(stderr);
						writeErrorToClient("Invalid command for Server.", controlfd);
					}
				}
			}
		}
	}
}
