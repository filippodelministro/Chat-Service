#include "all.h"

//////////////////////////////////////////////////////////////////////////
///                             DECLARATION                            ///
//////////////////////////////////////////////////////////////////////////

//-----------     SERVER    -----------------
int my_port;
struct sockaddr_in my_addr;


//-----------     DEVICES    -----------------
struct device{
    int port;           // port number       
    int sd;             // TCP socket
}devices[MAX_DEVICES];  //devices array


//-----------     SET    -----------------
int listening_socket;    //socket listener (get connect/service request)

fd_set master;          //main set: managed with macro 
fd_set read_fds;        //read set: managed from select()
int fdmax;

/*
int i, sd, ret, len, new_sd;
socklen_t addrlen;
struct sockaddr_in cl_addr[MAX_DEVICES];    //array of socket

char buffer[BUFFER_SIZE];
*/

//What a server user can use to interact with server
//////////////////////////////////////////////////////////////////////////
///                              COMMAND                               ///
//////////////////////////////////////////////////////////////////////////

void help_command(){
    printf("Type a command:\n\n"
                "1) help --> show command details\n"
                "2) list --> show connected users list\n"
                "3) esc  --> close the server\n" 
    );
}

//to do         ???
void list_command(){
    printf("LIST COMMAND ESEGUITO\n");
}

void esc_command(){
    printf("ESC COMMAND ESEGUITO\n");
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

void help_command();

void boot_message(){
    printf("**********************SERVER STARTED**********************\n");
    help_command();
}

//to do         ???
//legge da tatstiera il comando e lo fa gestire da un figlio
//con uno switch case
void read_command(){
    
    char cmd[COMMAND_LENGHT];

    get_cmd:
    scanf("%s", cmd);

    if(!strncmp(cmd, "help", 4))
        help_command();
    else if(!strncmp(cmd, "list", 4))
        list_command();
    else if(!strncmp(cmd, "esc", 3))
        esc_command();
    else{
        printf("[server] Not valid command\n");
        help_command();
        prompt();
        goto get_cmd;
    }
    prompt();
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

void create_tcp_socket(char* port){

    //crate socket
    if((listening_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        perror("[server] error socket()\n");
        exit(-1);
    }

    //create address
    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = INADDR_ANY;
    my_addr.sin_port = htons(atoi(port));

    //linking address
    if(bind(listening_socket, (struct sockaddr*)&my_addr, sizeof(my_addr)) == -1){
        perror("[server] Error bind: \n");
        exit(-1);
    }

}


//to do                 ???
//prende opcode dal device (recv), poi lo fa gestire da un 
//processo figlio con uno switch case
void handle_request(){

}


//////////////////////////////////////////////////////////////////////////
///                             MAIN                                   ///
//////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv){

    if(argc != 2){
		fprintf(stderr, "Error! Correct syntax: ./server <port>\n"); 
		exit(-1);
    }
    
    //create socket to get request
	create_tcp_socket(argv[1]);
	
    //Initialise set structure 
	fdt_init();


	FD_SET(listening_socket, &master);
	fdmax = listening_socket;

    //prompt boot message
    boot_message();	
    prompt();

    //while active the server loops:
        //  1. read command from keyboard
        //  2. handle request from devices
    while(true){
        int i;
        read_fds = master;

        if(select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
			perror("[server] error: select() ");
			exit(-1);
		}

        for(i=0; i<=fdmax; i++){
    
            if(FD_ISSET(i, &read_fds)){

                //keyboard                     (1)
                if(i == 0)                  
                    read_command();

                //deveices request             (2)
                if(i == listening_socket)   
                    handle_request();
            }	
        }
    }
}