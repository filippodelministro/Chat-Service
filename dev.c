#include "all.h"

//////////////////////////////////////////////////////////////////////////
///                             DECLARATION                            ///
//////////////////////////////////////////////////////////////////////////

//-----------     SERVER    -----------------
//each device has a socket to the server
// int srv_socket_udp;
// struct sockaddr_in srv_addr_udp;
int srv_port;
int srv_socket_tcp;
struct sockaddr_in srv_addr_tcp; 


//-----------     DEVICE    -----------------
int my_port;
bool connected = false;
bool registred = false;
struct sockaddr_in my_addr;

//each device has a port and a socket descriptor
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


//maybe in an unic extern file utility.c            ???
//////////////////////////////////////////////////////////////////////////
///                              UTILITY                               ///
//////////////////////////////////////////////////////////////////////////

void prompt(){
	printf("\n> ");
    fflush(stdout);
}

//prompt a boot message on stdout
void boot_message(){
    printf("**********************PEER %d**********************\n", my_port);
    printf( "Create an account or login to continue:\n"
                "1) signup <username> <password>        --> create account\n"
                "2) in <srv_port> <username> <password> --> connect to server\n"
    );
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

    printf("[device] fdt_init: set init done...\n");
}

void create_srv_socket_tcp(int p){

    printf("[device] create_srv_tcp_socket: trying to connect to server...\n");

    //create
    if((srv_socket_tcp = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        perror("[device] socket() error");
        exit(-1);
    }

    //address
    memset(&srv_addr_tcp, 0, sizeof(srv_addr_tcp));
    srv_addr_tcp.sin_family = AF_INET;
    srv_addr_tcp.sin_port = htons(p);
    // srv_addr_tcp.sin_addr.s_addr = INADDR_ANY;
    inet_pton(AF_INET, "127.0.0.1", &srv_addr_tcp.sin_addr);

    if(connect(srv_socket_tcp, (struct sockaddr*)&srv_addr_tcp, sizeof(srv_addr_tcp)) == -1){
        perror("[device]: error connect(): ");
        exit(-1);
    }

    printf("[device] create_srv_tcp_socket: waiting for connection...\n");
}

void send_opcode_recv_ack(int o){

    //copy opcode to buffer and send to server
    char buffer[BUFFER_SIZE];

    //send opcode to server
    uint16_t opcode = htons(o);
    memset(buffer, opcode, 2);
    send(srv_socket_tcp, buffer, BUFFER_SIZE, 0);

    //receive akc to proceed
    // while(recv(srv_socket_tcp, buffer, BUFFER_SIZE, 0) < 0);
    recv(listening_socket, buffer, BUFFER_SIZE, 0);
}
//What a device user can use to interact with device
//////////////////////////////////////////////////////////////////////////
///                              COMMAND                               ///
//////////////////////////////////////////////////////////////////////////

//prompt an help list message on stdout
void help_command()
{
	printf( "Type a command:\n"
            "1) hanging   --> receive old msg\n"
            "2) show      --> ??\n"
            "3) chat      --> ??\n"
            "4) share     --> ??\n"
            "5) out       --> ??\n"
    );
}

//to do         ???
void signup_command(){

    int lenght;
    char port[1024];
    char username[1024];
    char password[1024];
    char buffer[BUFFER_SIZE];

    printf("[device] send opcode to server...\n");
    send_opcode_recv_ack(IN_OPCODE);
    printf("[device] in_command: Received acknoledge!\n"
            "Insert <srv_port> <username> and <password> to continue\n"
    );


    //get data from stdin
    scanf("%s", username);
    scanf("%s", password);


    //prompt confermation message                   //maybe to remove
    printf("[device] in_command: got your data! \n"
        "\t username: %s \n"
        "\t password: %s\n",
        username, password
    );

    //check if 

    //create socket to communicate with server
    // create_srv_socket_tcp(srv_port);     ???

    //complete: device is now online
    registred = true;
    printf("[device] You are now registred!\n"); 
   
}

void in_command(){

    int lenght;
    char port[1024];
    char username[1024];
    char password[1024];
    char buffer[BUFFER_SIZE];

    printf("[device] send opcode to server...\n");
    send_opcode_recv_ack(IN_OPCODE);
    printf("[device] in_command: Received acknoledge!\n"
            "Insert <srv_port> <username> and <password> to continue\n"
    );


    //get data from stdin
    scanf("%s", port);
    srv_port = atoi(port);
    scanf("%s", username);
    scanf("%s", password);


    //prompt confermation message                   //maybe to remove
    printf("[device] in_command: got your data! \n"
        "\t srv_port: %d \n"
        "\t username: %s \n"
        "\t password: %s\n",
        srv_port, username, password
    );

    //check if 

    //create socket to communicate with server
    create_srv_socket_tcp(srv_port);



    //complete: device is now online
    connected = true;
    printf("[device] You are now online!\n"); 
 
    // to do ??? to speak witch other deiveces
    // create_dev_socket_tcp();

}

void hanging_command(){
    printf("COMANDO HANGING ESEGUITO \n");
}

void show_command(){
    printf("COMANDO SHOW ESEGUITO \n");
}

void chat_command(){
    printf("COMANDO CHAT ESEGUITO \n");
}

void share_command(){
    printf("COMANDO SHARE ESEGUITO \n");
}

void out_command(){
    printf("COMANDO OUT ESEGUITO \n");
}




//command for routine services
void read_command(){

    char cmd[COMMAND_LENGHT];

    //get commando from stdin
    scanf("%s", cmd);

    //signup and in allowed only if not connected
    //other command allowed only if connected
    
    if(!strncmp(cmd, "signup", 6)){
        if(!connected)
            signup_command();
        else{
            printf("Device already connected! Try one of below:\n");
            help_command();
        }
    }
	else if (!strncmp(cmd, "in", 2)){
        if(!connected)
            in_command();
        else{
            printf("device already connected! Try one of below:\n");
            help_command();
        }
    }
	else if (!strncmp(cmd, "hanging", 7) && connected)	
		hanging_command();
    else if (!strncmp(cmd, "show", 4) && connected)	
		show_command();
    else if (!strncmp(cmd, "chat", 4) && connected)	
		chat_command();
	else if (!strncmp(cmd, "share", 5) && connected)	
        share_command();
    else if (!strncmp(cmd, "out", 3) && connected)
        out_command();

    //command is not valid; ask to help_command and show available command
	else{
		/*
        fprintf(stderr, "Not valid command. Want help? Y/N\n");
        int c = scanf("%1s", cmd);
        if(c == 'Y' || 'y'){
        */
        printf("Command is not valid!\n");
            if(connected) help_command();
            else boot_message();
        // }
    }						
}


//////////////////////////////////////////////////////////////////////////
///                             MAIN                                   ///
//////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]){
    
    int i, newfd, ret;
    socklen_t addrlen;
    char buffer[BUFFER_SIZE];

    if(argc != 2){
		fprintf(stderr, "Error! Correct syntax: ./dev <port>\n"); 
		exit(-1);
    }

    my_port = atoi(argv[1]);

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
			perror("[device] error select() ");
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