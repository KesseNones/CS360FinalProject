//Jesse A. Jones
//15 Dec, 2022
//CS 360
//Final Project
//V: 1.14

#ifndef FTP_H
#define FTP_H

#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/stat.h>
#include<sys/wait.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<netdb.h>
#include<errno.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<ctype.h>
#include<dirent.h>
#include<fcntl.h>

#define CONTROL_BUF_SIZE 1024
#define DATA_BUF_SIZE 4096

#define BOOL_TRUE 1
#define BOOL_FALSE 0

//This function executes ls and displays 
// up to 20 lines of the result at a time.
//Returns 0 on success and an error number on failure.
int localListFunc();

//Error checks calls to fork by assessing the return value 
// and printing error message if return value indicated an error.
//Returns 0 on success and error number on failure.
int forkErrorCheck(int retVal);

//Error checks calls to close by looking at the return value
// and printing error message if return value indicated an error.
//Returns 0 on success and error number on failure.
int closeErrorCheck(int retVal);

//Error checks calls to dup function 
// printing an error message if return value indicated an error.
//Returns 0 on success and error number on failure.
int dupErrorCheck(int retVal);

//Examines the size of the path to see if it's too big.
// Writes an error to the console if the path is too big.
//Returns 1 if the path is too large and 0 if not.
int isPathTooLong(char *path);

//Changes location of client process to passed in path, if valid.
void changeClientDir(char *path); 

//Changes directory of server child process representing client, 
// if passed in path is valid. 
//Returns 0 on success, and error number on failure.
int changeServerDir(char *path, int controlfd);

//Sends the D command to the server and performs the necessary error checks.
//Returns an ascii encoded integer representing 
// the data line port the server will listen on.
//Returns NULL on error.
//If server unexpectedly quit, the client exits in this function.
char *cmdD(char *controlBuf, int socketFd);

//Makes it so the server listens on the passed in port number for connections, 
// either for the control pipe or data pipe.
//Returns a fd of the socket on success, -1 on error, 
// and -2 on specific bind error.
int serverSocket(const char *portNum);

//Attempts to connect to passed in port number and address. 
// Used for creation of data line 
// and control line between server and client.
//Returns file descriptor to connection on success and -1 on failure.
int clientToServerConnect(const char *port, const char *address);

//Writes the passed in error string 
// to the client via the passed in fd.
void writeErrorToClient(const char *errorString, int controlfd);

//This function reads from the passed in data pipe 
// and prints twenty lines at a time to stdout.
//Returns 0 on success and error number on failure.
int moreFunction(int datafd);

//Writes result of execing ls -l to datafd.
//Returns 0 on success, error number on failure.
int rlsServerLsFunction(int datafd);

//Reads from passed in readfd and writes 
// to datafd at most DATA_BUF_SIZE bytes at a time.
void getServerFunction(int readfd, int datafd);

//Checks to see if the file of a passed in path is a readable regular file.
//Returns 0 on success, returns an error number on failure.
//The server version also writes an error message 
// to the client specific to the failure case.
int isValidRegFile(char *path, int controlfd);

//Reads at most DATA_BUF_SIZE bytes at 
// a time from datafd and writes to writefd.
void getClientFunction(int writefd, int datafd);

//Extracts the filename from a path passed in.
//Returns a file name if a name could be extracted from the path, NULL if not.
char *extractNameFromPath(char *path);

//Reads from passed in fd and writes 
// to passed in buffer until a newline is read 
// or passed in buffer size is reached.
//Returns number of characters read in total.
//If no newline is found, function reads until EOF and returns 0.
int readUntilNewline(int fd, char *bigBuf, int bufSize);

//Reads from passed in readfd and writes 
// to datafd at most DATA_BUF_SIZE bytes at a time.
void putClientFunction(int readfd, int datafd);

//Reads at most DATA_BUF_SIZE bytes at 
// a time from datafd and writes to writefd.
void putServerFunction(int writefd, int datafd);

//SIGNLE TOKEN CASE: Checks to see if command is empty or just a newline.
// If the token is not empty and not a newline, 
// return 1 after replacing newline with null terminator.
// Otherwise, return 0.
//MULTI TOKEN CASE: Checks to see if the second token 
// of the command is empty or just a newline.
// If the second token is not empty/a newline,
// return 1 after replacing newline with null terminator.
// Otherwise return 0. 
int isValidArgs(char **commandChunks, int isTwoTokenCommand);

//Makes the necessary function calls to establish 
// a data pipe between the server and client on the client side.
//Returns a fd representing the data pipe on success, and -1 on failure.
int establishDatapipe(char *controlBuf, int controlfd, const char *address);

#endif
