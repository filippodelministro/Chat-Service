#include "all.h"
// #include "utility.c"

//////////////////////////////////////////////////////////////////////////
///                             DECLARATION                            ///
//////////////////////////////////////////////////////////////////////////


// File descriptor table utility
int fdmax;
fd_set master;
fd_set read_fds;
fd_set write_fds;

//-----------     DEVICE    -----------------
int my_port;
bool connected;
struct sockaddr_in my_addr;

//each device has a port and a socket descriptor
struct device{
    int port;           // port number       
    int sd;             // TCP socket
}devices[MAX_DEVICES];  //devices array

//each device socket has an address
struct sockaddr_in dev_addr[MAX_DEVICES];   //device address
int n_dev = 0;                              //number of devices

//socket which listen connect request from other devices
int listener;

//each device has a socket to the server
int srv_socket[2];              //0: TCP, 1:UDP
struct sockaddr_in srv_addr;

//maybe in an unic extern file utility.c            ???
//////////////////////////////////////////////////////////////////////////
///                             UTILITY                               ///
//////////////////////////////////////////////////////////////////////////

//prompt '>' character on stdout
void prompt()
{
	printf("\n> ");
    fflush(stdout);
}    

//inizialize fd_set
void fdt_init() {       
	FD_ZERO(&master);
	FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);
	FD_SET(0, &master);
	
	fdmax = 0;
}

//create a socket a srv_socket for who call the functionon port p; TCP if udp is false, UDP instead
//it call socket and connect
void create_srv_socket(bool udp, char* p){

    //create
    if((srv_socket[udp] = socket(AF_INET, (udp) ? SOCK_STREAM : SOCK_DGRAM, 0)) == -1){
        perror("socket() error");
        exit(-1);
    }

    //address
    memset(&srv_addr, 0, sizeof(srv_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(atoi(p));
    my_addr.sin_addr.s_addr = INADDR_ANY;

    /* no needed in client
    //bind
    if(bind(srv_socket, (struct sockaddr*)&my_addr, sizeof(my_addr)) == -1){
        perror("[client]: ERROR BIND> ");
        exit(-1);
    }
    */

    if(connect(srv_socket[udp], (struct sockaddr*)&srv_addr, sizeof(srv_addr))){
        perror("[client]: ERROR CONNECT> ");
        exit(-1);
    }
}


//////////////////////////////////////////////////////////////////////////
///                             FUNCTION                              ///
//////////////////////////////////////////////////////////////////////////

//prompt a boot message on stdout
void boot_message(){
    printf("**********************PEER %d**********************\n", my_port);
    printf( "Create an account or login to continue:\n\n"
                "1) signup <username> <password>        --> create account\n"
                "2) in <srv_port> <username> <password> --> connect to server\n"
    );
}

//prompt an help list message on stdout
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

//get a command from stdin
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

    // my_port = atoi(argv[1]);        //???

	// fdt_init();
    // create_listening_socket(true, my_port);

    //boot message
	boot_message();
    prompt();

    //create a socket with server
    create_srv_socket(false, "4242");

        ret = recv(sd, (void*)buffer, MSG_LEN, 0);
        if(ret < 0){
            perror("[client]: Errore nella recv");
            exit(-1);
        }

        buffer[MSG_LEN] = '\0';
        printf("%s\n", buffer);

        close(sd);
    while(1){


        /*
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
                //  all cases for a ready socket:
                //     - server contact me
                //     - other device contact me
                //     - else?
                
                else if(i == srv_socket){
                    //the server conctacted me
                }
                
                // else if(){
                //     //another device contacted me
                // }

        
            }
        }
        */
    }
}