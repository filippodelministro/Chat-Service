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
    bool connected;         //true if chat already open
    
    //not needed for device purpouse
    ////bool registred;
    ////char time[8];

    //chat info
    int msg_pend;
}devices[MAX_DEVICES];

struct device my_device;

int n_dev;                  //number of devices registred
int n_dev_chat;             //number of devices in chat

//-----------     SERVER    -----------------
//server considered as a device
struct device server;

//-----------     SET    -----------------
//socket which listen connect request from other devices
int listening_socket;    //socket listener (get connect request)

fd_set master;          //main set: managed with macro 
fd_set read_fds;        //read set: managed from select() 
int fdmax;

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

    printf("[device] fdt_init: set init done...\n");
}
void send_opcode(int op){
    //send opcode to server
    // printf("[device] send opcode %d to server...\n", op);
    send_int(op, server.sd);
}

//*manage socket
void create_srv_socket_tcp(int p){

    // printf("[device] create_srv_tcp_socket: trying to connect to server...\n");
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
        exit(-1);
    }

    // printf("[device] create_srv_tcp_socket: waiting for connection...\n");
}
void create_listening_socket_tcp(){
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
    printf("[device] create_chat_socket: BEGIN\n");

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

    printf("[device] create_chat_socket: END\n");
    return devices[id].sd;
}

//*manage devices
void dev_init(int id, const char* usr, const char* pswd){
    //initialize my_device structure with usr/pswd get by user & dev_id get server
    struct device* d = &my_device;

    d->id = id;
    d->username = malloc(sizeof(usr));
    d->password = malloc(sizeof(pswd));
    strcpy(d->username, usr);
    strcpy(d->password, pswd);

    printf("[device] dev_init: You are now registered!\n"
                    "\t dev_id: %u \n"
                    "\t username: %s \n"
                    "\t password: %s\n",
                    d->id, d->username, d->password
    );
}

void update_devices(){
    /*update other devices info; ask to server follwing info for each device registered:
        username | port | status*/

    printf("[device] update_devices: BEGIN\n");

    char buffer[BUFFER_SIZE];
    struct device* d;

    create_srv_socket_tcp(server.port);

    //send opcode to server and wait for ack
    send_opcode(UPDATE_OPCODE);
    sleep(1);

    n_dev = recv_int2(server.sd, false);
    for(int i=0; i<n_dev; i++){
        d = &devices[i];
        
        //receive other devices info
        recv_msg2(server.sd, buffer, false);
        int port = recv_int2(server.sd, false);
        int online = recv_int2(server.sd, false);
        bool connected = ((online == OK_CODE) ? true : false);

        d->id = i;
        d->port = port;
        d->connected = connected;

        d->username = malloc(sizeof(buffer));
        strcpy(d->username, buffer);
    }
    close(server.sd);
    printf("[device] update_devices: END\n");
}

/*
int find_device_from_socket(int sd){
    int i;

    // printf("[server] find_device_from_skt: looking for %d in %d devices connected...\n", sd, n_conn);
    for(i=0; i<MAX_DEVICES; i++){
        struct device *d = &devices[i];
        
        if(d->sd == sd && d->connected)
            return i;    
    }

    return -1;      //not found
}
*/

int find_device(const char* usr){
    //find device from username
    int i;

    printf("[device] find_device: looking for '%s' in %d devices registred...\n", usr, n_dev);
    for(i=0; i<n_dev; i++){
        struct device *d = &devices[i];
        
        if(!strcmp(d->username, usr))
            return i;    
    }
    return -1;      //not found
}

//*manage chats
void list_command();
int check_chat_command(char* cmd){
    char user[WORD_SIZE];

    if(!strncmp(cmd, "\\q", 2)){
        printf("[read_chat_command] Quit chat!\n");
        return QUIT_CODE;
    }
    else if(!strncmp(cmd, "\\u", 2)){
        //add new device to chat
        return USER_CODE;
    }
    else if(!strncmp(cmd, "\\a", 2)){
        //add new device to chat
        printf("[device] Type <user> to add to this chat.\n[device] <user> has to be online!\n");
        return ADD_CODE;
    }

    return OK_CODE;
}
void append_time(char * buffer, char *msg){
    time_t rawtime; 
    struct tm *msg_time;
    char tv[8];  

    time(&rawtime);
    msg_time = localtime(&rawtime);
    strftime(tv, 9, "%X", msg_time);
    sprintf(buffer, "%s [%s]: %s", my_device.username, tv, msg);
}

void handle_chat_w_server(){
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
        code = ((check_chat_command(msg)) ? ERR_CODE : OK_CODE);
        send_int(code, server.sd);
        if(code == ERR_CODE)
            break;

        append_time(buffer, msg);
        
        //todo: convert in send_msg (remove BUFFER_SIZE)
        send(server.sd, buffer, BUFFER_SIZE, 0);
        
    }
}

void handle_chat(int sock) {
    int code, ret, i;
    char msg[BUFFER_SIZE];          //message to send
    char buffer[BUFFER_SIZE];       //sending in this format --> <user> [hh:mm:ss]: <msg>
    //// bool first_interaction = true;

    FD_SET(sock, &master);
    fdmax = sock;
    system("clear");

    while(true){
        read_fds = master; 
        if(!select(fdmax + 1, &read_fds, NULL, NULL, NULL)){
			perror("[handle_chat] Error: select()\n");
			exit(-1);
        }
        for (i = 0; i <= fdmax; i++) {
			if(FD_ISSET(i, &read_fds)) {
                if (!i) {
                    //keyboard: sending message
                    //fix: double user [time] at first send
                    fgets(msg, BUFFER_SIZE, stdin);

                    //check chat_command and handle different cases
                    code = check_chat_command(msg);
                    send_int(code, sock);
                    
                    switch (code){
                    case OK_CODE:
                        //message: format message and send it
                        append_time(buffer, msg);
                        //send in any case message: if command, inform other device=
                        //todo: convert in send_msg (remove BUFFER_SIZE)
                        for(int i=0; i<n_dev_chat; i++){
                            send(sock, buffer, BUFFER_SIZE, 0);
                        }
                        
                        break;
                    case QUIT_CODE:
                        FD_CLR(sock, &master);
                        return;

                    case USER_CODE:
                        list_command();

                        break;
                    case ADD_CODE:
                        //get new device info and send to all chat_devices
                        update_devices();
                        scanf("%s", msg);
                        int n_id = find_device(msg);
                        send_int(n_id, sock);

                        if(n_id == -1){
                            printf("[device] user '%s' doesnt exists!\n", msg);
                            break;
                        }

                        if(!devices[n_id].connected){
                            printf("[device] user '%s' is not online!\n", msg);
                            break;
                        }

                        //if here chat with new user can start

                        break;
                    
                    default:
                        printf("[handle_chat] error: chat_command is not valid\n");
                        return;
                    }

                }
                else if(i == sock){
                    //received message
                    //todo: convert in recv_msg (remove BUFFER_SIZE)
                    code = recv_int2(sock, false);

                    switch (code){
                    case OK_CODE:
                       if(recv(sock, (void*)buffer, BUFFER_SIZE, 0) == 0){
                            //todo: handle this case
                            printf("Other device quit!\n");
                            FD_CLR(sock, &master);
                            close(sock);
                            return;
                        }
                        printf("%s", buffer);
                        
                        break;
                    case QUIT_CODE:
                        printf("[device] Other device quit...\n");
                        sleep(1);
                        printf("[device] Closing chat\n");
                        FD_CLR(sock, &master);
                        close(sock);
                        return;
                        return;

                    case USER_CODE:
                        // printf("[device] to add new user to chat type '\\a' <user>. User have to be online\n");
                        // list_command();

                        break;
                    case ADD_CODE:
                        int n_id = recv_int2(sock, true);
                        printf("[device] received 'add_command' from other device\n");
                        if(n_id == ERR_CODE){
                            printf("[device] failed: new device doesnt exists\n");
                            break;
                        }

                        if(!devices[n_id].connected){
                            printf("[device] failed: new device is not online\n");
                            // FD_CLR(sock, &master);
                            // close(sock);
                            break;
                        }
                        // update_devices();
                        printf("[device] other device want to add user '%s' to this chat\n", devices[n_id].username);
                        
                        //todo: send_int(altro device)

                        break;
                    
                    default:
                        printf("[handle_chat] error: chat_command is not valid\n");
                        return;
                    }
                }
                //fix: controllare
                else if(i == listening_socket){
                    //?inutile??;
                    printf("BBBBBBBBBBB\n");
                }
                else if(i == server.sd){
                    //todo: gestire caso della ESC del server;
                    printf("AAAAAAAAAA\n");

                    // server.connected = false;
                }
            }
        }
    }
    
}


void handle_request(){
    printf("\n[handle_request]\n");

    int s_sd, s_id, s_port;
    char s_username[BUFFER_SIZE];
    struct sockaddr_in s_addr;
    socklen_t addrlen = sizeof(s_addr);    
    s_sd = accept(listening_socket, (struct sockaddr*)&s_addr, &addrlen);
    
    printf("[handle_request] accepted\n");

    //receive sender info: can be server [ERR_CODE] or a device [ID]
    s_id = recv_int2(s_sd, true);

    if(s_id == ERR_CODE){
        //sender is server
        printf("[handle_request] request by server\n");
        int cmd = recv_int2(s_sd, true);
        switch (cmd){
        case ESC_OPCODE:
            printf("[device] server is now offline!\n");
            server.connected = false;
            break;

        case IN_OPCODE:
            printf("[device] server is online!\n");
            server.connected = true;
            break;
        
        default:
            printf("[device] Error in server command!\n");
            break;
        }
        return;
    }

    update_devices();
    printf("[device] Received conncection request from '%s'\n", devices[s_id].username);
    printf("[handle_request] %d devices in chat\n", ++n_dev_chat);
    //todo: add check Y/N to connect (handle d->connected)
    //todo: manage history of chat

    //fix: waiting
    sleep(1);
    /*
    for(int i=3; i>0; i--){
        printf("[device] Chat starting in %d seconds...\r", i);
        sleep(1);
    }
    */
    
    handle_chat(s_sd);
    close(s_sd);
    n_dev_chat = 0;
    // return;
}

//create a chat with devices passed in chat_devices [n_dev] 
void create_chat(int n_dev, int* chat_devices){
    struct device* d;

    printf("[create_chat] received chat_devices:\n");
    for(int i=0; i<n_dev; i++)
        printf("%d\n", chat_devices[i]);

    printf("[create_chat] chat request for following devices:\n");
    printf("\tid:\tusername:\tport\tonline\n");
    for(int i=0; i<n_dev; i++){
        if(i != my_device.id){
            int id = chat_devices[i];
            d = &devices[id];
            printf("\t%d\t%s\t\t%d\tDAFARE\n",   //todo: online
                d->id, d->username, d->port
            );
        }
    }
}

//What a device user can use to interact with device
//* ///////////////////////////////////////////////////////////////////////
//*                             COMMANDS                                ///
//* ///////////////////////////////////////////////////////////////////////

//prompt an help list message on stdout
void help_command(){
	printf( "Type a command:\n"
            "1) list         --> show registered users\n"
            "2) hanging      --> receive old messages\n"
            "3) show <user>  --> show pending messages from <user>\n"
            "4) chat <user>  --> open chat with <user>\n"
            "5) share <user> --> share file with <user>??\n"
            "6) out          --> logout\n"
    );
}

void signup_command(){

    char port[WORD_SIZE];
    char username[WORD_SIZE];
    char password[WORD_SIZE];
    char buffer[BUFFER_SIZE];

    //get data from stdin
    printf("[device] signup_command:\n[device] insert <srv_port> <username> and <password> to continue\n");

    scanf("%s", port);
    server.port = atoi(port);
    scanf("%s", username);
    scanf("%s", password);

    //prompt confermation message
    printf("[device] signup_command: got your data! \n"
        "\t srv_port: %d \n"
        "\t username: %s \n"
        "\t password: %s\n",
        server.port, username, password
    );

    //create socket to communicate with server
    create_srv_socket_tcp(server.port);

    //send opcode to server and wait for ack
    send_opcode(SIGNUP_OPCODE);
    sleep(1);

    //send username and password to server
    strcat(buffer, username);
    strcat(buffer, DELIMITER);
    strcat(buffer, password);
    send(server.sd, buffer, strlen(buffer), 0);

    //receive dev_id
    int dev_id = recv_int(server.sd);
    if(dev_id == ERR_CODE){
        printf("[device] Error in signup: username '%s' not available!\n", username);
        close(server.sd);
        exit(-1);
        return;
    }

    //update device structure with dev_id get from server
    dev_init(dev_id, username, password);

    memset(buffer, 0, sizeof(buffer));
    close(server.sd);
}

void in_command(){

    char srv_port[WORD_SIZE];
    char username[WORD_SIZE];
    char password[WORD_SIZE];
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, sizeof(buffer));

    //get data from stdin
    printf("[device] in_command:\n[device] insert <srv_port> <username> and <password> to continue\n");
    scanf("%s", srv_port);
    server.port = atoi(srv_port);
    scanf("%s", username);
    scanf("%s", password);

    //prompt confermation message
    printf("[device] in_command: got your data! \n"
        "\t srv_port: %d \n"
        "\t username: %s \n"
        "\t password: %s\n",
        server.port, username, password
    );

    create_srv_socket_tcp(server.port);

    //send opcode to server and wait for ack
    send_opcode(IN_OPCODE);
    sleep(1);

    //send username and password to server
    strcat(buffer, username);
    strcat(buffer, DELIMITER);
    strcat(buffer, password);
    send(server.sd, buffer, strlen(buffer), 0);

    //send dev_id & port to server
    printf("sending dev_id: %d\n", my_device.id);
    send_int(my_device.id, server.sd);
    printf("sending port: %d\n", my_device.port);
    send_int(my_device.port, server.sd);

    //receiving ACK to connection
    if(recv_int(server.sd) == ERR_CODE){
        printf("[device] Error in authentication: check usr or pswd and retry\n");
        close(server.sd);
        return;
    }

    create_listening_socket_tcp();

    //complete: device is now online
    my_device.connected = true;
    printf("[device] You are now online!\n");

    memset(&buffer, 0, sizeof(buffer));
    close(server.sd);
}

void list_command(){
    struct device* d;

    update_devices();

    //// printf("-------------------------------------------------\n");
    printf("\tid\tusername\tport\tonline\t\n");
    //// printf("-------------------------------------------------\n");
    for(int i=0; i<n_dev; i++){
        if(i == my_device.id) printf("=>");

        d = &devices[i];
        printf("\t%d\t%s\t\t%d\t[",
            d->id, d->username, d->port
        );
        if(d->connected) printf("x]\n");
        else printf(" ]\n");
    }
    //// printf("-------------------------------------------------\n");
}

void hanging_command(){

    //first handshake
    create_srv_socket_tcp(server.port);
    send_opcode(HANGING_OPCODE);
    sleep(1);

    printf("COMANDO HANGING ESEGUITO \n");
    prompt();
    close(server.sd);
}

void show_command(){

    //first handshake
    create_srv_socket_tcp(server.port);
    send_opcode(SHOW_OPCODE);
    sleep(1);
    // send_int(my_device.id, server);

    printf("COMANDO SHOW ESEGUITO \n");
    prompt();
    close(server.sd);
}


void chat_command(){
    char r_username[BUFFER_SIZE];
    int r_id, r_sd;
    scanf("%s", r_username);

    //check to avoid self-chat
    if(strcmp(r_username, my_device.username) == 0){
        printf("[device] Error: chatting with yourself");
	    return;
    } 

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
        //receiver is online: chatting with him
        r_sd = create_chat_socket(r_id);
        devices[r_id].sd = r_sd;

        //handshake with receiver
        send_int(my_device.id, r_sd);

        n_dev_chat = 1;         //used in chat and incremented in '\a' case
        handle_chat(r_sd);
        close(r_sd);
        n_dev_chat = 0;
    }

    chat_end:
    printf("COMANDO CHAT ESEGUITO \n");
    close(server.sd);
}

void share_command(){
    //first handshake
    create_srv_socket_tcp(server.port);
    send_opcode(SHARE_OPCODE);
    sleep(1);
    // send_int(my_device.id, server);

    printf("COMANDO SHARE ESEGUITO \n");
}

void out_command(){
    create_srv_socket_tcp(server.port);

    send_opcode(OUT_OPCODE);
    sleep(1);

    close(listening_socket);
    FD_CLR(listening_socket, &master);

    //send dev_id to server
    send_int(my_device.id, server.sd);
    //sending username & password for autentication
    send_msg(my_device.username, server.sd);
    send_msg(my_device.password, server.sd);
    
    //wait ACK from server to safe disconnect
    if(recv_int(server.sd) == OK_CODE){
        my_device.connected = false;    
        printf("[device] You are now offline!\n");
    }
    else printf("[device] out_command: Error! Device not online!\n");

    close(server.sd);
}

void read_command(){
    //get command from stdin
    char cmd[COMMAND_LENGHT];
    scanf("%s", cmd);

    if(!strncmp(cmd, "clear", 5) || !strncmp(cmd, "cls", 3)){
        system("clear");
        return;
    }

    if(!server.connected){
        printf("[device] server is offline: try later\n");
        return;
    }
    //signup and in allowed only if not connected
    //other command allowed only if connected
    if(!strncmp(cmd, "help", 4)){
        if(my_device.connected)
            help_command();
        else
            boot_message();
    }
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
	else if (!strncmp(cmd, "share", 5) && my_device.connected)	
        share_command();
    else if (!strncmp(cmd, "out", 3) && my_device.connected)
        out_command();

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

//* ///////////////////////////////////////////////////////////////////////
//* ///////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]){
    
    int i;
    // int i, newfd, ret;
    // socklen_t addrlen;
    char buffer[BUFFER_SIZE];

    if(argc != 2){
		fprintf(stderr, "Error! Correct syntax: ./dev <port>\n"); 
		exit(-1);
    }

    my_device.port = atoi(argv[1]);
    server.connected = true;
    n_dev = n_dev_chat = 0;

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
                    handle_request();
                }
                
                // else if(i == server.sd){
                //     //connection request by server
                //     // i = recv_int(server.sd);
                //     // printf("[device] TEST: received %d\n", i);

                //     printf("\t\ti == server.sd\n");
                //     int opcode = recv_int2(server.sd, true);
                //     if(opcode == OUT_OPCODE)
                //         printf("[device] SERVER E' OFFLINE\n");
                // }
                

                //clear buffer and prompt
                memset(buffer, 0, BUFFER_SIZE);
                prompt();
            }
        }
    }
}