#include "all.h"

//////////////////////////////////////////////////////////////////////////
///                             DECLARATION                            ///
//////////////////////////////////////////////////////////////////////////

int listener;               //socket listener (get connect/service request)
int my_port;
struct sockaddr_in my_addr;

int main(int argc, int argv[]){
    //creazione socket
    int ret, sd, new_sd, len;
    struct sockaddr_in my_addr, cl_addr;
    char buffer[1024];

    sd = socket(AF_INET, SOCK_STREAM, 0);       //SOCK_DGRAM per socket UDP
    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(4242);
    inet_pton(AF_INET, "127.0.0.1", &my_addr.sin_addr);

    //collegamento 
    ret = bind(sd, (struct sockaddr*)&my_addr, sizeof(my_addr));
    ret = listen(sd, 10);
    if(ret < 0){
        perror("[server]: Errore nella listen");
        exit(-1);
    }

    while(1){       //ciclicamente:
        len = sizeof(cl_addr);
        new_sd = accept(sd, (struct sockaddr*)&cl_addr, &len);

        strcpy(buffer, "Hello");
        len = strlen(buffer);

        ret = send(new_sd, (void*)buffer, len, 0);
        if(ret < 0)
            perror("[server]: Errore nella send");
        
        close(new_sd);
    }
}