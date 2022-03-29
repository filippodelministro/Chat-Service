#include "all.h"

//////////////////////////////////////////////////////////////////////////
///                             DECLARATION                            ///
//////////////////////////////////////////////////////////////////////////

int listening_socket;    //socket listener (get connect/service request)
int my_port;
struct sockaddr_in my_addr;

fd_set master;          //main set: managed with macro 
fd_set read_fds;        //read set: managed from select() 
int fdmax;

/*
int i, sd, ret, len, new_sd;
socklen_t addrlen;
struct sockaddr_in cl_addr[MAX_DEVICES];    //array of socket

char buffer[BUFFER_SIZE];
*/


//maybe in an unic extern file utility.c            ???
//////////////////////////////////////////////////////////////////////////
///                              UTILITY                               ///
//////////////////////////////////////////////////////////////////////////

void prompt()
{
	printf("\n> ");
    fflush(stdout);
}    

void boot_message(){
    printf("**********************SERVER STARTED**********************\n");
    help_command();
}

//to do         ???
//legge da tatstiera il comando e lo fa gestire da un figlio
//con uno switch case
void read_command(){

}

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

}

void esc_command(){

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
//prende opcode dal device (recv), poi lo fa gestire da un 
//processo figlio con uno switch case
void handle_request(){

}


//////////////////////////////////////////////////////////////////////////
///                             MAIN                                   ///
//////////////////////////////////////////////////////////////////////////

/*  old main, to check "Hello"
int main(int argc, int argv[]){
    //creazione socket
    int ret, sd, new_sd, len;
    struct sockaddr_in my_addr, cl_addr;
    char buffer[1024];

    sd = socket(AF_INET, SOCK_STREAM, 0);       //SOCK_DGRAM per socket UDP
    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(4242);
    inet_pton(AF_INET, "127.0.0.1", &my_addr.sin_addr);

    //collegamento 
    ret = bind(sd, (struct sockaddr*)&my_addr, sizeof(my_addr));
    ret = listen(sd, 10);
    if(ret < 0){
        perror("[server]: Errore nella listen");
        exit(-1);
    }

    while(1){       //ciclicamente:
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
*/


int main(int argc, char** argv){

    /*Stabilire utilizzo porta      ???
    if(argc != 2){
		fprintf(stderr, "Error! Correct syntax: ./server <porta>\n"); 
		exit(-1);
    }
    */
    
    // Creo socket per ricevere messaggi UDP dai peer       ???
	// create_listening_socket(argv[1]);
	
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
			perror("[server]=> ERROR: SELECT() ");
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