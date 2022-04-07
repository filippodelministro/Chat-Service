#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>

#define BUFFER_SIZE         4096
#define COMMAND_LENGHT      1024

#define MAX_DEVICES         100    

#define SIGNUP_OPCODE       0
#define IN_OPCODE           1
#define HANGING_OPCODE      2
#define SHOW_OPCODE         3
#define CHAT_OPCODE         4
#define SHARE_OPCODE        5
#define OUT_OPCODE          6

char* DELIMITER = "-";

void send_int(int i, int sd){
    uint16_t p = htons(i);
    send(sd, (void*)&p, sizeof(uint16_t), 0);
}

int recv_int(int sd){
    int num;
    uint16_t num_;
    if(!recv(sd, (void*)&num_, sizeof(uint16_t), 0)){
        perror("Error recv: \n");
        exit(-1);
    }
    
    num = ntohs(num_);
    printf("revc_int: received num %d\n", num);
    return num;
}

void send_msg(char *str, int sd){
    int len = strlen(str);
    
    char buffer[len];
    strcpy(buffer, str);
    send_int(len, sd);

    if(!send(sd, (void*)buffer, strlen(buffer), 0)){
        perror("Error send!");
        exit(-1);
    }
}

void recv_msg(int sd, char* ret){
    int len = recv_int(sd);

    char buf[len];

    recv(sd, (void*)&buf, len, 0);
    buf[len] = '\0';
    printf("recv_msg: received '%s'\n", buf);

    strcpy(ret, buf);
    ret = buf;
}