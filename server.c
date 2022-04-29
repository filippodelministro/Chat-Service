#include "all.h"

//* ///////////////////////////////////////////////////////////////////////
//*                             DECLARATION                             ///
//* ///////////////////////////////////////////////////////////////////////

//-----------     SERVER    -----------------
int my_port;
struct sockaddr_in my_addr;

//-----------     DEVICES    -----------------
struct device{
    int port;                   // port number       
    int sd;                     // TCP socket
    struct sockaddr_in addr;
    bool connected;

    char time[8];
    int id;
    char* username;
    char* password;
}devices[MAX_DEVICES];          //devices array

int n_dev;                 //number of devices registred
int n_conn;                 //number of devices connected

//-----------     SET    -----------------
int listening_socket;    //socket listener (get connect/service request)

fd_set master;          //main set: managed with macro 
fd_set read_fds;        //read set: managed from select()
int fdmax;

//What a server user can use to interact with server
//* ///////////////////////////////////////////////////////////////////////
//*                             COMMANDS                                ///
//* ///////////////////////////////////////////////////////////////////////

void help_command(){
    printf("Type a command:\n\n"
                "1) help --> show command details\n"
                "2) list --> show connected users list\n"
                "3) esc  --> close the server\n" 
    );
}

void list_command(){
    int i;

    printf("\n[server] list_command: %u devices registered, %d connected>\n", n_dev, n_conn);
    if(!n_conn){
        printf("\tThere are no devices connected!\n");
    }
    else{
    printf("\tdev_id\tusername\ttimestamp\tport\n");
        for(i=0; i<n_dev; i++){
            
            struct device* d = &devices[i];
            if(d->connected){
                printf("\t%d\t%s\t\t%s\t%d\n",
                    d->id, d->username, 
                    d->time,    
                    d->port
                );
            }
        }
    }
}

void esc_command(){
    printf("ESC COMMAND ESEGUITO\n");
}


//maybe in an unic extern file utility.c            ???
//* ///////////////////////////////////////////////////////////////////////
//*                              UTILITY                                ///
//* ///////////////////////////////////////////////////////////////////////

void boot_message(){
    printf("**********************SERVER STARTED**********************\n");
    help_command();
}

//read command from stdin
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
//* ///////////////////////////////////////////////////////////////////////
//*                               FUNCTIONS                             ///
//* ///////////////////////////////////////////////////////////////////////

//check if usr account already exists    
bool usr_exists(const char* usr){
    int i;
    for(i=0; i<n_dev; i++){
        struct device *d = &devices[i];
        
        if(!strcmp(d->username, usr))
            return true;    
    }

    return false;
}

//add deviceto devices list: return dev_id or -1 if not possible to add
int add_dev(const char* usr, const char* pswd){
    
    if(n_dev >= MAX_DEVICES)
        return ERR_CODE;

    if(usr_exists(usr))
        return ERR_CODE;

    struct device* d = &devices[n_dev];

    d->id = n_dev;
    d->username = malloc(sizeof(usr));
    d->password = malloc(sizeof(pswd));
    strcpy(d->username, usr);
    strcpy(d->password, pswd);

    printf("[server] add_dev: added new device! \n"
                    "\t dev_id: %d\n"
                    "\t username: %s\n"
                    "\t password: %s\n",
                    d->id, d->username, d->password
    );
    return n_dev++;
}

//look for device from username
int find_device(const char* usr){
    int i;

    printf("[server] find_device: looking for '%s' in %d devices registred...\n", usr, n_dev);
    for(i=0; i<n_dev; i++){
        struct device *d = &devices[i];
        
        if(!strcmp(d->username, usr))
            return i;    
    }

    return -1;      //not found
}

int find_device_from_socket(int sd){
    int i;

    // printf("[server] find_device_from_skt: looking for %d in %d devices connected...\n", sd, n_conn);
    for(i=0; i<n_dev; i++){
        struct device *d = &devices[i];
        
        if(d->sd == sd && d->connected)
            return i;    
    }

    return -1;      //not found
}

int find_device_from_port(int port){
    int i;

    printf("[server] find_device_from_port: looking for port '%d'...\n", port);
    for(i=0; i<n_dev; i++){
        struct device *d = &devices[i];
        
        if(d->port == port && d->connected)
            return i;    
    }

    return -1;      //not found
}

//check if device is registred then connect device to network
int check_and_connect(int id, int po, const char* usr, const char* pswd){
    struct device* d = &devices[id];
    printf("check_and_connect: checking for device #%d\n"
        "\tusr: %s\n"
        "\tpswd: %s\n", 
        id, usr, pswd
    );

    if(d && !strcmp(d->username, usr) && !strcmp(d->password, pswd)){
   
        printf("check_and_connect: authentication success!\n");
        
        //if here device is found
        d->port = po;

        //handle timestamp
        time_t rawtime;
        struct tm* tv;
        time(&rawtime);
        tv = localtime(&rawtime);
        strftime(d->time, 9, "%X", tv);

        d->connected = true;
        n_conn++;

        //show network info
        list_command();
        return id;
    }

    printf("[server] check_and_connect: authentication failed!\n");
    return ERR_CODE;
}

void fdt_init(){
    FD_ZERO(&master);
	FD_ZERO(&read_fds);
    // FD_ZERO(&write_fds);
	FD_SET(0, &master);
	
	fdmax = 0;

    printf("[server] fdt_init: set init done!\n");
}

void create_tcp_socket(char* port){
    int p = atoi(port);
    my_port = p;

    //crate socket
    if((listening_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        perror("[server] error socket()\n");
        exit(-1);
    }
    if(setsockopt(listening_socket, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");

    //create address
    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(p);
    my_addr.sin_addr.s_addr = INADDR_ANY;

    //linking address
    if(bind(listening_socket, (struct sockaddr*)&my_addr, sizeof(my_addr)) == -1){
        perror("[server] Error bind: \n");
        exit(-1);
    }

    listen(listening_socket, MAX_DEVICES);

    printf("[server] create_tcp_socket: waiting for connection...\n");
}

//* //////////////////////////////////////////////////////////////////////

//prende opcode dal device (recv), poi lo fa gestire da un 
//processo figlio con uno switch case
void handle_request(){

    //connecting device
    int new_dev;
    struct sockaddr_in new_addr;
    socklen_t addrlen = sizeof(new_addr);

    char buffer[BUFFER_SIZE];
    int ret;
    //// uint16_t ret_;
    //// pid_t pid;

    //tell which command to do
    uint16_t opcode;

    //for signup and in command
    int port;
    //// uint16_t id;
    int id;
    char username[1024];
    char password[1024];

    //accept new connection and get opcode
    new_dev = accept(listening_socket, (struct sockaddr*)&new_addr, &addrlen);
    opcode = recv_int(new_dev);
    printf("opcode: %d\n", opcode);

    //fix: semmai fare una fork() qui
    switch (opcode){
    case 0:                                                     
        //SIGNUP BRANCH

        //recevive username and password
        if(!recv(new_dev, buffer, BUFFER_SIZE, 0)){
            perror("[server]: Error recv: \n");
            exit(-1);
        }

        //add device to device list
        strcpy(username, strtok(buffer, DELIMITER));
        strcpy(password, strtok(NULL, DELIMITER));
        ret = add_dev(username, password);

        //send dev_id 
        send_int(ret, new_dev);

        memset(buffer, 0, sizeof(buffer));
        close(new_dev);
        prompt();
        break;
    case 1:                            
        //IN BRANCH              

        //recevive username and password
        if(!recv(new_dev, buffer, BUFFER_SIZE, 0)){
            perror("[server]: Error recv: \n");
            exit(-1);
        }

        //split buffer in two string: username and password
        strcpy(username, strtok(buffer, DELIMITER));
        strcpy(password, strtok(NULL, DELIMITER));

        //receive id & port
        id = recv_int(new_dev);
        port = recv_int(new_dev);

        //add device to list and connect & sending ACK to connection      
        ret = check_and_connect(id, port, username, password);
        send_int(ret, new_dev);

        memset(buffer, 0, sizeof(buffer));
        close(new_dev);
        prompt();
        break;

    case 2:
        //HANGING BRANCH
        printf("HANGING BRANCH!\n");

        

        break;

    case 3:
        //SHOW BRANCH
        printf("SHOW BRANCH!\n");

        break;

    case 4:
        //CHAT BRANCH
        printf("CHAT BRANCH!\n");

        //receive sender & receiver info
        int r_id, s_id;
        //todo: name sender and receiver
        //todo: char* s_username, r_username
        s_id = recv_int(new_dev);
        recv_msg(new_dev, username);

        r_id = find_device(username);

        //check if receiver is registered
        if(r_id == -1){
            printf("[server] receiver does not exist...\nsending ERR_CODE\n");    
            send_int(ERR_CODE, new_dev);
            goto chat_end;
        }
        else{
            printf("[server] chat request:\n"
                "\tsender_id: %d\n"
                "\treceiver_id: %d\n",
                s_id, r_id
            );
            //sending receiver's port
            send_int(OK_CODE, new_dev);
        }

        //manage chat in different situation
        if(devices[r_id].connected){
            //request device is online: sending destination info (port and id)
            send_int(devices[r_id].port, new_dev);
            send_int(r_id, new_dev);

        }
        else{
            //request device is offline: server manage chat with new_dev  
            send_int(my_port, new_dev);

            printf("receiver is offline: getting messages from sender!\n");
            while((recv_int2(new_dev, false)) == OK_CODE){
                //todo: convert in send_msg (remove BUFFER_SIZE)
                ret = recv(new_dev, (void*)buffer, BUFFER_SIZE, 0);
                //todo: make it invisible for server
                //todo: save messages in a file for receiver
                printf("%s", buffer);
            }
        }

    chat_end:
        prompt();
        break;

    case 5:
        printf("SHARE BRANCH!\n");

        break;

    case 6:
        //get id from device
        id = recv_int(new_dev);
        printf("id ricevuto: %d", id);

        //todo: check for autentichation (recv)

        //send ACK to safe disconnect
        send_int(id, new_dev);            
        
        struct device* d = &devices[id];
        d->connected = false;
        n_conn--;

        list_command();

        memset(buffer, 0, sizeof(buffer));
        close(new_dev);
        prompt();
        break;

    default:
        printf("[server] halde_request: opcode is not valid!\n");
        break;
    }
}


//* ///////////////////////////////////////////////////////////////////////
//*                                 MAIN                                ///
//* ///////////////////////////////////////////////////////////////////////

int main(int argc, char** argv){

    if(argc != 2){
		fprintf(stderr, "Error! Correct syntax: ./server <port>\n"); 
		exit(-1);
    }
    
    n_conn = n_dev = 0;

    //create socket to get request
	create_tcp_socket(argv[1]);
	
    //Init set structure 
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

                //keyboard                    
                if(i == 0)                      
                    read_command();     

                //deveices request             
                if(i == listening_socket)  
                    handle_request();
            }	 
        }
    }
} 