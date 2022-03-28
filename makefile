# make rule primaria con dummy target ‘all’--> non crea alcun file all ma fa un complete build 
#                                                                          che dipende dai target client e server scritti sotto  
all: dev server  
 
# make rule per il client 
client: dev.o 
	gcc -Wall dev.o -o dev 
 
# make rule per il server 
server: server.o 
	gcc -Wall server.o -o server 
 
 
# pulizia dei file della compilazione (eseguito con ‘make clean’ da terminale) 
reset: 
	rm *o dev server