#include "all.h"

//////////////////////////////////////////////////////////////////////////
///                             DECLARATION                            ///
//////////////////////////////////////////////////////////////////////////

//-----------     SERVER    -----------------
//each device has a socket to the server
// int srv_socket_udp;
// struct sockaddr_in srv_addr_udp;
int srv_socket_tcp;
struct sockaddr_in srv_addr_tcp; 


//-----------     DEVICE    -----------------
int my_port;
bool connected;
struct sockaddr_in my_addr;

// //each device has a port and a socket descriptor
struct device{
    int port;           // port number       
    int sd;             // TCP socket
}devices[MAX_DEVICES];  //devices array

// //each device socket has an address
// struct sockaddr_in dev_addr[MAX_DEVICES];   //device address
// int n_dev = 0;                              //number of devices

//-----------     SET    -----------------
//socket which listen connect request from other devices
int listening_socket;    //socket listener (get connect/service request)

fd_set master;          //main set: managed with macro 
fd_set read_fds;        //read set: managed from select() 
int fdmax;


//What a device user can use to interact with device
//////////////////////////////////////////////////////////////////////////
///                              COMMAND                               ///
//////////////////////////////////////////////////////////////////////////

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

//to do         ???
void signup_command(){

}

void in_command(){

}

void hanging_command(){

}

void show_command(){

}

void chat_command(){

}

void share_command(){

}

void out_command(){

}


//maybe in an unic extern file utility.c            ???
//////////////////////////////////////////////////////////////////////////
///                              UTILITY                               ///
//////////////////////////////////////////////////////////////////////////

void prompt()
{
	printf("\n> ");
    fflush(stdout);
}    

//prompt a boot message on stdout
void boot_message(){
    printf("**********************PEER %d**********************\n", my_port);
    printf( "Create an account or login to continue:\n\n"
                "1) signup <username> <password>        --> create account\n"
                "2) in <srv_port> <username> <password> --> connect to server\n"
    );
}

//to do         ???

//legge da tatstiera il comando e lo fa gestire da un figlio
//con uno switch case
//command for routine services
void read_command(){

}

//Function called by the server so manage socket and interaction with devices
//////////////////////////////////////////////////////////////////////////
///                             FUNCTION                               ///
//////////////////////////////////////////////////////////////////////////

void fdt_init(){
    FD_ZERO(&master);
	FD_ZERO(&read_fds);
    // FD_ZERO(&write_fds);
	FD_SET(0, &master);
	
	fdmax = 0;
}

//to do                 ???

/* create_scoket
create a socket a srv_socket for who call the functionon port p; TCP if udp is false, UDP instead
it call socket and connect
TCP if u = true, UDP instead
void create_srv_socket(bool tcp, char* p){
    int sd = (tcp) ? srv_socket_tcp : srv_socket_udp;
    struct sockaddr_in sd_addr = (tcp) ? srv_addr_tcp : srv_addr_udp;

    //create
    if((sd = socket(AF_INET, (tcp) ? SOCK_STREAM : SOCK_DGRAM, 0)) == -1){
        perror("socket() error");
        exit(-1);
    }

    //address
    memset(&sd_addr, 0, sizeof(sd_addr));
    my_addr.sin_family = AF_INET;
    // my_addr.sin_port = htons(atoi(p));
    my_addr.sin_port = htons(4242);
    my_addr.sin_addr.s_addr = INADDR_ANY;

    if(connect(sd, (struct sockaddr*)&sd_addr, sizeof(sd_addr))){
        perror("[client]: ERROR CONNECT> ");
        exit(-1);
    }
}
*/

void create_srv_socket_tcp(char* p){

    //create
    if((srv_socket_tcp = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        perror("socket() error");
        exit(-1);
    }

    //address
    memset(&srv_addr_tcp, 0, sizeof(srv_addr_tcp));
    srv_addr_tcp.sin_family = AF_INET;
    srv_addr_tcp.sin_port = htons(atoi(p));
    srv_addr_tcp.sin_addr.s_addr = INADDR_ANY;
    // inet_pton(AF_INET, "127.0.0.1", &srv_addr_tcp.sin_addr);

    if(connect(srv_socket_tcp, (struct sockaddr*)&srv_addr_tcp, sizeof(srv_addr_tcp)) == -1){
        perror("[client]: ERROR CONNECT: ");
        exit(-1);
    }
}


//////////////////////////////////////////////////////////////////////////
///                             MAIN                                   ///
//////////////////////////////////////////////////////////////////////////

int main(int argc, int argv[]){
    
    int i, newfd, ret;
    socklen_t addrlen;
    char buffer[BUFFER_SIZE];

    /*Stabilire utilizzo porta      ???
    if(argc != 2){
		fprintf(stderr, "Error! Correct syntax: ./server <porta>\n"); 
		exit(-1);
    }

    my_port = atoi(argv[1]);
    */

   //Initialise set structure 
	fdt_init();

	FD_SET(listening_socket, &master);
	fdmax = listening_socket;

    //prompt boot message
    boot_message();
    prompt();


    while(true){

        read_fds = master;

        if(select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
			perror("[device]=> ERROR: SELECT() ");
			exit(-1);
		}

        for(i=0; i<=fdmax; i++){

            if(FD_ISSET(i, &read_fds)){
                
                if(i == 0){
                    //keyboard


                    read_command();
                }

                else if(i == listening_socket){
                    //connection request

                    printf("[server]> Connection request");
                }
                
                else if(i == srv_socket_tcp){
                    //connection request by server
                }


                //clear buffer and prompt
                memset(buffer, 0, BUFFER_SIZE);
                prompt();
            }
        }
    }
}