# CS 360 (Systems Programming) Final Project
# Jesse Jones
# Completed mid December 2022

## Description
This was the final project for CS 360 Fall Semester of 2022. It involves a server and client 
and N clients that can talk to each other and perform IO and file transfers and stuff like that.

The program itself was written as a multithreaded program in C which makes use of sockets for networking.

Unfortunately, this project was made before I widely used version control so it's just the final version that's part of the initial commit.

## Using
### Compiling
Make sure to have gcc installed as well as make so the make file can do its job.

### Running
1. Run the command: `./myftpserve [PORT_NUMBER]`
where `PORT_NUMBER` represents a port number you want the server 
to listen on for clients to connect to.

2. If you're connecting the client to the server on a different machine, note the server's IP address, 
otherwise you can just use `localhost` if it's on the same machine.

3. Run the command: `./myftp [PORT_NUMBER] [HOST_NAME | IP_ADDRESS]` where `PORT_NUMBER` is the port the 
server's listening on, and `HOST_NAME | IP_ADDRESS` is the host name or ip address of the machine the server
is running on.

4. If successfully connected the server side will read something like: <br>
`Client: 743 successfully connected from localhost.`
This message shows the process id of the client 
and what IP it connected from, in this case localhost. <br> <br>
On the client side, you will see something like this text:
```
Successfully connected to server: localhost
<CMD>/:
```
Which states the client connected to the server at the given IP address 
followed by the command prompt `<CMD>/:`

5. Once the connection has formed, you can make use of all the existing server 
and client commands to explore the functionality implemented in this project.

### Commands

#### Server - Side



#### Client - Side