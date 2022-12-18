#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/ipc.h>
#include <time.h>
#include <sys/un.h>

#define GROUP_NUM 4

#define SOCKET_NAME "root"
#define KB_1_INDEX 1024 * 1
#define KB_1_SIZE 4 * 1024 * 1

#define KB_64_INDEX 1024*64 
#define KB_64_SIZE 4*1024*64

#define KB_256_INDEX 1024*256 
#define KB_256_SIZE 4*1024*256

void handler()
{
    ;
}

typedef struct PACKET
{
    int dataIndex;
    size_t dataSize;
    int data[KB_1_INDEX];
} packet;

void createDataFile(char *filename, int *data)
{

    int out = open(filename, O_CREAT | O_TRUNC | O_RDWR, 0644);
    write(out, data, sizeof(data));
    close(out);
}

int main(int argc, char **argv)
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
    int packetIndex = KB_1_INDEX, packetSize = KB_1_SIZE;
    int *data_0, *data_1, *data_2, *data_3;
    data_0 = data_1 =data_2 = data_3 = malloc(packetSize / GROUP_NUM);
    
    // for Share data
    int clientId, clen;
    struct sockaddr_un cli;

    printf("======Start input data======\n");
    //printf("N = %d, Amount of Data = %d\n", N, dataSize);
    printf("PID : %d\n", getpid());

    printf("Create Socket\n");
    clientId = socket(AF_UNIX, SOCK_STREAM, 0);

    printf("Bind socket\n");
    memset((char *)&cli, 0, sizeof(struct sockaddr_un));
    cli.sun_family = AF_UNIX;
    strcpy(cli.sun_path, SOCKET_NAME);
    clen = sizeof(cli.sun_family) + strlen(cli.sun_path);

    printf("Connect Server ...\n");
    connect(clientId, (struct sockaddr *)&cli, clen);

    printf("Receive data from input\n");   
    packetSize = recv(clientId, (packet *)&in, sizeof(in), 0);

    for (i = 0; i < packetIndex;i++) {
        
        if (i % 4 == 0) data_0[i] = i;
        if (i % 4 == 1) data_1[i] = i;
        if (i % 4 == 2) data_2[i] = i;
        if (i % 4 == 3) data_3[i] = i;

    } 

// 나중에 지워야 함
    printf("Create data file\n");
    createDataFile("data0", data_0);
    createDataFile("data1", data_1);
    createDataFile("data2", data_2);
    createDataFile("data3", data_3);

    close(clientId);

//

    key_t key;
    int shmid;

    // key = frok("shmfile",1);
    shmid = shmget(0700, 1024, IPC_CREAT | 0666);
    int shmid2 = shmget(0701, 1024, IPC_CREAT | 0666); // for pid

    int *shmaddr2 = (int *)shmat(shmid2, NULL, 0); //
    int parentpid = *(shmaddr2);                   //

    // fork 4 compute node
    for (int i = 0; i < 4; i++)
    {
        pid_t pid = fork();

        if (pid == -1)
        {
            perror("fork");
            exit(-1);
        }
        if (pid == 0)
        { // child process (compute node)

            // each process get sorted, properly distributed data

            // send to single file using shared memory
            packet *shmaddr = shmat(shmid, NULL, 0);
            char num[1024], buf[1024] = "This is compute node #";
            
            /*
            sprintf(num, "%d", i);
            strncat(buf, num, 1);
            strncat(buf, "\n", 2);
            strncat(shmaddr, buf, sizeof(buf));
            shmdt(shmaddr);
            exit(0);
            */
        }
    }

    for (int i = 0; i < 4; i++)
    {
        pid_t pid = wait(NULL);
        printf("Process %d is terminated.\n", pid);
        /* code */
    }
    // kill(atoi(argv[1]), SIGUSR1);
    kill(parentpid, SIGUSR1); //

    return 0;
}