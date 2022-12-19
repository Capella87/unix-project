#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
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
#include <sys/time.h>
#include <time.h>

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
    int data[KB_64_INDEX / 4];
} packet;

void handler(int signo)
{
}

struct timespec diff(struct timespec start, struct timespec end)
{
    struct timespec temp;

    if ((end.tv_nsec-start.tv_nsec) < 0)
    {
        temp.tv_sec = end.tv_sec-start.tv_sec-1;
        temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
    }
    else 
    {
        temp.tv_sec = end.tv_sec-start.tv_sec;
        temp.tv_nsec = end.tv_nsec-start.tv_nsec;
    }
    return temp;
}


void createDataFile(char* filename, int* data, int size)
{

    int out = open(filename, O_CREAT | O_TRUNC | O_RDWR, 0644);
    write(out, data, size);
    close(out);
}

int main(int argc, char** argv)
{
#ifdef TIMES
    struct timespec start, end, result;
    clock_gettime(CLOCK_REALTIME, &start);
#endif

    // for Manage signal
    struct sigaction usrctrl;
    usrctrl.sa_flags = 0;
    usrctrl.sa_handler = handler;
    sigemptyset(&usrctrl.sa_mask);
    sigaction(SIGUSR1, &usrctrl, (void*)0);

    sigset_t set;
    sigfillset(&set);
    sigdelset(&set, SIGUSR1);

    // To get server's pid
    int pid_shmid = shmget(234, sizeof(pid_t) * 3, 0);
    pid_t* pid_space = shmat(pid_shmid, (void*)0, 0);
    pid_t server_pid = pid_space[0];
    pid_space[2] = getpid();

    kill(pid_space[0], SIGUSR1);
    // for Loop
    int i, j;

    // for Data
    packet in;
    int packetIndex = KB_64_INDEX, packetSize = KB_64_SIZE;
    int* data_0, * data_1, * data_2, * data_3;
    data_0 = (int*)malloc(packetSize);
    data_1 = (int*)malloc(packetSize);
    data_2 = (int*)malloc(packetSize);
    data_3 = (int*)malloc(packetSize);

    // for Share data
    int clientId, clen;
    struct sockaddr_un cli;


    printf("======Start input data======\n");
    //printf("N = %d, Amount of Data = %d\n", N, dataSize);
    printf("PID : %d\n", getpid());

    printf("Create Socket\n");
    clientId = socket(AF_UNIX, SOCK_STREAM, 0);

    printf("Bind socket\n");
    memset((char*)&cli, 0, sizeof(struct sockaddr_un));
    cli.sun_family = AF_UNIX;
    strcpy(cli.sun_path, SOCKET_NAME);
    clen = sizeof(cli.sun_family) + strlen(cli.sun_path);

    printf("Connect Server ...\n");
    connect(clientId, (struct sockaddr*)&cli, clen);

    printf("Receive data from input\n");
    packetSize = recv(clientId, (packet*)&in, sizeof(in), 0);
    printf("%d %ld\n", in.dataIndex, in.dataSize);

    for (i = 0, j = 0; i < in.dataIndex; i++)
    {
        // printf(" %d ", i);
        switch (i % 4)
        {
        case 0:
            data_0[j] = i;
            break;
        case 1:
            data_1[j] = i;
            break;
        case 2:
            data_2[j] = i;
            break;
        case 3:
            data_3[j++] = i;
            break;
        }
    }
    putchar('\n');

    printf("Create data file\n");
    createDataFile("data0", data_0, packetSize);
    createDataFile("data1", data_1, packetSize);
    createDataFile("data2", data_2, packetSize);
    createDataFile("data3", data_3, packetSize);

    close(clientId);

    int my_number = 0;
    sigaction(SIGUSR1, &usrctrl, (void*)0);
    pid_t children[4] = { getpid(), 0, 0, 0 };

    int shmid_1 = shmget(123, sizeof(packet), 0);
    int shmid_2 = shmget(124, sizeof(packet), 0);
    int shmid_3 = shmget(125, sizeof(packet), 0);
    int shmid_4 = shmget(126, sizeof(packet), 0);
    packet* pkt[4];

    for(i = 0; i < 4;i++) {
        if(i == 0) pkt[i] = shmat(shmid_1, (void*)0, 0);
        if(i == 1) pkt[i] = shmat(shmid_2, (void*)0, 0);
        if(i == 2) pkt[i] = shmat(shmid_3, (void*)0, 0);
        if(i == 3) pkt[i] =  shmat(shmid_4, (void*)0, 0);
    }

    pkt[0]->dataSize = KB_64_SIZE / 4;
    pkt[0]->dataIndex = KB_64_INDEX / 4;
    pkt[1]->dataSize = KB_64_SIZE / 4;
    pkt[1]->dataIndex = KB_64_INDEX / 4;
    pkt[2]->dataSize = KB_64_SIZE / 4;
    pkt[2]->dataIndex = KB_64_INDEX / 4;
    pkt[3]->dataSize = KB_64_SIZE / 4;
    pkt[3]->dataIndex = KB_64_INDEX / 4;


    for (j = 1; j < 4; j++)
    {
        children[j] = fork();

        if (!children[j]) // child process
        {
            my_number = j;
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

        int count = pkt[0]->dataIndex;
        int temp;
        pid_space[1] = my_number;
        
        // Send to the shared memory
        for (i = 0; i < count; i++)
        {
            read(fd, &temp, sizeof(int));
            pkt[pid_space[1]]->data[i] = temp;
        }
        close(fd);
        kill(server_pid, SIGUSR1);
        sigsuspend(&set);

        for (i = 1; i < 4; i++)
        {
            kill(children[i], SIGUSR1);
            wait((void*)0);
        }
    }
    else
    {
        char name[6] = "data";
        char numbername[2] = { (char)my_number + '0', '\0' };
        strcat(name, numbername);
        pid_space[1] = my_number;
        int fd = open(name, O_RDONLY);

        int count = pkt[pid_space[1]]->dataIndex;
        int temp;

        // Send to the shared memory
        for (i = 0; i < count; i++)
        {
            read(fd, &temp, sizeof(int));
            pkt[pid_space[1]]->data[i] = temp;
        }
        close(fd);
        kill(server_pid, SIGUSR1);
        sigsuspend(&set);

        exit(0);
    }

#ifdef TIMES
    clock_gettime(CLOCK_REALTIME, &end);
    result = diff(start, end);

    printf("Elapsed Time : %ld.%ld sec\n", result.tv_sec, result.tv_nsec);
#endif
    shmdt(pid_space);
    shmdt(pkt);
    shmctl(pid_shmid, IPC_RMID, (void*)0);
    shmctl(shmid_1, IPC_RMID, (void*)0);
    shmctl(shmid_2, IPC_RMID, (void*)0);
    shmctl(shmid_3, IPC_RMID, (void*)0);
    shmctl(shmid_4, IPC_RMID, (void*)0);

    return 0;
}