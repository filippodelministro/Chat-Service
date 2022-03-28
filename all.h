#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>

#define BUFFER_SIZE         1024
#define MSG_LEN             9

#define MAX_DEVICES         100    

int listener;
int my_port;
struct sockaddr_in my_addr;


// File descriptor table utility
int fdmax;
fd_set master;
fd_set read_fds;
fd_set write_fds;


//-----------     DEVICE    -----------------
struct device{
    int port;           // port number       
    int sd;             // TCP socket
}devices[MAX_DEVICES];  //devices array

// int n_dev = 0;                  //number of devices

