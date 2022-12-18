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
    sigset_t set;
    sigfillset(&set);
    sigdelset(&set, SIGUSR1);
    struct sigaction sat;
    sat.sa_flags = 0;
    sigemptyset(&sat.sa_mask);
    sat.sa_handler = handler;

    // Create a shared memory to share their pid
    int pid_shmid = shmget(234, sizeof(pid_t) * 2, IPC_CREAT | 0644);
    pid_t* pid_space = shmat(pid_shmid, 0, 0);
    pid_space[0] = getpid();

    sigaction(SIGUSR1, &sat, (void*)0);
    printf("Run the data_provider and server_client in order.\n");
    sigsuspend(&set);

    // Making Shared Memory and be suspended to wait
    int shmid = shmget(123, sizeof(packet), IPC_CREAT | 0644);
    packet* pkt = shmat(shmid, (void*)0, 0);

    pkt->dataSize = KB_1_SIZE;
    pkt->dataIndex = KB_1_INDEX;

    printf("PID = %d\n", getpid());
    printf("Waiting for gathering all data at client #0...\n");
    sigsuspend(&set);
    
    pid_t pids[2] = { getpid(), fork() };

    if (pids[0] == getpid())
    {
        int fd = open("received1", O_CREAT | O_RDWR, 0644);

        write(fd, pkt->data, pkt->dataSize / 2);
        close(fd);
        puts("done.");
        wait((void*)0);
    }
    else
    {
        int fd = open("received2", O_CREAT | O_RDWR, 0644);
        write(fd, pkt->data + pkt->dataIndex / 2, pkt->dataSize / 2 );
        close(fd);
        puts("done.");
    }

    return 0;
}