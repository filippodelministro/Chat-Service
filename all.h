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

#define DELIMITER           "///"
