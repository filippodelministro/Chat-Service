#include "all.h"

//////////////////////////////////////////////////////////////////////////
///                             DECLARATION                            ///
//////////////////////////////////////////////////////////////////////////

//-----------     DEVICE    -----------------
struct device{
    int port;                   // port number       
    int sd;                     // TCP socket
    struct sockaddr_in addr;  

    int id;
    bool connected;
    bool registred;
    struct tm* tv;
    char* username;
    char* password;
};

struct device my_device;

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
//////////////////////////////////////////////////////////////////////////
///                              UTILITY                               ///
//////////////////////////////////////////////////////////////////////////

void prompt(){
	printf("\n> ");
    fflush(stdout);
}

//prompt a boot message on stdout
void boot_message(){
    printf("**********************PEER %d**********************\n", my_device.port);
    printf( "Create an account or login to continue:\n"
                "1) signup  <srv_port> <username> <password>    --> create account\n"
                "2) in      <srv_port> <username> <password>    --> connect to server\n"
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

    // printf("[device] create_srv_tcp_socket: trying to connect to server...\n");

    server.port = p;

    //create
    if((server.sd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        perror("[device] socket() error");
        exit(-1);
    }

    //address
    memset(&server.addr, 0, sizeof(server.addr));
    server.addr.sin_family = AF_INET;
    server.addr.sin_port = htons(p);
    // srv_addr_tcp.sin_addr.s_addr = INADDR_ANY;
    inet_pton(AF_INET, "127.0.0.1", &server.addr.sin_addr);

    if(connect(server.sd, (struct sockaddr*)&server.addr, sizeof(server.addr)) == -1){
        perror("[device]: error connect(): ");
        exit(-1);
    }

    // printf("[device] create_srv_tcp_socket: waiting for connection...\n");
}

void send_opcode_recv_ack(int op){
    //send opcode to server
    printf("[device] send opcode to server...\n");
    uint16_t opcode = htons(op);
    send(server.sd, (void*)&opcode, sizeof(uint16_t), 0);

    //receive akc to proceed
    recv(listening_socket, (void*)&opcode, sizeof(uint16_t), 0);
    int t = ntohs(opcode);
    printf("[device] Received acknoledge: %d\n", t);
}

/*
void send_port_to_srv(int port){
    uint16_t p = htons(port);
    send(server.sd, (void*)&p, sizeof(uint16_t), 0);
}
*/

void send_int(int i, struct device d){
    printf("[device] send_int: sending '%d'\n", i);
    uint16_t p = htons(i);
    send(d.sd, (void*)&p, sizeof(uint16_t), 0);
}

int recv_int(struct device d){
    uint16_t num;
    if(!recv(d.sd, (void*)&num, sizeof(uint16_t), 0)){
        perror("[device]: Error recv: \n");
        exit(-1);
    }

    int t = ntohs(num);
    printf("[device] revc_int: received '%d'\n", t);
    return t;
}

int dev_init(const char* usr, const char* pswd){
    
    char buffer[BUFFER_SIZE];
    struct device* d = &my_device;

    d->username = malloc(sizeof(usr));
    d->password = malloc(sizeof(pswd));
    strncpy(d->username, usr, sizeof(usr));
    strncpy(d->password, pswd, sizeof(usr));

    printf("[device] dev_init: You are now registered!\n"
                    "\t dev_id: %u \n"
                    "\t username: %s \n"
                    "\t password: %s\n",
                    d->id, d->username, d->password
    );
}


//to do???
void send_message(struct device* dev, char* string){
    int port = dev->port;
    int sd = dev->sd;
    // struct sockaddr_in sd_addr = dev->sd_addr;
    int sd_addr_l = sizeof(dev->addr);
    char buffer[BUFFER_SIZE];

    //send lenght to device
    // strcpy(buffer, "prova!");
    strcpy(buffer, string);
    // memchr((void*)buffer, string, sizeof(string));
    send(sd, buffer, sizeof(string), 0);
}
//What a device user can use to interact with device
//////////////////////////////////////////////////////////////////////////
///                              COMMAND                               ///
//////////////////////////////////////////////////////////////////////////

//prompt an help list message on stdout
void help_command(){
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

    char port[1024];
    char username[1024];
    char password[1024];
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
    send_opcode_recv_ack(SIGNUP_OPCODE);
    sleep(1);

    //send username and password to server
    strcat(buffer, username);
    strcat(buffer, DELIMITER);
    strcat(buffer, password);
    send(server.sd, buffer, strlen(buffer), 0);

    //receive dev_id
    // recv_int();

    //update device structure with dev_id get from server
    // dev_init(username, password);
 
    memset(buffer, 0, sizeof(buffer));
    close(server.sd);
}

void in_command(){

    char srv_port[1024];
    char username[1024];
    char password[1024];
    char buffer[BUFFER_SIZE];

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
    send_opcode_recv_ack(IN_OPCODE);
    sleep(1);

    //send username and password to server
    strcat(buffer, username);
    strcat(buffer, DELIMITER);
    strcat(buffer, password);
    send(server.sd, buffer, strlen(buffer), 0);

    //send port to server
    send_int(my_device.port, server);

    //complete: device is now online
    close(server.sd);
    my_device.connected = true;
    printf("[device] You are now online!\n");

    memset(buffer, 0, sizeof(buffer));
}

void hanging_command(){
	char buffer[4096];

	printf("Send Hello!\n");

	memset(buffer, 0, sizeof(buffer));
    strcpy(buffer, "Hello");
    send(server.sd, (void*)buffer, sizeof(buffer), 0);
}

void show_command(){
    printf("COMANDO SHOW ESEGUITO \n");
}

void chat_command(){
    char buffer[BUFFER_SIZE];
    create_srv_socket_tcp(server.port);

    send_opcode_recv_ack(CHAT_OPCODE);
    sleep(1);

    memset(buffer, 0, strlen(buffer));
    recv(server.sd, (void*)buffer, 6, 0);
    printf("%s\n", buffer);
    printf("RICEVUTA!\n\n");

    printf("MANDO ROBA!\n");
    memset(buffer, 0, strlen(buffer));
    strcpy(buffer, "Hello");
    send(server.sd, (void*)buffer, 6, 0);

    printf("COMANDO CHAT ESEGUITO \n");
}

void share_command(){
    printf("COMANDO SHARE ESEGUITO \n");
}

void out_command(){
    create_srv_socket_tcp(server.port);

    send_opcode_recv_ack(OUT_OPCODE);
    sleep(1);

    //change it to ID base handsake
    send_int(my_device.port, server);
    my_device.connected = false;    
    printf("[device] You are now offline!\n");

    //wait ACK from server to safe disconnect
    /*
    if(recv_int(server) == my_device.sd){
        my_device.connected = false;    
        printf("[device] You are now offline!\n");
    }
    else printf("[device] out_command: Error! Device not online!\n");
    */

    close(server.sd);
}

//command for routine services
void read_command(){

    char cmd[COMMAND_LENGHT];

    //get commando from stdin
    scanf("%s", cmd);

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

    //command is not valid; ask to help_command and show available command
	else{
        printf("Command is not valid!\n");
            if(my_device.connected) help_command();
            else boot_message();
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

    my_device.port = atoi(argv[1]);

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
                
                else if(i == server.sd){
                    //connection request by server
                    /*
                    //list
                    recv(server.sd, buffer, BUFFER_SIZE, 0);
                    printf("%s\n", buffer);
                    send(server.sd, buffer, sizeof(buffer), 0);
                    */
                }

                //clear buffer and prompt
                memset(buffer, 0, BUFFER_SIZE);
                prompt();
            }
        }
    }
}