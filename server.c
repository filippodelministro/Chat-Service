//server.c
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
    int i;

    //? formatted file
    // FILE* fp_f = fopen("f_network_status.txt", "w+");
    FILE* fp = fopen("network_status.txt", "w+");
    //todo: add timer
    // fprintf(fp_f, "[server] %u devices registered, %d connected at *TIMER*>\n\n", n_dev, n_conn);
    // fputs("+---------------------------------------------------+\n", fp_f);
    // fputs("|ID\tUSERNAME\t\tPASSWORD\t\tTIMESTAMP\tPORT|\n", fp_f);
    // fputs("+---------------------------------------------------+\n", fp_f);

    for(i=0; i<n_dev; i++){
        
        struct device* d = &devices[i];
        if(d->connected){
            // fprintf(fp_f, "|%d\t%s\t\t\t%s\t\t\t%s\t%d\t|\n",
            //     d->id, d->username,
            //     d->password,
            //     d->time,    
            //     d->port
            // );
            fprintf(fp, "%d %s %s %s %d\n",
                d->id, d->username,
                d->password,
                d->time,    
                d->port
            );
        }
    }

    fclose(fp);

    printf("[server] e\n");
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

    if(!strncmp(cmd, "clear", 5) || !strncmp(cmd, "cls", 3)){
        system("clear");
        prompt();
        return;
    }

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

void create_tcp_socket(int p){

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
    struct device* d;
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
    char username[WORD_SIZE];
    char password[WORD_SIZE];

    //accept new connection and get opcode
    new_dev = accept(listening_socket, (struct sockaddr*)&new_addr, &addrlen);
    opcode = recv_int(new_dev);
    printf("opcode: %d\n", opcode);

    //fix: semmai fare una fork() qui
    switch (opcode){
    case SIGNUP_OPCODE:                                                     

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
    case IN_OPCODE:                            

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

    case HANGING_OPCODE:
        printf("HANGING BRANCH!\n");

        

        break;

    case SHOW_OPCODE:
        printf("SHOW BRANCH!\n");

        break;

    case CHAT_OPCODE:
        int r_id, s_id;
        char r_username[WORD_SIZE];
        char s_username[WORD_SIZE];

        /*
        //directory
        //todo: change [25]
        char dir_path[25];
        char filename[25];
        struct stat st = {0};      //? 
        */

        //get sender & receiver info
        s_id = recv_int(new_dev);
        r_id = recv_int(new_dev);

        //check if receiver is registered
        if(r_id >= n_dev){
            printf("[server] user '%d' does not exist...\nclosing CHAT\n");    
            goto chat_end;
        }
        else{
            strcpy(r_username, devices[r_id].username);
            strcpy(s_username, devices[s_id].username); 
            printf("\n\n[server] chat request:\n"
                "\tsender: %s\t[%d]\n"
                "\treceiver: %s\t[%d]\n",
                s_username, s_id, r_username, r_id
            );
        }

        //manage chat in different situation
        if(devices[r_id].connected){
            //request device is online: chat handled by other device
            printf("[server] Error: device '%s' is online: should not handle this chat!\n", r_username);
            return;
        }
        else{
            //request device is offline: server manage chat with new_dev  
            printf("'%s' is offline: getting messages from '%s'!\n", r_username, s_username);

            /*
            //*check if directory already exists before create;
                //*create a directory for all the pending_messages
                //*create a subdirectory for each receiver [offline]

            //all pending_msgs
            sprintf(dir_path, "./pending_messages");
            if(stat(dir_path, &st) == -1)
                mkdir(dir_path, 0700);

            //subdirectory for receiver
            sprintf(dir_path, "./pending_messages/device_%d", r_id);
            if(stat(dir_path, &st) == -1)
                mkdir(dir_path, 0700);

            sprintf(filename, "%s/from_%d.txt", dir_path, s_id);
            FILE* fp = fopen(filename, "a");
            */

            //device is now running handle_chat_w_server() function
            while((recv_int2(new_dev, false)) == OK_CODE){
                //todo: convert in send_msg (remove BUFFER_SIZE)
                ret = recv(new_dev, (void*)buffer, BUFFER_SIZE, 0);
                //todo: make it invisible for server
                //todo: save messages in a file for receiver
                printf("%s", buffer);
                
                //copy messages in a file
                // fprintf(fp, buffer);

            }
            // fclose(fp);
        }

    chat_end:
        prompt();
        break;
    
    case GROUPCHAT_CODE:
        printf("GROUPCHAT BRANCH!\n");

        break;

    case SHARE_OPCODE:
        printf("SHARE BRANCH!\n");

        break;

    case OUT_OPCODE:
        //get id from device
        id = recv_int(new_dev);
        printf("id ricevuto: %d", id);
        d = &devices[id];

        //get username & password for autentication
        recv_msg(new_dev, username);
        recv_msg(new_dev, password);
        if(strcmp(d->username, username) || strcmp(d->password, password)){
            printf("[server] authentication failed: OUT not safe!\n");
            send_int(ERR_CODE, new_dev);
            return;
        }

        //send ACK to safe disconnect
        send_int(OK_CODE, new_dev);            
        
        //update device info
        d->connected = false;
        n_conn--;

        list_command();

        memset(buffer, 0, sizeof(buffer));
        close(new_dev);
        prompt();
        break;

    case UPDATE_OPCODE:
        
        //sending number of device, then username of each device
        send_int(n_dev, new_dev);
        for(int i=0; i<n_dev; i++){
            d = &devices[i];
            send_msg(d->username, new_dev);
            send_int(d->port, new_dev);

            //sending ONLINE or OFFLINE
            ret = ((d->connected) ? OK_CODE : ERR_CODE);
            send_int(ret, new_dev);
        }

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
    int p;

    if(argc != 2){
		printf("Note: Using default port [4242]\n");
        p = 4242;
    }
    else p = atoi(argv[1]);
    
    //create socket to get request
	create_tcp_socket(p);
    my_port = p;
    
    n_conn = n_dev = 0;

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