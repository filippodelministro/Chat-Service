//server.c
#include "all.h"

//* ///////////////////////////////////////////////////////////////////////
//*                             DECLARATION                             ///
//* ///////////////////////////////////////////////////////////////////////

//-----------     SERVER    -----------------
int my_port;
char my_time[TIMER_SIZE];
struct sockaddr_in my_addr;

//-----------     DEVICES    -----------------
struct device{
    int port;                   // port number       
    int sd;                     // TCP socket
    struct sockaddr_in addr;
    bool connected;

    char time_login[TIMER_SIZE];
    char time_logout[TIMER_SIZE];
    int id;
    char* username;
    char* password;
    bool busy;

    int pend_dev_before_logout;  
    int pend_dev;               //number of pending device (tot)
}devices[MAX_DEVICES];          //devices array

int n_dev;                  //number of devices registred
int n_conn;                 //number of devices connected

//-----------     SET    -----------------
int listening_socket;    //socket listener (get connect/service request)

fd_set master;          //main set: managed with macro 
fd_set read_fds;        //read set: managed from select()
int fdmax;

// matrix for pending messages:
//     - lines: sender
//     - coloumn: receiver
// example: pending_messages[2][4] = 10 => device #2 sent 10 messages to device #4
// this messages are saved in "./pending_messages/device_4/from_2.txt"
int pending_messages[MAX_DEVICES][MAX_DEVICES];

//todo:
/*
struct pend_msg{
    int num;
    char time[TIMER_SIZE];
 }pending_messages2[MAX_DEVICES][MAX_DEVICES];
*/

//What a server user can server
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
//show all registered devices and their status: username | time | port | status 
    int i;

    printf("\n[server] list_command: %u devices registered, %d connected>\n", n_dev, n_conn);
    if(!n_conn)
        printf("\tThere are no devices connected!\n");

    else{
    printf("\tdev_id\tusername\tport\ttimestamp\tonline\tbusy\n");
        for(i=0; i<n_dev; i++){
            struct device* d = &devices[i];
            printf("\t%d\t%s\t\t%d\t", d->id, d->username, d->port);

            if(d->connected) printf("%s\t[x]\t", d->time_login);
            else printf("%s\t[ ]\t", d->time_logout);
            
            if(d->busy) printf("[x]\n");
            else printf("[ ]\n");
        }
    }
}

int create_chat_socket(int);
void esc_command(){
//save network status before switching server off
    int i, j, sd;
    FILE* fp = fopen("network_status.txt", "w");
    fprintf(fp, "%d\n", n_dev);

    for(i=0; i<n_dev; i++){
        
        struct device* d = &devices[i];
        //copy network status in a file
        fprintf(fp, "%d %s %s %s %d %d %d\n",
            d->id, d->username,
            d->password,
            d->time_login,    
            d->port,
           d->pend_dev_before_logout, d->pend_dev
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
    
    //save pending_messages matrix (files are already saved in ./pending_messages/...)
    fp = fopen("pending_messages.txt", "w");
    for(i=0; i<MAX_DEVICES; i++){
        for(j=0; j<MAX_DEVICES; j++){
            fprintf(fp, "%d", pending_messages[i][j]);
            printf("%d", pending_messages[i][j]);
        }
        fprintf(fp, "\n");
        printf("\n");
    }
    fclose(fp);

    printf("[server] send ESC_OPCODE to other devices\n[server] closing...\n");
    exit(0);
}

int check_if_online(int);
void check_command(){
    int i;
    n_conn = 0;

    for(i=0; i<n_dev; i++){
        if(check_if_online(i)){
            devices[i].connected = true;
            n_conn++;
        }
        else
            devices[i].connected = false;
    }
}

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
void read_command(){
//read command from stdin
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
int add_dev(const char* usr, const char* pswd){
//add device to devices list: return dev_id or -1 if not possible to add
    
    if(n_dev >= MAX_DEVICES)
        return ERR_CODE;               

    if(find_device(usr) != -1)
        return ERR_CODE;                //device 'usr' already exists

    struct device* d = &devices[n_dev];

    d->id = n_dev;
    d->pend_dev = 0;
    d->pend_dev_before_logout = 0;
    d->busy = false;
    d->username = malloc(sizeof(usr)+1);
    d->password = malloc(sizeof(pswd)+1);
    strcpy(d->username, usr);
    strcpy(d->password, pswd);
    strcpy(d->time_login, "00:00:00");            //default value: case of signup and not login
    strcpy(d->time_logout, "00:00:00");

    printf("[server] add_dev: added new device!\n"
                    "\t dev_id: %d\n"
                    "\t username: %s\n"
                    "\t password: %s\n",
                    d->id, d->username, PSWD_STRING
    );
    return n_dev++;
}
int check_and_connect(int po, const char* usr, const char* pswd){
//check if device is registred then connect device to network
    int id = find_device(usr);
    if(id == -1){
        printf("[server] check_and_connect: device doesnt exists!\n");
        return ERR_CODE;
    }

    struct device* d = &devices[id];
    printf("check_and_connect: checking for device #%d\n"
        "\tusr: %s\n"
        "\tpswd: %s\n", 
        id, usr, PSWD_STRING
    );


    if(d && !strcmp(d->username, usr) && !strcmp(d->password, pswd)){
        //if here device is found
        printf("check_and_connect: authentication success!\n");
        
        d->port = po;
        d->connected = true;
        n_conn++;

        //handle time_login
        time_t rawtime;
        struct tm* tv;
        time(&rawtime);
        tv = localtime(&rawtime);
        strftime(d->time_login, 9, "%X", tv);

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
int check_if_online(int id){
    int busy;
    int sd = create_chat_socket(id);
    if(sd != -1){
        //if connection doesnt fail, device is online
        send_int(ERR_CODE, sd);
        send_int(IN_OPCODE, sd);
        busy = recv_int(sd, true);
        close(sd);
        return busy;
    }
    else{
        printf(": %d\n", id);                        //return:   ERR_CODE -> offline
        close(sd);                                   //          OK_CODE -> online
        return ERR_CODE;                             //          BUSY_CODE -> online & busy
    }
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
    int i, j;
    printf("[restore_network] network is not empty: restore from 'network_status.txt'\n");
    
    //get numer of devices
    fscanf(fp, "%d\n", &n_dev);
    n_conn = 0;

    //get devices info in following format: |#id usr pswd HH:MM:SS #port|
    char buff[BUFFER_SIZE];
    for(i=0; i<n_dev; i++){
        struct device* d = &devices[i];
        fgets(buff, sizeof(buff), fp);

        //use strtok() to get buffer values 
        char* b = strtok(buff, " ");
        d->id = atoi(b);                        //id
        b = strtok(NULL, " ");                  
        d->username = malloc(sizeof(b));        //username
        strcpy(d->username, b);
        b = strtok(NULL, " ");
        d->password = malloc(sizeof(b));        //password
        strcpy(d->password, b);
        b = strtok(NULL, " ");
        strcpy(d->time_login, b);               //time_login
        b = strtok(NULL, " ");
        d->port = atoi(b);                      //port
        b = strtok(NULL, " ");
        d->pend_dev_before_logout = atoi(b);    //pend_dev_before_logout
        b = strtok(NULL, " ");
        d->pend_dev = atoi(b);                  //pend_dev

        //inform devices that server is online
        int ret = check_if_online(i);
        switch (ret){
        case OK_CODE:
            d->connected = true;
            d->busy = false;
            n_conn++;
            break;
        
        case BUSY_CODE:
            d->connected = true;
            d->busy = true;
            n_conn++;
            break;
        
        case ERR_CODE:
            d->connected = false;
            d->busy = false;
            break;
        
        default: printf("[restore_network] check_if_online value not correct\n");
            break;
        }
    }    
    printf("\n[restore_network] got devices info\n");
    list_command();

    //fix dont work
    fp = fopen("pending_messages.txt", "r");
    if(!fp){
        printf("[restore_network] error in opening file: pending_messages got lost!\n");
        return;
    }

    for(i=0; i<MAX_DEVICES; i++){
        for(j=0; j<MAX_DEVICES; j++){
            int val;
            fscanf(fp, "%u", &pending_messages[i][j]);
            // fscanf(fp, "%d", &val);
            // printf("%d", val);
            printf("%d", &pending_messages[i][j]);
        }
        // printf("\n");
    }
    fclose(fp);

    remove("network_status.txt");
    remove("pending_messages.txt");
    printf("\n[restore_network] restored pending_messages matrix\n");
}

bool authentication(int id, int sock){
    char usr[WORD_SIZE];
    char pswd[WORD_SIZE];
    
    //receive device info from device
    recv_msg(sock, usr, false);
    recv_msg(sock, pswd, false);

    //check if info are correct
    if(strcmp(usr, devices[id].username)){
        printf("[authentication] Error: username is not correct!\n");
        send_int(ERR_CODE, sock);
        return false;
    }
    if(strcmp(pswd, devices[id].password)){
        printf("[authentication] Error: pswd is not correct!\n");
        send_int(ERR_CODE, sock);
        return false;
    }
    send_int(OK_CODE, sock);
    printf("[authentication] success!\n");
    return true;
}

//* //////////////////////////////////////////////////////////////////////

void handle_request(){
    //connecting device
    int new_dev, i;
    struct device* d;
    struct sockaddr_in new_addr;
    socklen_t addrlen = sizeof(new_addr);

    char buffer[BUFFER_SIZE];
    int ret;

    //tell which command to do
    uint16_t opcode;

    //for signup and in command
    int port;
    int id, sd;
    char username[WORD_SIZE];
    char password[WORD_SIZE];

    //for chat, hanging, show command
    int r_id, s_id;

    //accept new connection and get opcode
    new_dev = accept(listening_socket, (struct sockaddr*)&new_addr, &addrlen);
    opcode = recv_int(new_dev, false);
    printf("opcode: %d\n", opcode);

    switch (opcode){
    case SIGNUP_OPCODE:                                                     

        //recevive username and password
        recv_msg(new_dev, username, false);
        recv_msg(new_dev, password, false);
        ret = add_dev(username, password);

        //send dev_id 
        send_int(ret, new_dev);

        close(new_dev);
        prompt();
        break;

    case IN_OPCODE:                            

        //receive usr, pswd, id, port from device
        recv_msg(new_dev, username, false);
        recv_msg(new_dev, password, false);
        port = recv_int(new_dev, false);

        //add device to list and connect & sending ACK to connection      
        id = check_and_connect(port, username, password);
        send_int(id, new_dev);

        //check if device had pending_device before logout
        if(devices[id].pend_dev_before_logout)
            send_int(OK_CODE, new_dev);
        else{
            send_int(ERR_CODE, new_dev);
            goto in_end;
        }
        
        //if here there are pending_messages
        for(i=0; i<MAX_DEVICES; i++){
            while(pending_messages[id][i]){
                //inform that user 'i' did not read pend_msgs
                send_int(OK_CODE, new_dev);    
                send_int(i, new_dev);   
                break;
            }
        }
        send_int(ERR_CODE, new_dev);

        in_end:
        close(new_dev);
        prompt();
        break;

    case HANGING_OPCODE:
        //sending logged off receiver his pending messages from other devices
        id = recv_int(new_dev, false);
        
        if(!authentication(id, new_dev)){
            printf("[server] hanging branch: authentication failed!\n");
            return;
        }

        //check if there are pending messages and send OK_CODE or ERR_CODE
        ret = ERR_CODE;
        for(i=0; i<MAX_DEVICES; i++){
            if(pending_messages[i][id]){
                ret = OK_CODE;
                break;
            }
        }
        send_int(ret, new_dev);

        //sending for each device: OK_CODE | sender_id | number of message from sender
        for(i=0; i<MAX_DEVICES; i++){
            if(pending_messages[i][id]){
                send_int(OK_CODE, new_dev);

                //todo: change my_time with message_time
                send_msg(my_time, new_dev);                     //timer                
                send_int(i, new_dev);                           //sender_id
                send_int(pending_messages[i][id], new_dev);     //number of messages from sender_id

                char path[WORD_SIZE];
                sprintf(path, "./pending_messages/device_%d/from_%d.txt", id, i);
                FILE* fp = fopen(path, "r");
                if(!fp){
                    printf("[server] hanging_cmd: pend_msgs file not exists!\n");
                    return;
                }

                send_file(fp, new_dev);
                fclose(fp);

                //once send delete the pending_message file
                remove(path);
            }
        }    
        send_int(ERR_CODE, new_dev);    //sending end of pend_msgs

        close(new_dev);
        prompt();
        break;

    case SHOW_OPCODE:
        //first handshake
        if(!authentication(id, new_dev)){
            printf("[server] show branch: authentication failed!\n");
            return;
        }
        
        //receiver just read pending_messages from device sender
        //get sender & receiver info about pending_messages
        r_id = recv_int(new_dev, false);
        s_id = recv_int(new_dev, false);

        //if OK_CODE show_cmd worked for receiver
        ret = recv_int(new_dev, false);
        if(ret == OK_CODE){
            printf("[server] sending %d messages from '%s' to '%s'\n", pending_messages[s_id][r_id], devices[s_id].username, devices[r_id].username);
            pending_messages[s_id][r_id] = 0;
            
            if(devices[s_id].connected){
                //inform sender that receiver has read pending_messages
                sd = create_chat_socket(s_id);
                send_int(ERR_CODE, sd);

                send_int(SHOW_OPCODE, sd);
                send_int(r_id, sd);
                close(sd);
            }
            else{
                //handled in IN_OPCODE branch (at sender's next login)
                devices[s_id].pend_dev--;
            }
        }

        close(new_dev);
        prompt();
        break;

    case CHAT_OPCODE:
        char r_username[WORD_SIZE];
        char s_username[WORD_SIZE];

        //directory
        char dir_path[WORD_SIZE];
        char filename[WORD_SIZE];
        struct stat st = {0};

        //get sender & receiver info
        s_id = recv_int(new_dev, false);
        r_id = recv_int(new_dev, false);

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
            printf("[server] '%s' is offline: getting messages from '%s'!\n", r_username, s_username);
            devices[s_id].pend_dev++;
            
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

            FILE* fp;
            if((fp = fopen(filename, "a")) == NULL){
                perror("[server] Error: fopen()");
                exit(-1);
            }

            //device is now running handle_chat_w_server() function
            while((recv_int(new_dev, false)) == OK_CODE){
                recv_msg(new_dev, buffer, false);
                pending_messages[s_id][r_id]++;
                
                //copy messages in a file
                fprintf(fp, buffer);     
            }
            printf("\ts_id: %d\tr_id: %d\tn_msgs:  %d\n", s_id, r_id, pending_messages[s_id][r_id]);
            fclose(fp);                  
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
        id = recv_int(new_dev, false);

        //fix or remove
        // if(!authentication(id, new_dev)){
        //     printf("[server] out branch: authentication failed!\n");
        //     return;
        // }
        
        //update device info
        d = &devices[id];
        d->pend_dev_before_logout = d->pend_dev;
        d->connected = false;
        n_conn--;

        //handle time_logout
        time_t rawtime;
        struct tm* tv;
        time(&rawtime);
        tv = localtime(&rawtime);
        strftime(d->time_logout, 9, "%X", tv);

        list_command();

        close(new_dev);
        prompt();
        break;

    case UPDATE_OPCODE:
        
        //sending number of device, then username of each device
        send_int(n_dev, new_dev);
        for(i=0; i<n_dev; i++){
            d = &devices[i];
            send_msg(d->username, new_dev);
            send_int(d->port, new_dev);
            send_int(d->busy, new_dev);

            //sending ONLINE or OFFLINE
            ret = ((d->connected) ? OK_CODE : ERR_CODE);
            send_int(ret, new_dev);
        }

        prompt();
        break;
    
        case BUSY_OPCODE:
        //get id from device
        id = recv_int(new_dev, false);

        if(!authentication(id, new_dev)){
            printf("[server] out branch: authentication failed!\n");
            return;
        }
        devices[id].busy = recv_int(new_dev, false);
        
        prompt();
        break;


    default:
        printf("[server] handle_request: opcode is not valid!\n");
        break;
    }
}


//* ///////////////////////////////////////////////////////////////////////
//*                                 MAIN                                ///
//* ///////////////////////////////////////////////////////////////////////

int main(int argc, char** argv){
    int p, i, j;

    if(argc != 2){
		printf("[server] using default port [4242]\n");
        p = 4242;
    }
    else p = atoi(argv[1]);
    
    my_port = p;
    create_listening_socket_tcp();

    FILE *file = fopen("network_status.txt", "r");
    if(file){
        //if file exists restore old status
        restore_network(file);
        fclose(file);
    }
    else{
        //first boot from server
        printf("[server] first boot: network is empty\n");
        n_conn = n_dev = 0;

        //there are no pending messages at firts boot
        for(i=0; i<MAX_DEVICES; i++)
            for(j=0; j<MAX_DEVICES; j++)
                pending_messages[i][j] = 0;
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
        read_fds = master;
        //todo: set check_command() as a deamon
        if(select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
			perror("[server] error: select() ");
			exit(-1);
		}
        for(i=0; i<=fdmax; i++){
            if(FD_ISSET(i, &read_fds)){
                if(i == 0)                      
                    read_command();             //keyboard
                if(i == listening_socket)       //device request  
                    handle_request();
            }	 
        }
    }
} 