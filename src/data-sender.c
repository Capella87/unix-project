#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_NAME "root"
#define KB_1_INDEX 1024 * 1
#define KB_1_SIZE 4 * 1024 * 1

#define KB_64_INDEX 1024*64 
#define KB_64_SIZE 4*1024*64

#define KB_256_INDEX 1024*256 
#define KB_256_SIZE 4*1024*256

void handler(int signo)
{
}

typedef struct PACKET
{
    int dataIndex;
    size_t dataSize;
    int data[KB_64_INDEX];
} packet;

int main(int argc, char** argv)
{
    
    system("clear");

    // for Manage signal 
    struct sigaction usrctrl;
    usrctrl.sa_flags = 0;
    usrctrl.sa_handler = handler;
    sigemptyset(&usrctrl.sa_mask);
    sigaction(SIGUSR1, &usrctrl, (void*)0);

    sigset_t set;
    sigfillset(&set);
    sigdelset(&set, SIGUSR1);
    
    // for Loop
    int i, j;

    // for Data
    packet in;
    in.dataIndex = KB_64_INDEX; 
    in.dataSize = KB_64_SIZE;
    for (i = 0; i < KB_64_INDEX; i++) {
        in.data[i] = i;
        //printf(" %d ", in.data[i]);
    }
    
    // for Share
    struct sockaddr_un ser, cli;
    int serverId, clientId, len, clen;
    

    printf("======Start input data======\n");
    printf("PID : %d\n", getpid());

    printf("Create Socket\n");
    serverId = socket(AF_UNIX, SOCK_STREAM, 0);

    printf("Bind socket\n");
    memset((char *)&ser, 0, sizeof(struct sockaddr_un));
    ser.sun_family = AF_UNIX;
    strcpy(ser.sun_path, SOCKET_NAME);
    unlink(ser.sun_path);
    len = sizeof(ser.sun_family) + strlen(ser.sun_path);
    
    bind(serverId, (struct sockaddr *)&ser, len);
    
    listen(serverId, 5);

    printf("Waiting for Client Connection ...\n");
    clientId = accept(serverId, (struct sockaddr *)&cli, &clen);
    printf("CONNECTION SUCCESS\n");

    send(clientId, (packet *)&in, sizeof(in), 0);
    printf("Send data to Client\n");   

    close(serverId);
    close(clientId);
    printf("END\n");
   
    return 0;
}