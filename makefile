# primary make rule: doesnt create any file but needed to build target file                                                                              
all: dev server  
 
# make rule for client file 
client: dev.o 
	gcc -Wall dev.c -o dev 
 
# make rule for server file
server: server.o 
	gcc -Wall server.c -o server 

 
# remove ojbect file reate durign compilation ('make reset' to execute)
reset: 
	rm *o dev server

gdb:
	gcc -g -Wall dev.c -o dev
	gcc -g -Wall server.c  -o server 
 