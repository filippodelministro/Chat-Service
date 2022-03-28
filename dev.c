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

// int n_dev = 0;                  //number of devices


//maybe in an unic extern file utility.c            ???
//////////////////////////////////////////////////////////////////////////
///                             UTILITY                               ///
//////////////////////////////////////////////////////////////////////////

void prompt()
{
	printf("\n> ");
    fflush(stdout);
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


//////////////////////////////////////////////////////////////////////////
///                             FUNCTION                              ///
//////////////////////////////////////////////////////////////////////////


//prompted at boot of each device
void boot_command(){
    printf( "Create an account or login to continue:\n\n"
                "1) signup <username> <password>        --> create account\n"
                "2) in <srv_port> <username> <password> --> connect to server\n"
    );
}

void help_command()
{
	printf( "Type a command:\n\n"
            "1) hanging                             --> receive old msg\n"
            "2) show                                --> ??\n"
            "3) chat                                --> ??\n"
            "4) share                               --> ??\n"
            "5) out                                 --> ??\n"
    );
}

void get_command(){

}

int main(int argc, char** argv)
{
    int i, sd, ret;
    socklen_t addrlen;
    struct sockaddr_in srv_addr;

    char buffer[BUFFER_SIZE];

    /*                              ???
    if(argc != 2){
		fprintf(stderr, "Uso: ./dev <porta>\n"); 
		exit(-1);
    }
    */

    my_port = atoi(argv[1]);        //???

	fdt_init();
    create_listening_socket(true, my_port);

    // Messaggio iniziale
    printf("**********************PEER %d**********************\n", my_port);
	boot_command();
    prompt();

    while(1){
        read_fds = master;
        
        if(select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
			perror("ERROR SELECT()");
			exit(-1);
		}

        for(i=0; i<fdmax; i++){
            if(FD_ISSET(i, &read_fds)){
                if(i == 0){
                    //no socket ready: get a command ???
                    get_command();
                }
                /* all cases for a ready socket:
                    - server contact me
                    - other device contact me
                    - else?
                else if(i == listener){
                    //the server conctacted me
                }
                else if(){
                    //another device contacted me
                }
                */
            }
        }
    }
}