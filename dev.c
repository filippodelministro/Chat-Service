//dev.c
#include "all.h"

//* ///////////////////////////////////////////////////////////////////////
//*                             DECLARATION                             ///
//* ///////////////////////////////////////////////////////////////////////

// -----------     DEVICE    -----------------
struct device{
    int port;                   // port number       
    int sd;                     // TCP socket
    struct sockaddr_in addr;  

    //device info
    int id;
    char* username;
    char* password;
    bool connected;             //true if chat already open

    //chat info
    // int pend_msg;
    char chat_path[15];
    bool hanging_done;
    bool busy;                  //true if already in chat
}devices[MAX_DEVICES];

struct device my_device;
int n_dev;                  //number of devices registred

//-----------     SERVER    -----------------
//server considered as a special device
struct device server;

//-----------     SET    -----------------
//socket which listen connect request from other devices
int listening_socket;    //listener socket (get connect request)

fd_set master;          //main set: managed with macro 
fd_set read_fds;        //read set: managed from select() 
int fdmax;

//-----------    CHAT   -----------------
int n_dev_chat;                     //number of devices in chat

//maybe in an unic extern file utility.c            ???
//* ///////////////////////////////////////////////////////////////////////
//*                                 UTILITY                             ///
//* ///////////////////////////////////////////////////////////////////////

//prompt a boot message on stdout
void boot_message(){
    printf("**********************PEER %d**********************\n", my_device.port);
    printf( "Create an account or login to continue:\n"
                "1) signup  <srv_port> <username> <password>    --> create account\n"
                "2) in      <srv_port> <username> <password>    --> connect to server\n"
    );
}
void help_chat_command(){
    printf( "[%s]\ntype a message + ENTER to send it, or one of the following command:\n"
                "1) \\u         --> show all registered user\n"
                "2) \\a <user>  --> add new user to chat (if online)\n"
                "3) \\s <file>  --> share <file> to all user in current chat\n"
                "4) \\c         --> remove chat history\n"
                "5) \\q         --> quit chat\n",
                my_device.username
    );
    printf("----------------------------------------------------------\n");
}
//Function called by the server so manage socket and interaction with devices
//* ///////////////////////////////////////////////////////////////////////
//*                             FUNCTIONS                               ///
//* ///////////////////////////////////////////////////////////////////////

void fdt_init(){
    FD_ZERO(&master);
	FD_ZERO(&read_fds);
    // FD_ZERO(&write_fds);
	FD_SET(0, &master);
	
	fdmax = 0;

    printf("[fdt_init] set init done...\n");
}
void init_status(){
    int i=0;
    server.connected = true;
    n_dev = n_dev_chat = 0;
    for(i=0; i<MAX_DEVICES; i++)
        devices[i].sd = 0;

   //init set structure 
	fdt_init();
	FD_SET(listening_socket, &master);
	fdmax = listening_socket;
}
void send_opcode(int op){
//send opcode to server
    // printf("[device] send opcode %d to server...\n", op);
    send_int(op, server.sd);
}

//*manage socket
void create_srv_socket_tcp(int p){
//create socket and connect to server
    server.port = p;

    //create
    if((server.sd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        perror("[device] socket() error");
        exit(-1);
    }
    if(setsockopt(server.sd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");

    //address
    memset(&server.addr, 0, sizeof(server.addr));
    server.addr.sin_family = AF_INET;
    server.addr.sin_port = htons(p);
    // srv_addr_tcp.sin_addr.s_addr = INADDR_ANY;
    inet_pton(AF_INET, "127.0.0.1", &server.addr.sin_addr);

    if(connect(server.sd, (struct sockaddr*)&server.addr, sizeof(server.addr)) == -1){
        perror("[device]: error connect(): ");
        printf("<server_port> could be wrong; otherwise server is offline: try later\n");
        // exit(-1);
    }

    // printf("[device] create_srv_tcp_socket: waiting for connection...\n");
}
void create_listening_socket_tcp(){
//create socket to listen and wait for connection from other devices
    if((listening_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        perror("[device] socket() error");
        exit(-1);
    }
    if (setsockopt(listening_socket, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");

    //address
    memset(&my_device.addr, 0, sizeof(my_device.addr));
    my_device.addr.sin_family = AF_INET;
    my_device.addr.sin_port = htons(my_device.port);
    my_device.addr.sin_addr.s_addr = INADDR_ANY;
    // inet_pton(AF_INET, "127.0.0.1", &server.addr.sin_addr);

    if(bind(listening_socket, (struct sockaddr*)&my_device.addr, sizeof(my_device.addr)) == -1){
        perror("[device] Error bind: \n");
        exit(-1);
    }

    listen(listening_socket, MAX_DEVICES);
    
    FD_SET(listening_socket, &master);
    if(listening_socket > fdmax){fdmax = listening_socket;}
}
int create_chat_socket(int id){
//create socket to connect with other devices during chat
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
        exit(-1);
    }

    return devices[id].sd;
}

//*manage devices
void dev_init(int id, const char* usr, const char* pswd){
//initialize my_device structure with usr/pswd get by user and dev_id get server
    struct device* d = &my_device;

    d->id = id;
    d->hanging_done = false;
    d->username = malloc(sizeof(usr));
    d->password = malloc(sizeof(pswd));
    strcpy(d->username, usr);
    strcpy(d->password, pswd);
    printf("[device] dev_init: You are now registered!\n"
                    "\t dev_id: %u \n"
                    "\t username: %s \n"
                    "\t password: %s\n",
                    d->id, d->username, PSWD_STRING
    ); 
    //all pending_msgs
    struct stat st = {0};
    char dir_path[15];
    sprintf(dir_path, "./chat_device_%d", my_device.id);
    strcpy(my_device.chat_path, dir_path);
    if(stat(dir_path, &st) == -1)
        mkdir(dir_path, 0700);
}
void update_devices(){
//update other devices info; ask to server follwing info for each device registered:
//username | port | status
    int i;
    char buffer[BUFFER_SIZE];
    struct device* d;

    create_srv_socket_tcp(server.port);

    //send opcode to server and wait for ack
    send_opcode(UPDATE_OPCODE);
    // sleep(1);

    n_dev = recv_int(server.sd, false);
    for(i=0; i<n_dev; i++){
        d = &devices[i];
        
        //receive other devices info
        recv_msg(server.sd, buffer, false);
        int port = recv_int(server.sd, false);
        bool busy = recv_int(server.sd, false);
        int online = recv_int(server.sd, false);
        bool connected = ((online == OK_CODE) ? true : false);

        d->id = i;
        d->port = port;
        d->busy = busy;
        d->connected = connected;

        d->username = malloc(sizeof(buffer));
        strcpy(d->username, buffer);
    }
    close(server.sd);
}
int find_device(const char* usr){
//find device from username
    int i;
    printf("[find_device] looking for '%s' in %d devices registred...\n", usr, n_dev);
    for(i=0; i<n_dev; i++){
        if(!strcmp(devices[i].username, usr))
            return i;    
    }
    return -1;      //not found
}
int find_device_from_socket(int sock){
//find device from socket descriptor: used in handle_chat to identify sender 
    int j;
    for(j=0; j<n_dev; j++){
        if(devices[j].sd == sock){
            // printf("[find_device_from_socket] found: '%s'\n", devices[i].username);
            return j;    
        }
    }
    printf("[find_device_from_socket] not found!\n");
    return -1;      //not found
}
bool authentication(){    
//send username & password to server to authenticate
    send_msg(my_device.username, server.sd);
    send_msg(my_device.password, server.sd);
    if(recv_int(server.sd, false) == OK_CODE)
        return true;
    else
        return false;
}

//*manage chats
int check_chat_command(char* cmd){
//check if user typed a command while chatting: return an INT with COMMAND_CODE
    char user[WORD_SIZE];

    if(!strncmp(cmd, "\\q", 2)){
        return QUIT_CODE;
    }
    else if(!strncmp(cmd, "\\u", 2)){
        return USER_CODE;
    }
    else if(!strncmp(cmd, "\\a", 2)){
        return ADD_CODE;
    }
    else if(!strncmp(cmd, "\\s", 2)){
        return SHARE_CODE;
    }
    else if(!strncmp(cmd, "\\h", 2)){
        return HELP_CODE;
    }
    else if(!strncmp(cmd, "\\c", 2)){
        return CLEAR_CODE;
    }

    return OK_CODE;     //no command: just a message
}
void append_time(char * buffer, char *msg){
    time_t rawtime; 
    struct tm *msg_time;
    char tv[TIMER_SIZE];  

    time(&rawtime);
    msg_time = localtime(&rawtime);
    strftime(tv, 9, "%X", msg_time);
    sprintf(buffer, "%s [%s]: %s", my_device.username, tv, msg);
}
void read_chat(int id){
    char filename[WORD_SIZE];
    sprintf(filename, "%s/chat_with_%d.txt", my_device.chat_path, id);
    FILE* fp = fopen(filename, "r");
    if(fp){
        char buff[BUFFER_SIZE];
        while(fgets(buff, BUFFER_SIZE, fp) != NULL)
            printf("%s", buff);
        fclose(fp);
    }
    else
        printf("[read_chat] first chat with '%s': opened %s\n", devices[id].username, filename);
}
void set_busy(bool val){
//inform server that device is starting a chat
    create_srv_socket_tcp(server.port);

    send_opcode(BUSY_OPCODE);
    send_int(my_device.id, server.sd);

    if(authentication());
        send_int(val, server.sd);

    close(server.sd);
}

void handle_chat_w_server(){
//handle device comunication with server
//used when a device tries to chat with an offline device
    int code;
    char msg[BUFFER_SIZE];          //message to send
    char buffer[BUFFER_SIZE];       //sending in this format --> <user> [hh:mm:ss]: <msg>
    
    //Handle time value
    time_t rawtime; 
    struct tm *msg_time;
    char tv[8];                 

    printf("[handle_chat_w_server]\n");
    sleep(1);
    system("clear");

    while(true){
        //keyboard: sending message
        fgets(msg, BUFFER_SIZE, stdin);

        //sending while user type "\q"
        code = (check_chat_command(msg));
        if(code == OK_CODE || code == QUIT_CODE)    //sending only valid command
            send_int(code, server.sd);
        
        switch (code){
        case OK_CODE:
            append_time(buffer, msg);
            send_msg(buffer, server.sd);
            break;

        case QUIT_CODE:
            printf("[device] Quit chat!\n");
            return;
        case USER_CODE:
            printf("[device] Error: command is not valid: other device is offline\n");
            break;
        case ADD_CODE:
            printf("[device] Error: command is not valid: other device is offline\n");
            break;
        case SHARE_CODE:
            printf("[device] Error: command is not valid: other device is offline\n"); 
            break;
        case HELP_CODE:
            help_chat_command();
            break;
        default:
            printf("[handle_chat_w_server] error: chat_command is not valid\n");
            return;
        }
    }
}

void add_dev_to_chat(int id, int sd){
//device id is joining the chat: set his socket descriptor and adding it to master_set
    devices[id].sd = sd;
    
    FD_SET(sd, &master);
    if(sd > fdmax){fdmax = sd;}
    n_dev_chat++;
}
void remove_dev_from_chat(int id){
//device id is leaving the chat: clear his socket descriptor from master_set
    FD_CLR(devices[id].sd, &master);
    close(devices[id].sd);
    devices[id].sd = 0;
    n_dev_chat--;    
}
void send_int_broadcast(int num){
//send int to all devices connected in current chat
    int i, n_iter = 0;
    for(i=0; i<MAX_DEVICES && n_iter <= n_dev_chat; i++){
        if(devices[i].sd){
            send_int(num, devices[i].sd);
            n_iter++;
        }
    }
}
int send_msg_broadcast(char buffer[BUFFER_SIZE]){
//send msg to all devices connected in current chat
//if single_chat return ID of device in chat
    int ret = -1;
    int i, n_send = 0;
    for(i=0; i<MAX_DEVICES && n_send < n_dev_chat; i++)     //optimization: max n_dev_chat iteration
        if(devices[i].sd){
            if(n_dev_chat == 1)
                ret = i;                        //when single_chat return ID of device in chat 
            send_msg(buffer, devices[i].sd);
            n_send++;
        }
    return ret;
}

void list_command();
void handle_chat() {
    int code, ret, i, j, id;
    char msg[BUFFER_SIZE];          //message to send
    char buffer[BUFFER_SIZE];       //sending in this format --> <user> [hh:mm:ss]: <msg>
   
    system("clear");
    help_chat_command();

    //when handle_chat starts, chat is single_chat (two devices) 
    //find device to chat with: used to save chat in file (only for single_chat)
    for(i=0; i<MAX_DEVICES; i++){
        if(devices[i].sd){
            id = i;
            break;
        }
    }
    printf("[handle_chat] chatting with %d device: '%s'\n", n_dev_chat, devices[id].username);
    read_chat(id);

    while(true){
        read_fds = master; 
        if(!select(fdmax + 1, &read_fds, NULL, NULL, NULL)){
			perror("[handle_chat] Error: select()\n");
			exit(-1);
        }
        for(i = 0; i <= fdmax; i++) {
			if(FD_ISSET(i, &read_fds)) {
                if (!i) {                        //keyboard: sending message
                    //fix: double user [time] at first send
                    fgets(msg, BUFFER_SIZE, stdin);

                    //check chat_command: send code to other devices to inform what to do                           
                    code = check_chat_command(msg);
                    send_int_broadcast(code);
                    
                    switch(code){
                    case OK_CODE:
                        //message: format message and send it
                        append_time(buffer, msg);
                        send_msg_broadcast(buffer);

                        //if single_chat copying on chat_file for chat history
                        if(n_dev_chat == 1){
                            char filename[WORD_SIZE];
                            sprintf(filename, "%s/chat_with_%d.txt", my_device.chat_path, id);
                            FILE *fp = fopen(filename, "a");
                            if(fp){
                                fprintf(fp, "%s", buffer);
                                fclose(fp);
                            }
                        }

                        break;

                    case QUIT_CODE:
                        printf("[device] Quit chat!\n");
                        for(j=0; j<MAX_DEVICES; j++){
                            if(devices[j].sd)
                                FD_CLR(devices[j].sd, &master);
                            devices[j].sd = 0;
                        }
                        return;

                    case USER_CODE:
                        //show network status
                        if(!server.connected){
                            printf("[device] command is not valid while server is offline!\n");
                            break;
                        }
                        
                        list_command();
                        break;

                    case ADD_CODE:
                        //add new device to chat; inform all other devices in chat
                        if(!server.connected){
                            printf("[device] command is not valid while server is offline!\n");
                            break;
                        }

                        printf("[device] Type <user> to add to this chat: <user> has to be online!\n");
                        update_devices();

                        scanf("%s", msg);
                        int n_id = find_device(msg);
                        send_int_broadcast(n_id);           //send even if dev doesnt exists: receiver handle that case

                        //check if new_device exists and is online
                        if(n_id == -1){
                            printf("[device] user '%s' doesnt exists!\n", msg);
                            break;
                        }
                        if(!devices[n_id].connected){
                            printf("[device] user '%s' is not online!\n", msg);
                            break;
                        }

                        //if here chat with new user can start
                        int n_sd = create_chat_socket(n_id);
                        add_dev_to_chat(n_id, n_sd);
                        send_int(my_device.id, n_sd);           //handshake
                        break;
                    
                    case SHARE_CODE:
                        //get filename and check if file exists, then send it to other devices
                        printf("[device] type <filename> to share\n");
                        system("ls");
                        scanf("%s", msg);

                        FILE *fp = fopen(msg, "r");
                        if(fp == NULL){
                            printf("[device] file '%s' does not exists!\n", msg);
                            send_int_broadcast(ERR_CODE);
                            break;
                        }
                        
                        //file exists: sending it
                        send_int_broadcast(OK_CODE);
                        
                        //get file type from name
                        char* name = strtok(msg, ".");
                        char* type = strtok(NULL, ".");

                        //send type, than file to other device
                        printf("[device] sending %s file...\n", type);
                        for(j=0; j<MAX_DEVICES; j++){
                            if(devices[j].sd){
                                send_msg(type, devices[j].sd);
                                send_file(fp,  devices[j].sd);
                            }
                        }
                        fclose(fp);
                        printf("[device] file shared!\n");
                        break;

                    case HELP_CODE:
                        help_chat_command();
                        break;
                    
                    case CLEAR_CODE:
                        //remove chat_history of current chat (only for single_chats)
                        if(n_dev_chat == 1){
                            char filename[WORD_SIZE];
                            sprintf(filename, "%s/chat_with_%d.txt", my_device.chat_path, id);
                            remove(filename);
                            printf("[device] removed chat_history with %s\n", devices[id].username);
                        }
                        else
                            printf("[device] command '\\c' is not valid during a group_chat\n");

                        break;

                    default:
                        printf("[handle_chat] error: chat_command is not valid\n");
                        return;
                    }

                }
                else if(i == listening_socket){  //received connection request
                    printf("[handle_chat] received connection request!\n");
                    struct sockaddr_in s_addr;
                    socklen_t addrlen = sizeof(s_addr);    
                    int s_sd = accept(listening_socket, (struct sockaddr*)&s_addr, &addrlen);

                    printf("[handle_chat] accepted request\n");
                    int s_id = recv_int(s_sd, false);

                    if(s_id == ERR_CODE){
                        //received request from server
                        printf("[handle_chat] request by server\n");
                        int cmd = recv_int(s_sd, false);

                        switch (cmd){
                            case ESC_OPCODE:
                                //server is logging out while chat is opened
                                server.connected = false;
                                printf("[device] server is now offline!\n");
                                break;

                            case IN_OPCODE:
                                server.connected = true;

                                //inform server on actual device status
                                code = (my_device.busy ? BUSY_CODE : OK_CODE);
                                send_int(code, s_sd);
                                server.sd = s_sd;
                                
                                printf("[device] server is online!\n");
                                break;

                            default:
                                printf("[device] Error in server command!\n");
                                break;
                        }
                    }
                    else
                        add_dev_to_chat(s_id, s_sd);        //received request from other device
                }
                else if(i != listening_socket){  //received message
                    //find device who send it, than receive code and message
                    int s_id = find_device_from_socket(i);
                    int sock = devices[s_id].sd;
                    code = recv_int(sock, false);

                    switch (code){
                    case OK_CODE:
                        //receive message
                        if(!recv_msg(devices[s_id].sd, buffer, false)){
                            printf("[device] other device quit!\n");
                            remove_dev_from_chat(s_id);
                            if(!n_dev_chat){
                                printf("[device] Closing chat\n");
                                return;
                            }
                            break;
                        }

                        //if single_chat copying on chat_file for chat history
                        if(n_dev_chat == 1){    
                            char filename[WORD_SIZE];
                            sprintf(filename, "%s/chat_with_%d.txt", my_device.chat_path, s_id);
                            FILE *fp = fopen(filename, "a");
                            if(fp){
                                fprintf(fp, "%s", buffer);
                                fclose(fp);
                            }
                        }
                        
                        printf("%s", buffer);
                        break;

                    case QUIT_CODE:
                        //other device quit
                        printf("[device] Other device quit...\n");
                        sleep(1);
                        remove_dev_from_chat(s_id);
                        if(!n_dev_chat){
                            printf("[device] Closing chat\n");
                            return;
                        }
                        break;

                    case USER_CODE:
                        //nothing to do here
                        break;

                    case ADD_CODE:
                        int n_id = recv_int(sock, true);
                        printf("[device] received 'add_command' from other device\n");
                        update_devices();
                        
                        //check if new_device exists and is online (double check)
                        if(n_id == ERR_CODE){
                            printf("[device] failed: new device does not exists\n");
                            break;
                        }
                        if(!devices[n_id].connected){
                            printf("[device] failed: new device is not online\n");
                            break;
                        }
                        printf("[device] adding user '%s' to this chat\n", devices[n_id].username);
                        
                        sleep(1);
                        int n_sd = create_chat_socket(n_id);
                        add_dev_to_chat(n_id, n_sd);
                        send_int(my_device.id, n_sd);           //handshake
                        break;
                    
                    case SHARE_CODE:
                        printf("[device] other device is sending you a file: wait...\n");

                        //receive OK_CODE to start file transaction, than receive file
                        if((recv_int(sock, true)) == ERR_CODE){
                            printf("[device] file transfer failed: sender error!\n");
                            break;
                        }

                        //get file type [.txt, .c, .h, ecc.]
                        char type[WORD_SIZE];
                        recv_msg(sock, type, true);

                        //get file and copy in recv.[type]
                        printf("[device] receiving %s file...\n", type);
                        recv_file(sock, type, true);
                        struct stat st;
                        stat("recv.txt", &st);
                        int size = st.st_size;                        
                        printf("[device] received %d byte: check 'recv.%s'\n", size, type);
                        break;
                    
                    case HELP_CODE:
                        //nothing to do here
                        break;
                    
                    case CLEAR_CODE:
                        //nothing to do here
                        break;

                    default:
                        printf("[handle_chat] error: chat_command is not valid\n");
                        return;
                    }
                }
            }
        }
    }
}

void handle_request(){
    int s_sd, s_id, s_port;
    char s_username[BUFFER_SIZE];
    struct sockaddr_in s_addr;
    socklen_t addrlen = sizeof(s_addr);    
    s_sd = accept(listening_socket, (struct sockaddr*)&s_addr, &addrlen);
    
    printf("\n[handle_request] accepted request\n");

    //receive sender info: can be server [ERR_CODE] or a device [ID]
    s_id = recv_int(s_sd, false);

    if(s_id == ERR_CODE){
        //received request from server
        printf("[handle_request] request by server\n");

        int cmd = recv_int(s_sd, false);

        switch (cmd){
        case ESC_OPCODE:
            printf("[device] server is now offline!\n");
            server.connected = false;
            break;

        case IN_OPCODE:
            printf("[device] server is online!\n");
            server.connected = true;
            int code = (my_device.busy ? BUSY_CODE : OK_CODE);
            send_int(code, s_sd);
            server.sd = s_sd;
            break;

        case SHOW_OPCODE:
            int r_id = recv_int(server.sd, false);
            printf("[device] user '%s' has now read your messages!\n", devices[r_id].username);
            break;

        default:
            printf("[device] Error in server command!\n");
            break;
        }
        return;
    }

    //received request from device
    update_devices();
    //todo se ho tempo: add check Y/N to connect (handle d->connected)

    add_dev_to_chat(s_id, s_sd);
    n_dev_chat = 1;
    printf("[device] Received conncection request from '%s'\n", devices[s_id].username);
    printf("[handle_request] %d devices in chat\n", n_dev_chat);
    
    set_busy(true);
    sleep(1);
    handle_chat();

    if(server.connected)
        set_busy(false);

    close(s_sd);
    n_dev_chat = 0;
}

//What a device user can use to interact with device
//* ///////////////////////////////////////////////////////////////////////
//*                             COMMANDS                                ///
//* ///////////////////////////////////////////////////////////////////////

void help_command(){
	printf( "Type a command:\n"
            "1) list         --> show registered users\n"
            "2) hanging      --> receive pending messages\n"
            "3) show <user>  --> show pending messages from <user>\n"
            "4) chat <user>  --> open chat with <user>\n"
            "5) out          --> logout\n"
    );
}

void signup_command(){
    char username[WORD_SIZE];
    char password[WORD_SIZE];

    //get data from stdin
    printf("[device] signup_command:\n[device] insert <srv_port> <username> and <password> to continue\n");

    scanf("%d", &server.port);
    scanf("%s", username);
    scanf("%s", password);

    //create socket to communicate with server
    create_srv_socket_tcp(server.port);

    //send opcode to server and wait for ack
    send_opcode(SIGNUP_OPCODE);
    sleep(1);

    //send username and password to server
    send_msg(username, server.sd);
    send_msg(password, server.sd);    

    //receive dev_id
    int dev_id = recv_int(server.sd, false);
    if(dev_id == ERR_CODE){
        printf("[device] Error in signup: username '%s' not available!\n", username);
        close(server.sd);
        return;
    }

    //update device structure with dev_id get from server
    dev_init(dev_id, username, password);

    close(server.sd);
}

void in_command(){
    char username[WORD_SIZE];
    char password[WORD_SIZE];

    //get data from stdin
    printf("[device] in_command:\n[device] insert <srv_port> <username> and <password> to continue\n");
    scanf("%d", &server.port);
    scanf("%s", username);
    scanf("%s", password);

    //prompt confermation message
    printf("[device] in_command: got your data! \n"
        "\t srv_port: %d \n"
        "\t username: %s \n"
        "\t password: %s\n",
        server.port, username, PSWD_STRING
    );

    // update_devices();
    create_srv_socket_tcp(server.port);

    //send opcode to server and wait for ack
    send_opcode(IN_OPCODE);
    sleep(1);

    //send username and password to server
    printf("[device] sending info to server to login\n");
    send_msg(username, server.sd);
    send_msg(password, server.sd);    
    send_int(my_device.port, server.sd);

    //receiving ID for connect
    int id = recv_int(server.sd, false);
    if(id == ERR_CODE){
        printf("[device] Error in authentication: check usr or pswd and retry\n");
        close(server.sd);
        return;
    }

    //login worked
    my_device.id = id;
    my_device.username = malloc(sizeof(username));
    my_device.password = malloc(sizeof(password));
    strcpy(my_device.username, username);
    strcpy(my_device.password, password);
    
    char dir_path[15];
    sprintf(dir_path, "./chat_device_%d", my_device.id);
    strcpy(my_device.chat_path, dir_path);
    printf("IN_CMD: chat_path = %s\n", my_device.chat_path);

    create_listening_socket_tcp();

    //receive info about pending_messages: OK if dev had pend_msgs before logout
    if(recv_int(server.sd, false) == OK_CODE){
        //device had pending messages before logout
        bool check = true;
        while(recv_int(server.sd, false) == OK_CODE){
            check = false;
            int id = recv_int(server.sd, false);
            printf("[device] user '%s' has not read your messages yet!\n", devices[id].username);
        }
        if(check)
            printf("[device] all devices have read your pending messages!\n");
    }
    
    //complete: device is now online
    my_device.connected = true;
    my_device.hanging_done = false;
    printf("[device] You are now online!\n");

    close(server.sd);
}

void list_command(){
//print all devices info: id | username | port | status
    int i;
    struct device* d;
    update_devices();

    printf("\tid\tusername\tport\tonline\tbusy\n");
    for(i=0; i<n_dev; i++){
        if(i == my_device.id) printf("=>");

        d = &devices[i];
        printf("\t%d\t%s\t\t%d\t",
            d->id, d->username, d->port
        );
        if(d->connected) printf("[x]\t");
        else printf("[ ]\t");

        if(d->busy) printf("[x]\n");
        else printf("[ ]\n");
    }
}

void hanging_command(){
    if(my_device.hanging_done){
        printf("[device] 'hanging' already executed since last logoff\n");
        goto hanging_end;
    }

    //first handshake
    update_devices();
    create_srv_socket_tcp(server.port);
    send_opcode(HANGING_OPCODE);
    send_int(my_device.id, server.sd);
    if(!authentication()){
        printf("[device] hanging_command: authentication failed!\n");
        return;
    }
    sleep(1);

    //get OK_CODE if there are pending_messages
    if((recv_int(server.sd, false)) == OK_CODE){
        //received messages
        int s_id, msg_from_s, n_sender, msg_tot;
        s_id = msg_from_s = n_sender = msg_tot = 0;

        //for each sender receive sender_id and number of messages received
        printf("\tid\tusername\tn_messages\ttimer\t\n");
        while((recv_int(server.sd, false)) == OK_CODE){
            char timer[TIMER_SIZE];
            recv_msg(server.sd, timer, false);  //todo: change in msg_timer
            s_id = recv_int(server.sd, false);               //sender_id
            msg_from_s = recv_int(server.sd, false);         //number of messages from sender   

            msg_tot += msg_from_s;
            n_sender++;
            printf("\t%d\t%s\t\t%d\t\t%s\n", s_id, devices[s_id].username, msg_from_s, timer);

            //receive file with pending_messages
            char type[WORD_SIZE] = {"txt"};
            recv_file(server.sd, type, false);      
        
            //rename file to handle multiple file (need to rename because recv_file() put file in 'recv.txt')
            char new_name[10];
            sprintf(new_name, "%d_from_%d.txt", my_device.id, s_id);
            rename("recv.txt", new_name);               
            printf("[device] saved in %s\n", new_name);
        }

        printf("\n[device] received %d messages from %d different devices\n", msg_tot, n_sender);

        //extra check
        if(n_sender > n_dev){
            printf("[device] Error in pending_messages structure: closing program...\n");
            exit(-1);
        }
    }
    else
        printf("[device] there are no pending messages\n");

    hanging_end:
    my_device.hanging_done = true;
    printf("HANGING TASK COMPLETED\n");
    close(server.sd);
}

void show_command(){
    int s_id;
    char s_username[WORD_SIZE];
    char buff[BUFFER_SIZE];
    scanf("%s", s_username);

    //check to avoid self-show
    if(!strcmp(s_username, my_device.username)){
        printf("[device] Error: showing messages from yourself\n");
	    return;
    } 

    update_devices();

    s_id = find_device(s_username);
    if(s_id == -1){
        printf("[device] user '%s' does not exists: try one of below...\n", s_username);
        list_command();
        return;
    }

    //look for pending_messages file: ID_device_sID.txt
    char filename[13];
    sprintf(filename, "%d_from_%d.txt", my_device.id, s_id);

    FILE *fp = fopen(filename, "r");
    if(fp){
        while(fgets(buff, sizeof(buff), fp))        //read file and print
            printf("%s", buff);
        remove(filename);
        printf("[device] removed file %s\n", filename);
    }   
    else{
        printf("[device] there are no pending_messages: try <hanging> before!\n");
        goto show_end;
    }

    //notifying server that show_command has been executed
    create_srv_socket_tcp(server.port);
    send_opcode(SHOW_OPCODE);
    send_int(my_device.id, server.sd);
    if(!authentication()){
        printf("[device] show_command: authentication failed!\n");
        return;
    }
    sleep(1);

    send_int(my_device.id, server.sd);
    send_int(s_id, server.sd);
    send_int(OK_CODE, server.sd);

    show_end:
    printf("SHOW TASK COMPLETED\n");
    close(server.sd);
}

void chat_command(){
    char r_username[WORD_SIZE];
    int r_id, r_sd;
    scanf("%s", r_username);

    //check to avoid self-chat
    if(!strcmp(r_username, my_device.username)){
        printf("[device] Error: chatting with yourself\n");
	    return;
    } 

    //check if receiver exists
    update_devices();
    r_id = find_device(r_username);
    if(r_id == -1){
        printf("[device] user '%s' does not exists: try one of below...\n", r_username);
        list_command();
        return;
    }

    if(!devices[r_id].connected){
        //receiver is not online: chatting with server
        create_srv_socket_tcp(server.port);
        send_opcode(CHAT_OPCODE);
        sleep(1);

        //sending chat info: my_id & r_username
        send_int(my_device.id, server.sd);
        send_int(r_id, server.sd);

        handle_chat_w_server();
    }
    else{
        //handshake with receiver
        if(devices[r_id].busy){
            printf("[device] user '%s' is chatting already: try later!\n", devices[r_id].username);

            //todo se ho tempo: mando a server che manderà a dev la richiesta di connessione
        }
        else{
            int r_sd = create_chat_socket(r_id);
            send_int(my_device.id, r_sd);

            //telling server that I am busy and enter the chat
            set_busy(true);
            sleep(1);
            add_dev_to_chat(r_id, r_sd);
            handle_chat();
            if(server.connected)            //server could have done ESC during chat
                set_busy(false);

            close(r_sd);
            n_dev_chat = 0;
        }
    }

    chat_end:
    printf("CHAT TASK COMPLETED\n");
    close(server.sd);
}

void out_command(){
    //first handshake
    if(server.connected){
        create_srv_socket_tcp(server.port);
        send_opcode(OUT_OPCODE);
        send_int(my_device.id, server.sd);
    
        if(!authentication()){
            printf("[device] out_command: authentication failed!\n");
            exit(-1);
            return;
        }
        sleep(1);
    }

    my_device.connected = false;
    close(listening_socket);
    FD_CLR(listening_socket, &master);
    close(server.sd);
    printf("[device] You are now offline!\n");
   
    sleep(1);
    exit(0);
}

void read_command(){
    //get command from stdin
    char cmd[COMMAND_LENGHT];
    scanf("%s", cmd);

    if(!strncmp(cmd, "clear", 5) || !strncmp(cmd, "cls", 3)){
        system("clear");
        return;
    }
    if (!strncmp(cmd, "out", 3) && my_device.connected){
        out_command();
        return;
    }

    if(!server.connected){
        printf("[device] server is offline: try later\n");
        return;
    }
    //'signup' and 'in' allowed only if not connected
    //other command allowed only if connected
    //todo: move this block before server.connected check
    if(!strncmp(cmd, "help", 4)){
        if(my_device.connected)
            help_command();
        else
            boot_message();
    }
    //todo ----------

    else if(!strncmp(cmd, "signup", 6)){
        if(!my_device.connected)
            signup_command();
        else{
            printf("Device already connected! Try one of below:\n");
            help_command();
        }
    }
	else if (!strncmp(cmd, "in", 2)){
        if(!my_device.connected)
            in_command();
        else{
            printf("device already connected! Try one of below:\n");
            help_command();
        }
    }
    else if (!strncmp(cmd, "list", 4) && my_device.connected)	
		list_command();
	else if (!strncmp(cmd, "hanging", 7) && my_device.connected)	
		hanging_command();
    else if (!strncmp(cmd, "show", 4) && my_device.connected)	
		show_command();
    else if (!strncmp(cmd, "chat", 4) && my_device.connected)	
		chat_command();

    //command is not valid: show available command
	else{
        printf("[device] command is not valid!\n");
            if(my_device.connected) help_command();
            else boot_message();
    }						
}

//* ///////////////////////////////////////////////////////////////////////
//*                                 MAIN                                ///
//* ///////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]){
    int i;
    if(argc != 2){
		fprintf(stderr, "Error! Correct syntax: ./dev <port>\n"); 
		exit(-1);
    }

    //init device and network status
    init_status();
    my_device.port = atoi(argv[1]);
    
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
                if(!i)                              //keyboard
                    read_command();
                else if(i == listening_socket)      //handle request (server or other device)
                    handle_request();
                
                prompt();
            }
        }
    }
}