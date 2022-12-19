#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_NAME "root"

#define GROUP_NUM 4

#define KB_1_INDEX 1024 * 1
#define KB_1_SIZE 4 * 1024 * 1

#define KB_64_INDEX 1024*64
#define KB_64_SIZE 4*1024*64

#define KB_256_INDEX 1024*256
#define KB_256_SIZE 4*1024*256

void handler() {
    ;
}

typedef struct PACKET {
    int id;
    int dataIndex;
    size_t dataSize;
    int data[KB_256_INDEX];
} packet;


void createDataFile(char * filename, int * data) {
    int out = open(filename, O_CREAT | O_TRUNC | O_RDWR, 0644);
    write(out, data, sizeof(data));
    close(out);
}

void main(int argc, char ** argv) {
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
    packet in, packets[4];

    // for Share data
    int clientId, clen, pid, sid, packetSize  = 0;
    struct sockaddr_un cli;

    printf("======Start Client======\n");
    pid = getpid();
    printf("PID : %d\n", pid);

    printf("Create Socket\n");
    clientId = socket(AF_UNIX, SOCK_STREAM, 0);

    memset((char *)&cli, 0, sizeof(struct sockaddr_un));
    cli.sun_family = AF_UNIX;
    strcpy(cli.sun_path, SOCKET_NAME);
    clen = sizeof(cli.sun_family) + strlen(cli.sun_path);

    printf("Connect Server ...\n");
    connect(clientId, (struct sockaddr *)&cli, clen);

    // PID 보내기
    printf("1. Send PID\n");
    send(clientId, (int *)&pid, sizeof(int), 0);

    // pid 받기
    sigsuspend(&set);
    printf("2. Recieve PID : ");
    int inid;
    recv(clientId, (int *)&inid, sizeof(int), 0);
    printf("%d\n", inid);

    printf("3. Receive data from input\n");

    for (i = 0; i < 4; i++) {

        //PID 받기
        sigsuspend(&set);
        printf("4. Recieve PID : ");
        int nodeid;
        recv(clientId, (int *)&nodeid, sizeof(int), 0);
        printf("%d\n", nodeid);

        // DATA 받기
        sigsuspend(&set);
        packetSize = recv(clientId, (packet *)&in, sizeof(in), 0);

        if (in.id == 0) {
            printf("Receicve Node #0 PACKET\n");
            packets[0] = in;
            printf("ID : %d\n", packets[0].id);
        }

         if (in.id == 1) {
            printf("Receicve Node #1 PACKET\n");
             packets[1] = in;
             printf("ID : %d\n", packets[1].id);
        }

         if (in.id == 2) {
            printf("Receicve Node #2 PACKET\n");
             packets[2] = in;
             printf("ID : %d\n", packets[2].id);
        }

         if (in.id == 3) {
            printf("Receicve Node #3 PACKET\n");
            packets[3] = in;
            printf("ID : %d\n", packets[3].id);
        }


    }

    /*
    int dataindex = packets[0].dataIndex;
    int dataSize = packets[0].dataSize * 2;
    int * output1 = malloc(dataSize);
    int * output2 = malloc(dataSize);

    int my_number = 0;
    pid_t children[2] = { getpid(), 0};

    for (int i = 1; i < 3; i++) {

        children[i] = fork();

        if (!children[i]) { // child process
            my_number = i;
            break;
        }

    }

    if(my_number == 0) {

       for (i = 0; i < dataindex; i++) {
            output1[i] = packets[0].data[i];
       }

       for (i = 0; i < dataindex; i++) {
            output1[i + dataindex] = packets[1].data[i];
       }

       createDataFile("output1", output1);

    }

    if(my_number == 1) {

        for (i = 0; i < dataindex; i++) {
            output2[i] = packets[2].data[i];
        }

        for (i = 0; i < dataindex; i++) {
            output2[i + dataindex] = packets[3].data[i];
        }

        createDataFile("output2", output2);
    }
    */

    close(clientId);
    return;

}