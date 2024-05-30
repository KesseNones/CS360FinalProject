CPP = gcc
FLAGS = -Wall -g

default:prime

clean:
	rm -f myftpserve
	rm -f myftp

prime: myftpserve myftp

serverExec:
	${CPP} ${FLAGS} -o myftpserve myftpserve.c myftp.h

clientExec:
	${CPP} ${FLAGS} -o myftp myftp.c myftp.h
