# primary make rule: doesnt create any file but needed to build target file                                                                              
all: dev server #reset gdb
 
# make rule for client file 
client: dev.o 
	gcc -Wall dev.o -o dev 
 
# make rule for server file
server: server.o 
	gcc -Wall server.o -o server 

 
# remove ojbect file reate durign compilation ('make reset' to execute)
reset: 
	rm *o dev server
	rm *txt
	rmdir -r pending_messages

gdb:
	gcc -g -Wall dev.c -o dev
	gcc -g -Wall server.c  -o server 
 