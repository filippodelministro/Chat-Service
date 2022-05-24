//server.c
#include "all.h"

//* ///////////////////////////////////////////////////////////////////////
//*                             DECLARATION                             ///
//* ///////////////////////////////////////////////////////////////////////

//-----------     SERVER    -----------------
int my_port;
char my_time[8];
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

int n_dev;                  //number of devices registred
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
    printf("\tdev_id\tusername\ttimestamp\tport\tonline\n");
        for(i=0; i<n_dev; i++){
            
            struct device* d = &devices[i];
                printf("\t%d\t%s\t\t%s\t%d\t",
                    d->id, d->username, 
                    d->time,    
                    d->port
                );
            if(d->connected)printf("[x]\n");
            else printf("[ ]\n");
        }
    }
}

int create_chat_socket(int);
void esc_command(){
    //save network status before switching server off

    int i, sd;
    FILE* fp = fopen("network_status.txt", "w");
    fprintf(fp, "%d\n", n_dev);

    for(i=0; i<n_dev; i++){
        
        struct device* d = &devices[i];
        //copy network status in a file
        fprintf(fp, "%d %s %s %s %d\n",
            d->id, d->username,
            d->password,
            d->time,    
            d->port
        );

        if(d->connected){
            //sending ESC_OPCODE to online devices 
            sd = create_chat_socket(i);
            send_int(ERR_CODE, sd);
            send_int(ESC_OPCODE, sd);
        }
    }

    fclose(fp);
    printf("[server] created 'network_status.txt'\n");
    printf("[server] send ESC_OPCODE to other devices\n[server] closing...\n");
    exit(0);
}

bool check_if_online(int);
void check_command(){
    n_conn = 0;
    for(int i=0; i<n_dev; i++){
        if(check_if_online(i)){
            devices[i].connected = true;
            n_conn++;
        }
        else
            devices[i].connected = false;
    }
}


/*
void print_command(){
    int i;

    FILE* fp_f = fopen("f_network_status.txt", "w+");
    //todo: add server timer
    fprintf(fp_f, "[server] %u devices registered, %d connected at *TIMER*>\n\n", n_dev, n_conn);
    fputs("+---------------------------------------------------+\n", fp_f);
    fputs("|ID\tUSERNAME\t\tPASSWORD\t\tTIMESTAMP\tPORT|\n", fp_f);
    fputs("+---------------------------------------------------+\n", fp_f);

    for(i=0; i<n_dev; i++){
        struct device* d = &devices[i];
        if(d->connected){
            fprintf(fp_f, "|%d\t%s\t\t\t%s\t\t\t%s\t%d\t|\n",
                d->id, d->username,
                d->password,
                d->time,    
                d->port
            );
        }
    }
    fclose(fp_f);
}
*/

//fix: maybe in an unic extern file utility.c            ???
//* ///////////////////////////////////////////////////////////////////////
//*                              UTILITY                                ///
//* ///////////////////////////////////////////////////////////////////////

void boot_message(){
    //add time value
    time_t rawtime;
    struct tm* tv;
    time(&rawtime);
    tv = localtime(&rawtime);
    strftime(my_time, 9, "%X", tv);

    printf("**********************SERVER STARTED**********************\n");
    printf("\tstart_time: %s\n\tport: %d\n", my_time, my_port);
    help_command();
}

//read command from stdin
void read_command(){
    
    char cmd[COMMAND_LENGHT];
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
    else if(!strncmp(cmd, "check", 5))
        check_command();
    else{
        printf("[server] command is not valid!\n");
        help_command();
    }
    prompt();
}

//Function called by the server to manage socket and interaction with devices
//* ///////////////////////////////////////////////////////////////////////
//*                               FUNCTIONS                             ///
//* ///////////////////////////////////////////////////////////////////////

int find_device(const char* usr){
//look for device from username
    int i;

    printf("[server] find_device: looking for '%s' in %d devices registred...\n", usr, n_dev);
    for(i=0; i<n_dev; i++){
        struct device *d = &devices[i];
        
        if(!strcmp(d->username, usr))
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
int add_dev(const char* usr, const char* pswd){
//add deviceto devices list: return dev_id or -1 if not possible to add
    
    if(n_dev >= MAX_DEVICES)
        return ERR_CODE;               

    if(find_device(usr) != -1)
        return ERR_CODE;                //device 'usr' already exists
    // if(usr_exists(usr))
    //     return ERR_CODE;

    struct device* d = &devices[n_dev];

    d->id = n_dev;
    d->username = malloc(sizeof(usr));
    d->password = malloc(sizeof(pswd));
    strcpy(d->username, usr);
    strcpy(d->password, pswd);

    printf("[server] add_dev: added new device!\n"
                    "\t dev_id: %d\n"
                    "\t username: %s\n"
                    "\t password: %s\n",
                    d->id, d->username, d->password
    );
    printf("ADD_DEV: OK\n");
    return n_dev++;
}
int check_and_connect(int id, int po, const char* usr, const char* pswd){
//check if device is registred then connect device to network
    struct device* d = &devices[id];
    printf("check_and_connect: checking for device #%d\n"
        "\tusr: %s\n"
        "\tpswd: %s\n", 
        id, usr, pswd
    );

    if(find_device(usr) == -1){
        printf("[server] check_and_connect: device doesnt exists!\n");
        return ERR_CODE;
    }

    if(d && !strcmp(d->username, usr) && !strcmp(d->password, pswd)){
        //if here device is found
        printf("check_and_connect: authentication success!\n");
        
        d->port = po;
        d->connected = true;
        n_conn++;

        //handle timestamp
        time_t rawtime;
        struct tm* tv;
        time(&rawtime);
        tv = localtime(&rawtime);
        strftime(d->time, 9, "%X", tv);

        //show network info
        list_command();
        return id;
    }

    //if here authentication failed: prompt the reason
    printf("[server] check_and_connect: authentication failed:\n");
    if(strcmp(d->username, usr))printf("\terror on username: %s\n", usr);
    if(strcmp(d->username, usr))printf("\terror on password: %s\n", pswd);
    return ERR_CODE;
}
bool check_if_online(int id){
    int sd = create_chat_socket(id);
    if(sd != -1){
            //if connection doesnt fail, device is online
            send_int(ERR_CODE, sd);
            send_int(IN_OPCODE, sd);
            return true;
        }
        else
            return false;
}
int create_chat_socket(int id){

    //create socket
    if((devices[id].sd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        perror("socket() error");
        printf("closing program...\n"); 
        exit(-1);
    }
    if(setsockopt(devices[id].sd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");

    //create address
    memset((void*)&devices[id].addr, 0, sizeof(devices[id].addr));
    devices[id].addr.sin_family = AF_INET;
    devices[id].addr.sin_port = htons(devices[id].port);
    inet_pton(AF_INET, "127.0.0.1", &devices[id].addr.sin_addr);

    //connection
    if(connect(devices[id].sd, (struct sockaddr*)&devices[id].addr, sizeof(devices[id].addr)) == -1) {
        printf("connect() error");
        return -1;
        // exit(-1);
    }
    return devices[id].sd;
}
void create_listening_socket_tcp(){
    if((listening_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        perror("[device] socket() error");
        exit(-1);
    }
    if (setsockopt(listening_socket, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");

    //address
    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(my_port);
    my_addr.sin_addr.s_addr = INADDR_ANY;
    // inet_pton(AF_INET, "127.0.0.1", &server.addr.sin_addr);

    if(bind(listening_socket, (struct sockaddr*)&my_addr, sizeof(my_addr)) == -1){
        perror("[server] Error bind: \n");
        exit(-1);
    }

    listen(listening_socket, MAX_DEVICES);
    
    FD_SET(listening_socket, &master);
    if(listening_socket > fdmax){ fdmax = listening_socket; }
}

void fdt_init(){
    FD_ZERO(&master);
	FD_ZERO(&read_fds);
    // FD_ZERO(&write_fds);
	FD_SET(0, &master);
	
	fdmax = 0;

    printf("[server] fdt_init: set init done!\n");
}
void restore_network(FILE* fp){
    printf("[restore_network] network is not empty: restore from 'network_status.txt'\n");
    
    //get numer of devices
    fscanf(fp, "%d\n", &n_dev);
    n_conn = 0;

    //get devices info in following format: |#id usr pswd HH:MM:SS #port|
    char buff[4096];
    for(int i=0; i<n_dev; i++){
        struct device* d = &devices[i];
        fgets(buff, sizeof(buff), fp);

        //use strtok() to get buffer values 
        char* b = strtok(buff, " ");
        d->id = atoi(b);
        
        b = strtok(NULL, " ");
        d->username = malloc(sizeof(b));
        strcpy(d->username, b);

        b = strtok(NULL, " ");
        d->password = malloc(sizeof(b));
        strcpy(d->password, b);

        b = strtok(NULL, " ");
        strcpy(d->time, b);

        b = strtok(NULL, " ");
        d->port = atoi(b);

        //inform devices that server is online
        //todo: change with check_if_online(i)
        int sd = create_chat_socket(i);
        if(sd != -1){
            //if connection doesnt fail, device is online
            send_int(ERR_CODE, sd);
            send_int(IN_OPCODE, sd);
            d->connected = true;
            n_conn++;
        }
        else
            d->connected = false;
    }
    
    printf("\n[restore_network] got devices info\n");
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

        memset(&buffer, 0, sizeof(buffer));
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

        memset(&buffer, 0, sizeof(buffer));
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

        
        //directory
        //todo: change [25]
        char dir_path[50];
        char filename[50];
        struct stat st = {0};      //?

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
            printf("[server] Create file to save messages:\n\t%s\n", filename);

            //fix: dont work like this
            // FILE* fp;
            // if((fp = fopen(filename, "w")) == NULL){
            //     perror("[server] Error: fopen()");
            //     exit(-1);
            // }
            // if((fp = fopen("prova.txt", "w")) == NULL){
            //     perror("[server] Error: fopen()");
            //     exit(-1);
            // }

            //device is now running handle_chat_w_server() function
            while((recv_int2(new_dev, false)) == OK_CODE){
                //todo: convert in send_msg (remove BUFFER_SIZE)
                ret = recv(new_dev, (void*)buffer, BUFFER_SIZE, 0);
                //todo: make it invisible for server
                //todo: save messages in a file for receiver
                printf("%s", buffer);
                
                //copy messages in a file
                // fprintf(fp, buffer);     //fix
            }
            // fclose(fp);                  //fix
        }

    chat_end:
        memset(buffer, 0, sizeof(buffer));
        close(new_dev);
        prompt();
        break;

    case SHARE_OPCODE:
        printf("SHARE BRANCH!\n");

        break;

    case OUT_OPCODE:
        //get id from device
        id = recv_int(new_dev);
        printf("id ricevuto: %d\n", id);
        d = &devices[id];

        printf("[server] authentication for user %d\n", id);

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
		printf("[server] using default port [4242]\n");
        p = 4242;
    }
    else p = atoi(argv[1]);
    
    my_port = p;
    create_listening_socket_tcp();

    FILE *file;
    if((file = fopen("network_status.txt", "r"))){
        //if file exists restore old status
        restore_network(file);
        fclose(file);
    }
    else{
        //first boot from server
        printf("[server] first boot: network is empty\n");
        n_conn = n_dev = 0;
    }

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
        //todo: set check_command() as a deamon

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