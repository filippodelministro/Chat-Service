#include "all.h"

//////////////////////////////////////////////////////////////////////////
///                             DECLARATION                            ///
//////////////////////////////////////////////////////////////////////////

//-----------     DEVICE    -----------------
int my_port;
bool connected;
struct sockaddr_in my_addr;

// //each device has a port and a socket descriptor
// struct device{
//     int port;           // port number       
//     int sd;             // TCP socket
// }devices[MAX_DEVICES];  //devices array

// //each device socket has an address
// struct sockaddr_in dev_addr[MAX_DEVICES];   //device address
// int n_dev = 0;                              //number of devices

//socket which listen connect request from other devices
int listener;

//each device has a socket to the server
int srv_socket_udp;
int srv_socket_tcp;
struct sockaddr_in srv_addr_udp;
struct sockaddr_in srv_addr_tcp; 

//////////////////////////////////////////////////////////////////////////
///                             FUNCTION                              ///
//////////////////////////////////////////////////////////////////////////

//create a socket a srv_socket for who call the functionon port p; TCP if udp is false, UDP instead
//it call socket and connect
//TCP if u = true, UDP instead
// void create_srv_socket(bool tcp/*, char* p*/){
//     int sd = (tcp) ? srv_socket_tcp : srv_socket_udp;
//     struct sockaddr_in sd_addr = (tcp) ? srv_addr_tcp : srv_addr_udp;

//     //create
//     if((sd = socket(AF_INET, (tcp) ? SOCK_STREAM : SOCK_DGRAM, 0)) == -1){
//         perror("socket() error");
//         exit(-1);
//     }

//     //address
//     memset(&sd_addr, 0, sizeof(sd_addr));
//     my_addr.sin_family = AF_INET;
//     // my_addr.sin_port = htons(atoi(p));
//     my_addr.sin_port = htons(4242);
//     my_addr.sin_addr.s_addr = INADDR_ANY;

//     if(connect(sd, (struct sockaddr*)&sd_addr, sizeof(sd_addr))){
//         perror("[client]: ERROR CONNECT> ");
//         exit(-1);
//     }
// }


void create_srv_socket_tcp(char* p){

    //create
    if((srv_socket_tcp = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        perror("socket() error");
        exit(-1);
    }

    //address
    memset(&srv_addr_tcp, 0, sizeof(srv_addr_tcp));
    srv_addr_tcp.sin_family = AF_INET;
    srv_addr_tcp.sin_port = htons(atoi(p));
    srv_addr_tcp.sin_addr.s_addr = INADDR_ANY;
    // inet_pton(AF_INET, "127.0.0.1", &srv_addr_tcp.sin_addr);

    if(connect(srv_socket_tcp, (struct sockaddr*)&srv_addr_tcp, sizeof(srv_addr_tcp)) == -1){
        perror("[client]: ERROR CONNECT: ");
        exit(-1);
    }
}


int main(int argc, int argv[]){
    
    /*
    */
    //creazione socket
    int ret, sd, len;
    char buffer[1024];

    //da modificare ???
    create_srv_socket_tcp("4242");

    if(recv(srv_socket_tcp, (void*)buffer, MSG_LEN, 0) == -1){
        perror("[client]: Errore nella recv");
        exit(-1);
    }

    buffer[MSG_LEN] = '\0';
    printf("%s\n", buffer);

    close(srv_socket_tcp);


    ///////////////////////////////////////////////////////////////////

    /*
    int ret, sd, len;
    struct sockaddr_in srv_addr;
    char buffer[1024];

    sd = socket(AF_INET, SOCK_STREAM, 0);       //SOCK_DGRAM per socket UDP
    memset(&srv_addr, 0, sizeof(srv_addr));
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_port = htons(4242);
    inet_pton(AF_INET, "127.0.0.1", &srv_addr.sin_addr);

    //collegamento 
    ret = connect(sd, (struct sockaddr*)&srv_addr, sizeof(srv_addr));
    if(ret < 0){
        perror("[client]: Errore nella connect");
        exit(-1);
    }

    ret = recv(sd, (void*)buffer, MSG_LEN, 0);
    if(ret < 0){
        perror("[client]: Errore nella recv");
        exit(-1);
    }

    buffer[MSG_LEN] = '\0';
    printf("%s\n", buffer);

    close(sd);
    */


}