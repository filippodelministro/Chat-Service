#include "all.h"
// #include "utility.c"

//////////////////////////////////////////////////////////////////////////
///                             DECLARATION                            ///
//////////////////////////////////////////////////////////////////////////

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

int n_dev = 0;                  //number of devices

//maybe in an unic extern file utility.c            ???
//////////////////////////////////////////////////////////////////////////
///                             UTILITY                               ///
//////////////////////////////////////////////////////////////////////////

void prompt()
{
	printf("\n> ");
    fflush(stdout);
}    

void boot_message(){
    printf("**********************SERVER STARTED**********************\n" 
                "Type a command:\n\n"
                "1) help --> show command details\n"
                "2) list --> show connected users list\n"
                "3) esc  --> close the server\n" 
    );
}

void fdt_init() {       
	FD_ZERO(&master);
	FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);
	FD_SET(0, &master);
	
	fdmax = 0;
}

//create a socket: TCP if tcp is true, UDP instead
void create_listening_socket(bool tcp, char* port){
    
    //create
    if((listener = socket(AF_INET, (tcp) ? SOCK_STREAM : SOCK_DGRAM, 0)) == -1){
        perror("socket() error");
        exit(-1);
    }

    //address
    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = INADDR_ANY;
    my_addr.sin_port = htons(atoi(port));

    //connection
    if(bind(listener, (struct sockaddr*)&my_addr, sizeof(my_addr)) == -1){
        perror("[server]: ERROR BIND: ");
        exit(-1);
    }
}


int main(int argc, int argv[]){
    int i, sd, ret, len, new_sd;
    socklen_t addrlen;
    struct sockaddr_in cl_addr[MAX_DEVICES];    //array of socket

    char buffer[BUFFER_SIZE];

/*          ???
    if(argc != 2){
		fprintf(stderr, "Uso: ./dev <porta>\n"); 
		exit(-1);
    }
*/

    //inizializzazione ???
	// fdt_init();

    boot_message();	
    prompt();

    create_listening_socket(true, "4242");

    while(1){   
        len = sizeof(cl_addr);
        new_sd = accept(sd, (struct sockaddr*)&cl_addr, &len);

        strcpy(buffer, "Hello");
        len = strlen(buffer);

        ret = send(new_sd, (void*)buffer, len, 0);
        if(ret < 0)
            perror("[server]: Errore nella send");
        
        close(new_sd);

    }
}