#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <sys/un.h>
#include <stdbool.h>

#define GROUP_NUM 4
#define SOCKET_NAME "root"
#define KB_1_INDEX 1024 * 1
#define KB_1_SIZE 4 * 1024 * 1

#define KB_64_INDEX 1024*64
#define KB_64_SIZE 4*1024*64

#define KB_256_INDEX 1024*256 
#define KB_256_SIZE 4*1024*256

typedef struct PACKET
{
    int dataIndex;
    size_t dataSize;
    int data[KB_1_INDEX];
} packet;

void handler(int signo)
{
}

int main(int argc, char** argv)
{
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
    printf("%d %ld\n", in.dataIndex, in.dataSize);

    for (i = 0; i < in.dataIndex;i++) {
        printf(" %d ", i);
        if (i % 4 == 0) data_0[i] = i;
        if (i % 4 == 1) data_1[i] = i;
        if (i % 4 == 2) data_2[i] = i;
        if (i % 4 == 3) data_3[i] = i;
    } putchar('\n');

    printf("Create data file\n");
    createDataFile("data0", data_0);
    createDataFile("data1", data_1);
    createDataFile("data2", data_2);
    createDataFile("data3", data_3);

    close(clientId);

    int my_number = 0;
    sigaction(SIGUSR1, &usrctrl, (void*)0);    
    pid_t children[4] = { getpid(), 0, 0, 0 };


    int server_shmid = shmget(123, sizeof(packet), IPC_PRIVATE);
    packet* pkt = shmat(server_shmid, (void*)0, 0);


    for (int i = 1; i < 4; i++)
    {
        children[i] = fork();
        
        if (!children[i]) // child process
        {
            my_number = i;
            sigsuspend(&set);
            break;
        }
        else
        {
        }
    }

    if (children[0] == getpid())
    {
        // Open a file
        int fd = open("data0", O_RDONLY);
        
        int count = pkt->dataIndex / 4;
        int temp;
        
        // Send to the shared memory
        for (int i = 0; i < count; i++)
        {
            read(fd, &temp, sizeof(int));
            pkt->data[i * 4] = temp;
        }

        close(fd);

        for (int i = 1; i < 4; i++)
        {
            kill(children[i], SIGUSR1);
        }
        while (wait((void*)0) > 0);        // Send to the shared memory of server.

        kill(atoi(argv[2]), SIGUSR1);
    }
    else
    {
        char name[6] = "data";
        char numbername[2] = { (char)i + '0', '\0' };
        strcat(name, numbername);

        int fd = open(name, O_RDONLY);
        
        int count = pkt->dataIndex / 4;
        int temp;
        
        // Send to the shared memory
        for (int i = 0; i < count; i++)
        {
            read(fd, &temp, sizeof(int));
            pkt->data[i * 4 + my_number] = temp;
        }

        close(fd);
        return 0;
    }
    shmdt(pkt);

    return 0;
}
/*
    struct sigaction act;
    sigset_t set;
    sigfillset(&set);
    sigdelset(&set, SIGUSR1);
    act.sa_handler = handler;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);
    int my_number = 0;

    sigaction(SIGUSR1, &act, (void*)0);    
    pid_t children[4] = { getpid(), 0, 0, 0};

    // Open the server

    int server_shmid = shmget(123, sizeof(collection), IPC_PRIVATE);
    collection* serv_collection = shmat(server_shmid, (void*)0, 0);


    for (int i = 1; i < 4; i++)
    {
        children[i] = fork();
        
        if (!children[i]) // child process
        {
            my_number = i;
            sigsuspend(&set);
            break;
        }
        else
        {
        }
    }

    if (children[0] == getpid())
    {
        int fd = socket(AF_UNIX, SOCK_STREAM, 0);
        struct sockaddr_un ar;
        ar.sun_family = AF_UNIX;
        strcpy(ar.sun_path, SOCKET_NAME);
        int len = strlen(ar.sun_path) + sizeof(ar.sun_family);

        if (connect(fd, (const struct sockaddr*)&ar, len) == -1)
        {
            perror("connect");
            exit(1);
        }
        
        packet* p = (packet*)malloc(sizeof(packet));
        if (recv(fd, p, sizeof(packet), 0) == -1)
        {
            perror("recv");
            exit(1);
        }
        close(fd);

        // Send to the server.
        memcpy(serv_collection->data, p->data, p->dataSize);
        serv_collection->check--;

        for (int i = 1; i < 4; i++)
        {
            kill(children[i], SIGUSR1);
        }
        while (wait((void*)0) > 0);        // Send to the shared memory of server.

        kill(atoi(argv[2]), SIGUSR1);
    }
    else
    {   
        int fd = socket(AF_UNIX, SOCK_STREAM, 0);
        struct sockaddr_un ar;
        ar.sun_family = AF_UNIX;
        strcpy(ar.sun_path, SOCKET_NAME);
        int len = strlen(ar.sun_path) + sizeof(ar.sun_family);

        if (connect(fd, (const struct sockaddr*)&ar, len) == -1)
        {
            perror("connect");
            exit(1);
        }
        
        packet* p = (packet*)malloc(sizeof(packet));
        if (recv(fd, p, sizeof(packet), 0) == -1)
        {
            perror("recv");
            exit(1);
        }
        close(fd);

        // Send to the server.
        memcpy(serv_collection->data + my_number * (p->dataSize), p->data, p->dataSize);
        serv_collection->check--;
        printf("Node %d is done\n", my_number);

        return 0;
    }

    shmdt(serv_collection);
    shmctl(server_shmid, IPC_RMID, (void*)0);
    return 0;
}
*/