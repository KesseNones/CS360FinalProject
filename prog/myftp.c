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

	//Error checks passed in return value of fork.
	if (retVal == -1){
		errnoTemp = errno;
		fprintf(stderr, "Fork Error: %s\n", strerror(errnoTemp));
		fflush(stderr);
		return errnoTemp;
	}

	return 0;
}

int closeErrorCheck(int retVal){
	int errnoTemp;

	//Error checks passed in return value of close.
	if (retVal != 0){
		errnoTemp = errno;
		fprintf(stderr, "Close Error: %s\n", strerror(errnoTemp));
		fflush(stderr);
		return errnoTemp;
	}

	return 0;
}

int dupErrorCheck(int retVal){
	int errnoTemp;

	//Error checks passed in return value of dup and dup2.
	if (retVal == -1){
		errnoTemp = errno;
		fprintf(stderr, "Dup Error: %s\n", strerror(errnoTemp));
		fflush(stderr);
		return errnoTemp;
	}

	return 0;
}

int isPathTooLong(char *path){
	if (strlen(path) > PATH_MAX){
		fprintf(stderr, "Path Size Error: %s\n", "Path is too big. Try a smaller path.");
		fflush(stderr);
		return 1;
	}else{
		return 0;
	}
}

int localListFunc(){
	int fd[2];
	int errnoTemp, forkRes, funcCallResult, waitResult;

	//Pipes and error checks pipe.
	if (pipe(fd) < 0){
		errnoTemp = errno;
		fprintf(stderr, "Pipe Error: %s\n", strerror(errnoTemp));
		fflush(stderr);
		return errnoTemp;
	}

	//Forks and error checks fork.
	forkRes = fork();
	errnoTemp = forkErrorCheck(forkRes);
	if (errnoTemp != 0) return(errnoTemp);

	//Parent waits for child and child does piping.
	if (forkRes){
		//Closes pipe ends that are not used and error checks their closing.
		errnoTemp = closeErrorCheck(close(fd[0]));
		if (errnoTemp != 0) return(errnoTemp);
		errnoTemp = closeErrorCheck(close(fd[1]));
		if (errnoTemp != 0) return(errnoTemp);
		
		wait(&waitResult);
		return waitResult;
	}

	//Performs pipes and execs.
	else{
		//Forks and error checks.
		forkRes = fork();
		errnoTemp = forkErrorCheck(forkRes);
		if (errnoTemp != 0) exit(errnoTemp);

		//Fork branch used for executing ls and more pipe command.
		if (forkRes){
			int reader = fd[0];
			int writer = fd[1];

			//Closes writing end of pipe and error checks close.
			errnoTemp = closeErrorCheck(close(writer));
			if (errnoTemp != 0) exit(errnoTemp);
			
			//Closes stdin and error checks.
			errnoTemp = closeErrorCheck(close(0));
			if (errnoTemp != 0) exit(errnoTemp);

			//Makes it so pipe reads from stdin. //POOPY
			errnoTemp = dupErrorCheck(dup(reader));
			if (errnoTemp != 0) exit(errnoTemp);

			//Closes original reader end of pipe and error checks.
			errnoTemp = closeErrorCheck(close(reader));
			if (errnoTemp != 0) exit(errnoTemp);

			//String array used in execvp to execute "more -20".
			char *moreCmd[3] = {NULL, NULL, NULL};
			moreCmd[0] = "more";
			moreCmd[1] = "-20";

			funcCallResult = execvp(moreCmd[0], moreCmd);

			//Error checks execution of "more -20".
			if (funcCallResult == -1){
				errnoTemp = errno;
				fprintf(stderr, "Exec Error: %s\n", strerror(errnoTemp));
				fflush(stderr);
				exit(errnoTemp);
			}

		}else{
			int reader = fd[0];
			int writer = fd[1];

			//Closes reading end of pipe and error checks.
			errnoTemp = closeErrorCheck(close(reader));
			if (errnoTemp != 0) exit(errnoTemp);
			
			//Closes stdout and error checks.
			errnoTemp = closeErrorCheck(close(1));
			if (errnoTemp != 0) exit(errnoTemp);

			//Error checks having pipe write to stdout.
			errnoTemp = dupErrorCheck(dup(writer));
			if (errnoTemp != 0) exit(errnoTemp);

			//Closes original writer fd and error checks.
			errnoTemp = closeErrorCheck(close(writer));
			if (errnoTemp != 0) exit(errnoTemp);

			//String array used in execvp to execute the "ls -l" command.
			char *lsCmd[3] = {NULL, NULL, NULL};
			lsCmd[0] = "ls";
			lsCmd[1] = "-l";

			funcCallResult = execvp(lsCmd[0], lsCmd);

			//Error checks execution of "ls -l".
			if (funcCallResult == -1){
				errnoTemp = errno;
				fprintf(stderr, "Exec Error: %s\n", strerror(errnoTemp));
				fflush(stderr);
				exit(errnoTemp);
			}
		}
	}
}

void changeClientDir(char *path){
	int errnoTemp, cdRetVal;

	cdRetVal = chdir(path);

	//Error checks changing directories.
	if (cdRetVal != 0){
		errnoTemp = errno;
		fprintf(stderr, "Change Client Dir Error: %s\n", strerror(errnoTemp));
		fflush(stderr);
	}
}

char *cmdD(char *controlBuf, int socketfd){
	int readLength, errnoTemp;
	char *portString;

	write(socketfd, "D\n", 2);
	readLength = readUntilNewline(socketfd, controlBuf, CONTROL_BUF_SIZE);

	if (readLength == 0){
		fprintf(stderr, "Fatal Error: %s\n", "Control socket unexpectedly closed, exiting...");
		fflush(stderr);
		close(socketfd);
		exit(-1);
	}

	//If there wasn't an error on the server end,
	// copy the port characters over to the port string to be returned.
	//If there was an error, print that and return NULL.
	if (controlBuf[0] == 'A'){
		portString = (char *)malloc(readLength - 1);
		
		//Error checks malloc.
		if (portString == NULL){
			fprintf(stderr, 
				"Malloc Error: %s\n", 
				"Failed to allocate memory for port string.");
			fflush(stderr);
			return NULL;
		}

		//Plus and minus one operations used to ignore the 'A' 
		// at the beginning of controlBuf.
		portString = strncpy(portString, controlBuf + 1, readLength - 1);

		return portString;

	}else{
		//Writes error message to stdout if one occured.
		write(1, controlBuf + 1, readLength - 1);
		fflush(stdout);
		return NULL;
	}
}

int clientToServerConnect(const char *port, const char *address){
	int socketfd, errnoTemp, funcCallResult, err;
	struct addrinfo hints, *actualdata;
	memset(&hints, 0, sizeof(hints));

	hints.ai_socktype = SOCK_STREAM;
	hints.ai_family = AF_INET;

	err = getaddrinfo(address,
		port, 
		&hints, 
		&actualdata);

	//Error checks getting info from server address.
	if (err != 0){
		errnoTemp = err;
		fprintf(stderr, "Address Fetch Error: %s\n", gai_strerror(errnoTemp));
		fflush(stderr);
		return -1;
	}

	socketfd = socket(actualdata->ai_family, 
		actualdata->ai_socktype, 
		0);

	//Error checks socket creation.
	if (socketfd == -1){
		errnoTemp = socketfd;
		fprintf(stderr, "Socket Creation Error: %s\n", gai_strerror(errnoTemp));
		fflush(stderr);
		return -1;
	}

	err = connect(socketfd, 
		actualdata->ai_addr, 
		actualdata->ai_addrlen);

	//Error checks connecting to server.
	if (err < 0){
		errnoTemp = errno;
		fprintf(stderr, "Connection Error: %s\n", strerror(errnoTemp));
		fflush(stderr);
		return -1;
	}

	//Empties out address info struct after use has expired.
	freeaddrinfo(actualdata);

	return socketfd;
}

int moreFunction(int datafd){
	int errnoTemp, funcCallResult, forkRes, waitResult;

	forkRes = fork();

	//Forks and error checks.
	errnoTemp = forkErrorCheck(forkRes);
	if (errnoTemp != 0) return errnoTemp;

	//Parent waits for child and closes data pipe.
	if (forkRes){
		wait(&waitResult);
		closeErrorCheck(close(datafd));
		return waitResult;
	}else{
		//Makes it so "more -20" reads from the datafd pipe.
		errnoTemp = dupErrorCheck(dup2(datafd, 0));
		if (errnoTemp != 0) exit(errnoTemp);

		//String array used in execvp to execute "more -20".
		char *moreCmd[3] = {NULL, NULL, NULL};
		moreCmd[0] = "more";
		moreCmd[1] = "-20";

		funcCallResult = execvp(moreCmd[0], moreCmd);

		//Error checks execution of "more -20".
		if (funcCallResult == -1){
			errnoTemp = errno;
			fprintf(stderr, "Exec Error: %s\n", strerror(errnoTemp));
			fflush(stderr);
			exit(errnoTemp);
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
		fprintf(stderr, "File Error: %s\n", strerror(errnoTemp));
		fflush(stderr);
		return errnoTemp;
	}

	//Checks file regularity.
	isRegFile = S_ISREG(st->st_mode);
	if (!isRegFile){
		fprintf(stderr, "File Error: %s\n", "Requested file is not a regular file.");
		fflush(stderr);
		return 1;
	}

	//Checks file readability
	isReadable = (access(path, R_OK)) == 0;
	if (!isReadable){
		errnoTemp = errno;
		fprintf(stderr, "File Error: %s\n", strerror(errnoTemp));
		fflush(stderr);
		return errnoTemp;
	}

	return 0;
}

void getClientFunction(int writefd, int datafd){
	char dataBuf[DATA_BUF_SIZE];
	int readSize;

	while((readSize = read(datafd, dataBuf, DATA_BUF_SIZE)) > 0){
		write(writefd, dataBuf, readSize);
	}
}

char *extractNameFromPath(char *path){
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

int readUntilNewline(int fd, char *bigBuf, int bufSize){
	char littleBuf[2] = {'\0'};
	int commandLength = 0;
	int characterWasRead;

	//Reads one character at a time to passed in buffer 
	// until newline is hit, or maximum command size is hit, or EOF is hit.
	while (commandLength < bufSize){
		characterWasRead = read(fd, littleBuf, 1);

		//Detects if EOF was hit.
		if (!characterWasRead){
			return 0;
		}

		//If newline was hit, replace newline 
		// with null terminator and return size.
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

void putClientFunction(int readfd, int datafd){
	char dataBuf[DATA_BUF_SIZE];
	int readSize;

	while((readSize = read(readfd, dataBuf, DATA_BUF_SIZE)) > 0){
		write(datafd, dataBuf, readSize);
	}
}

int isValidArgs(char **commandChunks, int isTwoTokenCommand){
	int index;

	//Input sanitization.
	if (commandChunks == NULL){
		return 0;
	}	
	if ((isTwoTokenCommand > 1) || (isTwoTokenCommand < 0)){
		return 0;
	}

	index = isTwoTokenCommand;

	//Single token case: If command not empty and not just a newline, 
	// replace newline with null terminator and return 1. Return 0 otherwise.
	//Multi token case: If second token of command 
	// is not empty and not just a newline, 
	// relace newline with null terminator and return 1. Return 0 otherwise.  
	if ((commandChunks[index] != NULL) && (strcmp(commandChunks[index], "\n") != 0)){
		//Replaces newline with null terminator if it exists.
		if (commandChunks[index][strlen(commandChunks[index]) - 1] == '\n'){
			commandChunks[index][strlen(commandChunks[index]) - 1] = '\0';
		}
		return 1;
	}else{
		return 0;
	}
}

int establishDatapipe(char *controlBuf, int controlfd, const char *address){
	char *dataPortNum;
	int datafd;

	dataPortNum = cmdD(controlBuf, controlfd);
	if (dataPortNum != NULL){
		datafd = clientToServerConnect(dataPortNum, address);
		free(dataPortNum);
		return datafd;
	}else{
		return -1;
	}
}

void main(int argc, char const *argv[]){
	//Error checks no ip address or portnumber for a server being provided.
	// Provides usage format.
	if (argc < 3){
		fprintf(stderr, "Arg Count Error: %s\n", "Too few arguments given.");
		fprintf(stderr, "Program Format: %s\n", "./myftp <Port Number> <Hostname | IP Address>");
		exit(-1);
	}

	//argv[1] is port number, argv[2] is address.
	int socketfd = clientToServerConnect(argv[1], argv[2]);

	//Error checks socket creation. If error happens, 
	// program format stated and program exits.
	if (socketfd == -1){
		fprintf(stderr, "Program Format: %s\n", "./myftp <Port Number> <Hostname | IP Address>");
		exit(-1);
	} 

	int controlStatementSize, errnoTemp, funcCallResult;
	char controlBuf[CONTROL_BUF_SIZE];

	fprintf(stdout, "Successfully connected to server: %s\n", argv[2]); 

	//Max size of command based on five command characters, 
	// a pathmax sized path, and a newline.
	int maxCommandSize = 5 + PATH_MAX + 1;
	char commandBuf[maxCommandSize];

	char *commandChunks[2] = {NULL, NULL};
	int commandSize, datafd, isSingleToken;
	char *dataPortNum;

	//Takes in user commands and calls functions.
	while(1){
		//Prompts user for command.
		fprintf(stdout, "%s", "<CMD>/:");
		fflush(stdout);
	
		//Reads user command and null terminates it.
		commandSize = read(0, commandBuf, maxCommandSize);
		commandBuf[commandSize - (commandSize == maxCommandSize)] = '\0';

		//Max two command tokens acquired. Remaining tokens ignored.
		commandChunks[0] = strtok(commandBuf, " ");
		commandChunks[1] = strtok(NULL, " ");

		//Determines if user entered command contains one or two tokens.
		isSingleToken = ((commandChunks[1] == NULL) || (strcmp(commandChunks[1], "") == 0));

		//Replaces newline at end of single token command if it exists.
		if (isSingleToken && ((strlen(commandChunks[0])) > 1)){
			if (commandChunks[0][strlen(commandChunks[0]) - 1] == '\n'){
				commandChunks[0][strlen(commandChunks[0]) - 1] = '\0';
			}	
		}

		if (strcmp(commandChunks[0], "cd") == 0){
			if (isValidArgs(commandChunks, BOOL_TRUE)){
				
				//If the path isn't too long, change directory to path.
				if (!isPathTooLong(commandChunks[1])){
					changeClientDir(commandChunks[1]);
				}

			}else{
				fprintf(stderr, "Command Error: %s\n", "No second parameter for cd given.");
				fflush(stderr);
			}
		}else if ((strcmp(commandChunks[0], "rcd")) == 0){
			//If there is a second argument for the rcd command, move on, 
			// otherwise, print command error.
			if (isValidArgs(commandChunks, BOOL_TRUE)){
				if (!isPathTooLong(commandChunks[1])){
					//Writes rcd command to server.
					write(socketfd, "C", 1);
					write(socketfd, commandChunks[1], strlen(commandChunks[1]));
					write(socketfd, "\n", 1);

					controlStatementSize = readUntilNewline(socketfd, controlBuf, CONTROL_BUF_SIZE);

					//Prints error if one occured on server end.
					if (controlBuf[0] == 'E'){
						write(1, controlBuf + 1, controlStatementSize - 1);
						write(1, "\n", 1);
						fflush(stdout);
					}
				}
			}else{
				fprintf(stderr, "Command Error: %s\n", "No second parameter for rcd given.");
				fflush(stderr);
			}
		}else if ((strcmp(commandChunks[0], "get")) == 0){
			if (isValidArgs(commandChunks, BOOL_TRUE)){
				if (!isPathTooLong(commandChunks[1])){
					//Gets name form path and duplicates path string for writing to server.
					char *pathName = strdup(commandChunks[1]);
					char *fileName = extractNameFromPath(commandChunks[1]);

					int writefd;

					if ((fileName != NULL) && (pathName != NULL)){
						writefd = open(fileName, O_CREAT | O_EXCL | O_WRONLY, 0700);

						if (writefd == -1){
							errnoTemp = errno;
							fprintf(stderr, "File Creation Error: %s\n", strerror(errnoTemp));
							fflush(stderr);
							free(pathName);
						}
						else{
							datafd = establishDatapipe(controlBuf, socketfd, argv[2]);
							if (datafd != -1){
								write(socketfd, "G", 1);
								write(socketfd, pathName, strlen(pathName));
								write(socketfd, "\n", 1);
								controlStatementSize = readUntilNewline(socketfd, controlBuf, CONTROL_BUF_SIZE);

								//Runs client end of put command if server is ready, 
								// otherwise writes error from server 
								// and closes necessary descriptors.
								if (controlBuf[0] == 'A'){
									getClientFunction(writefd, datafd);
									close(writefd);
									close(datafd);
								}else{
									write(1, controlBuf + 1, controlStatementSize - 1);
									write(1, "\n", 1);
									fflush(stdout);
									close(datafd);
									unlink(fileName);
								}
							}
							free(pathName);
						}
					}else{
						fprintf(stderr, 
							"Input/StrDup Error: %s\n", 
							"Name could not be extracted from path or path string could not be duplicated.");
						fflush(stderr);

						//Frees pathName if it was successfully duplicated.
						if (pathName != NULL){
							free(pathName);
						}
					}
				}
			}else{
				fprintf(stderr, "Command Error: %s\n", "No second parameter for get given.");
				fflush(stderr);
			}
		}else if ((strcmp(commandChunks[0], "show")) == 0){
			if (isValidArgs(commandChunks, BOOL_TRUE)){
				if (!isPathTooLong(commandChunks[1])){
					datafd = establishDatapipe(controlBuf, socketfd, argv[2]);
					if (datafd != -1){
						write(socketfd, "G", 1);
						write(socketfd, commandChunks[1], strlen(commandChunks[1]));
						write(socketfd, "\n", 1);
						controlStatementSize = readUntilNewline(socketfd, controlBuf, CONTROL_BUF_SIZE);

						//Runs client end of put command if server is ready, 
						// otherwise writes error from server 
						// and closes necessary descriptors.
						if (controlBuf[0] == 'A'){
							moreFunction(datafd);
							close(datafd);
						}else{
							write(1, controlBuf + 1, controlStatementSize - 1);
							write(1, "\n", 1);
							fflush(stdout);
							close(datafd);
						}
					}
				}
			}else{
				fprintf(stderr, 
					"Error: %s\n", 
					"Name could not be extracted from path or path string could not be duplicated.");
				fflush(stderr);
			}
		}else if ((strcmp(commandChunks[0], "put")) == 0){
			if (isValidArgs(commandChunks, BOOL_TRUE)){
				int readfd;

				if (!isPathTooLong(commandChunks[1])){
					funcCallResult = isValidRegFile(commandChunks[1], 0);

					//Moves on if the path given is a valid regular file to be read.
					if (funcCallResult == 0){
						readfd = open(commandChunks[1], O_RDONLY, 0);

						//Error checks open.
						if (readfd == -1){
							errnoTemp = errno;
							fprintf(stderr, "Open Error: %s\n", strerror(errnoTemp));
							fflush(stdout);
						}else{
							datafd = establishDatapipe(controlBuf, socketfd, argv[2]);

							//Move on if data connection could be established.
							if (datafd != -1){
								//Sends P command with path and waits for server response.
								write(socketfd, "P", 1);
								write(socketfd, commandChunks[1], strlen(commandChunks[1]));
								write(socketfd, "\n", 1);
								controlStatementSize = readUntilNewline(socketfd, controlBuf, CONTROL_BUF_SIZE);

								//Runs client end of put command if server is ready, 
								// otherwise writes error from server 
								// and closes necessary descriptors.
								if (controlBuf[0] == 'A'){
									putClientFunction(readfd, datafd);
									close(readfd);
									close(datafd);
								}else{
									write(1, controlBuf + 1, controlStatementSize - 1);
									write(1, "\n", 1);
									fflush(stdout);
									close(datafd);
								}
							}	
						}
					}	
				}
			}else{
				fprintf(stderr, "Command Error: %s\n", "No second parameter for put given.");
				fflush(stderr);
			}
		}else if ((strcmp(commandChunks[0], "exit")) == 0){
			if (isValidArgs(commandChunks, BOOL_FALSE)){
				//Writes quit command to server and reads response.
				write(socketfd, "Q\n", 2);
				controlStatementSize = readUntilNewline(socketfd, controlBuf, CONTROL_BUF_SIZE);

				//Exits after printing error if server suddenly stopped.
				if (controlStatementSize == 0){
					fprintf(stderr, "Fatal Error: %s\n", "Control socket unexpectedly closed, exiting...");
					fflush(stderr);
					close(socketfd);
					exit(-1);
				}

				//Exits when server side is done. 
				// Writes error from server if something went wrong.
				if (controlBuf[0] == 'A'){
					close(socketfd);
					exit(0);
				}else{
					write(1, controlBuf + 1, controlStatementSize - 1);
					write(1, "\n", 1);
					close(socketfd);
					fflush(stdout);
				}
			}
		}else if ((strcmp(commandChunks[0], "ls")) == 0){
			if ((isValidArgs(commandChunks, BOOL_FALSE))){
				funcCallResult = localListFunc();

				//Error checks ls.
				if (funcCallResult != 0){
					fprintf(stderr, "Ls error: %s\n", "ls command failed.");
					fflush(stderr);
				}
			}
		}else if ((strcmp(commandChunks[0], "rls")) == 0){
			if (isValidArgs(commandChunks, BOOL_FALSE)){
				datafd = establishDatapipe(controlBuf, socketfd, argv[2]);

				if (datafd != -1){
					write(socketfd, "L\n", 2);
					controlStatementSize = readUntilNewline(socketfd, controlBuf, CONTROL_BUF_SIZE);

					//Executes "more -20" if ls executed successfully on the server.
					// Prints error and closes data line client end if not successful.
					if (controlBuf[0] == 'A'){
						moreFunction(datafd);
						close(datafd);
					}else{
						write(1, controlBuf + 1, controlStatementSize - 1);
						write(1, "\n", 1);
						fflush(stdout);
						close(datafd);
					}
				}
			}
		}else{
			//If an invalid command was entered, print error.
			if (commandChunks[0][0] != '\n'){
				fprintf(stderr, "Command Error: %s\n", "Unknown command.");
				fflush(stderr);
			}
		}
	}
}
