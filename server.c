#include "all.h"

//////////////////////////////////////////////////////////////////////////
///                             DECLARATION                            ///
//////////////////////////////////////////////////////////////////////////

//-----------     SERVER    -----------------
int my_port;
struct sockaddr_in my_addr;

//-----------     DEVICES    -----------------
struct device{
    int port;                   // port number       
    int sd;                     // TCP socket
    struct sockaddr_in addr;
    bool connected;

    struct tm* tv;          
    char* username;
    char* password;
    // time_t
}devices[MAX_DEVICES];          //devices array

int n_dev;                 //number of devices registred
int n_conn;                 //number of devices connected

//-----------     SET    -----------------
int listening_socket;    //socket listener (get connect/service request)

fd_set master;          //main set: managed with macro 
fd_set read_fds;        //read set: managed from select()
int fdmax;

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
    int i;

    printf("\n[server] list_command: %u devices registred>\n", n_dev);
    if(!n_conn){
        printf("\tThere are no devices connected!\n");
    }
    else{
    printf("\tdev_id\tusername\ttimestamp\tport\n");
        for(i=0; i<n_dev; i++){

            struct device* d = &devices[i];
            if(d->connected){
                printf("\t#%d)\t%s\t\t%d:%d:%d\t\t%d\n",
                    i, d->username, 
                    d->tv->tm_hour, d->tv->tm_min, d->tv->tm_sec,
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

//to do         ???

//Function called by the server so manage socket and interaction with devices
//////////////////////////////////////////////////////////////////////////
///                             FUNCTION                               ///
//////////////////////////////////////////////////////////////////////////

//handshake to get opcode; prompted in stdout
uint16_t recv_opcode_send_ack(int sd, char* buf){
    uint16_t op;

    if(!recv(sd, (void*)&op, sizeof(uint16_t), 0)){
        perror("[server]: Error recv: \n");
        exit(-1);
    }
    op = ntohs(op);

    send(sd, buf, strlen(buf), 0);           //send ACK
    printf("\n[server] revc_opcode_send_ack: received opcode: %d\n", op);
    return op;
}

//add deviceto devices list: return dev_id or -1 if not possible to add
int add_dev(int sd, struct sockaddr_in addr, const char* usr, const char* pswd){
    
    if(n_dev >= MAX_DEVICES)
        return -1;

    struct device* d = &devices[n_dev];

    d->username = malloc(sizeof(usr));
    d->password = malloc(sizeof(pswd));
    strncpy(d->username, usr, sizeof(usr));
    strncpy(d->password, pswd, sizeof(usr));

    printf("[server] add_dev: added new device! \n"
                    "\t dev_id: %d \n"
                    "\t username: %s \n"
                    "\t password: %s\n",
                    n_dev, d->username, d->password
    );
    return n_dev++;
}

//look for device from username and password
int find_device(const char* usr, const char* pswd){
    int i;

    printf("[server] find_device: looking for '%s' in %d devices registred...\n", usr, n_dev);
    for(i=0; i<n_dev; i++){
        struct device *d = &devices[i];
        
        if(!strcmp(d->username, usr) && !strcmp(d->password, pswd))
            return i;    
    }

    return -1;      //not found
}

//check if device is registred then connect device to network
bool check_and_connect(int sd, int po, struct sockaddr_in addr, const char* usr, const char* pswd){
    
    int dev_id = find_device(usr, pswd);
    if(dev_id == -1){
        printf("[server] check_and_connect: device not found!\n");
        return false;
    }

    printf("[server] check_and_connect: found device %d \n", dev_id);
    //if here device is found
    struct device* d = &devices[dev_id];
    d->sd = sd;
    d->addr = addr;
    d->port = po;

    //handle timestamp
    time_t rawtime;
    time(&rawtime);
    d->tv = localtime(&rawtime);
    
    d->connected = true;
    n_conn++;

    //show network info
    list_command();
    return true;
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

    //crate socket
    if((listening_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        perror("[server] error socket()\n");
        exit(-1);
    }

    //create address
    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(atoi(port));
    my_addr.sin_addr.s_addr = INADDR_ANY;

    //linking address
    if(bind(listening_socket, (struct sockaddr*)&my_addr, sizeof(my_addr)) == -1){
        perror("[server] Error bind: \n");
        exit(-1);
    }

    listen(listening_socket, MAX_DEVICES);

    printf("[server] create_tcp_socket: waiting for connection...\n");
}

//to do                 ???
//prende opcode dal device (recv), poi lo fa gestire da un 
//processo figlio con uno switch case
void handle_request(){

    //connecting device
    int new_dev;
    struct sockaddr_in new_addr;
    socklen_t addrlen = sizeof(new_addr);

    char buffer[BUFFER_SIZE];
    int ret;
    pid_t pid;

    //tell which command to do
    uint16_t opcode;

    //for signup and in command
    int port;
    char username[1024];
    char password[1024];

    //accept new connection and get opcode
    new_dev = accept(listening_socket, (struct sockaddr*)&new_addr, &addrlen);
    opcode = recv_opcode_send_ack(new_dev, buffer);

    //semmai fare una fork() qui ???
    switch (opcode){
    case 0:                                                     //signup command
        //recevive username and password
        if(!recv(new_dev, buffer, BUFFER_SIZE, 0)){
            perror("[server]: Error recv: \n");
            exit(-1);
        }
        sleep(1);

        //add device to device list
        strcpy(username, strtok(buffer, DELIMITER));
        strcpy(password, strtok(NULL, DELIMITER));
        ret = add_dev(new_dev, new_addr, username, password);

        prompt();

        close(new_dev);
        break;
    case 1:                                                     //in command

        //recevive username and password
        if(!recv(new_dev, buffer, BUFFER_SIZE, 0)){
            perror("[server]: Error recv: \n");
            exit(-1);
        }

        //split buffer in two string: username and password
        strcpy(username, strtok(buffer, DELIMITER));
        strcpy(password, strtok(NULL, DELIMITER));
        sleep(1);

        //receive port
        uint16_t p;
        if(!recv(new_dev, (void*)&p, sizeof(uint16_t), 0)){
            perror("[server]: Error recv: \n");
            exit(-1);
        }
        port = ntohs(p);

        //receive addr
        struct sockaddr_in a;
        /*
        if(!recv(new_dev, (void*)&a, sizeof(struct sockaddr_in), 0)){
            perror("[server]: Error recv: \n");
            exit(-1);
        }
        printf("addr: %d\n", a);
        */

        //add device to list and connect          
        ret = check_and_connect(new_dev, port, a, username, password);
        
        prompt();
        break;

    case 2:
        printf("HANGING BRANCH!\n");

        recv(new_dev, (void*)buffer, BUFFER_SIZE, 0);
        printf("%s\n", buffer);

        break;

    case 3:
        printf("SHOW BRANCH!\n");
        break;

    case 4:
        printf("CHAT BRANCH!\n");
        break;

    case 5:
        printf("SHARE BRANCH!\n");
        break;

    case 6:
        printf("OUT BRANCH!\n");

        //find device and disconnect from network
        // // int id = find_device();
        // if(id == -1){
        //     printf("[sever] handle_request: device not found!\n");
        //     break;
        // }

        // struct device* d = &devices[id];
        // d->connected = false;

        // send(new_dev, "ACK", BUFFER_SIZE, 0);
        // list_command();

        // close(new_dev);
        // break;

    default:
        printf("[server] halde_request: opcode is not valid!\n");
        break;
    }
}


//////////////////////////////////////////////////////////////////////////
///                             MAIN                                   ///
//////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv){

    if(argc != 2){
		fprintf(stderr, "Error! Correct syntax: ./server <port>\n"); 
		exit(-1);
    }
    
    n_conn = n_dev = 0;

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