//all.h

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>

#define BUFFER_SIZE         4096
#define COMMAND_LENGHT      1024
#define WORD_SIZE           1024

#define MAX_DEVICES         100    

#define SIGNUP_OPCODE       0
#define IN_OPCODE           1
#define HANGING_OPCODE      2
#define SHOW_OPCODE         3
#define CHAT_OPCODE         4
#define SHARE_OPCODE        5
#define OUT_OPCODE          6

#define UPDATE_OPCODE       8
#define ESC_OPCODE          9

#define ERR_CODE            65535
#define OK_CODE             65534
#define QUIT_CODE           65533
#define USER_CODE           65532
#define ADD_CODE            65531
#define SHARE_CODE          65530

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
    if(num == ERR_CODE){ printf("revc_int: received ERR_CODE!\n"); }
    else if (num == OK_CODE) { printf("recv_int: received OK_CODE!\n"); }
    else { printf("revc_int: received num %d\n", num); }
    
    return num;
}

int recv_int2(int sd, bool show){
    int num;
    uint16_t num_;
    if(!recv(sd, (void*)&num_, sizeof(uint16_t), 0)){
        perror("Error recv: \n");
        return ERR_CODE;
    }
    
    num = ntohs(num_);
    if(show){
        printf("revc_int: received ");
        switch (num){
        case ERR_CODE:
            printf("ERR_CODE!\n");
            break;
        case OK_CODE:
            printf("OK_CODE!\n");
            break;
        case QUIT_CODE:
            printf("QUIT_CODE!\n");
            break;
        case ADD_CODE:
            printf("ADD_CODE!\n");
            break;

        default:
            printf("%d\n", num);
            break;
        }
    }
    
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

/*
#define recv_msg(...) OVERLOAD(recv_msg, (__VA_ARGS__), \
    (recv_msg1, (int, char*)), \
    (recv_msg2, (int, char*, bool)), \
)
*/

void recv_msg(int sd, char* ret){
    int len = recv_int(sd);

    char buf[len];

    recv(sd, (void*)&buf, len, 0);
    buf[len] = '\0';
    printf("recv_msg: received '%s'\n", buf);

    strcpy(ret, buf);
    ret = buf;
}

//todo: overloading of functions
int recv_msg2(int sd, char* ret, bool show){
    int len = recv_int2(sd, show);
    int ok;

    char buf[len];

    ok = recv(sd, (void*)&buf, len, 0);
    buf[len] = '\0';

    if(show)
        printf("recv_msg: received '%s'\n", buf);

    strcpy(ret, buf);
    ret = buf;
    return ok;
}


void prompt(){
	printf("\n> ");
    fflush(stdout);
}

void recv_file(int sd){
    printf("[recv_file] start\n");
    FILE *fp;
    int n;
    char buffer[BUFFER_SIZE];

    fp = fopen("recv.txt", "w");
    printf("[recv_file] opened 'recv.txt'\n");

    while(true){
        int code = recv_int2(sd, false);
        if(code == OK_CODE){
            n = recv(sd, buffer, BUFFER_SIZE, 0);
            fprintf(fp, "%s", buffer);
            bzero(buffer, BUFFER_SIZE);
        }
        else{
            printf("[recv_file] end\n");
            fclose(fp);
            return;
        }
    }
}

void send_file(FILE *fp, int sd){
    int n;
    char buff[BUFFER_SIZE] = {0};

    while (true){
        if(fgets(buff, BUFFER_SIZE, fp) != NULL){
            send_int(OK_CODE, sd);
            if(send(sd, buff, sizeof(buff), 0) == -1) {
                perror("[send_file] Error!\n");
                exit(1);
            }
            bzero(buff, BUFFER_SIZE);
        }
        else{
            send_int(ERR_CODE, sd);
            return;
        }
    }
}


