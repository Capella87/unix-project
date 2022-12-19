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
#include <sys/times.h>
#include <time.h>
#include <sys/time.h>

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
    int pid_shmid = shmget(234, sizeof(pid_t) * 3, IPC_CREAT | 0644);
    pid_t* pid_space = shmat(pid_shmid, 0, 0);
    pid_space[0] = getpid();

    sigaction(SIGUSR1, &sat, (void*)0);
    printf("Run the data_provider and server_client in order.\n");
    sigsuspend(&set);

    // Making Shared Memory and be suspended to wait
    int shmid_1 = shmget(123, sizeof(packet), IPC_CREAT | 0644);
    packet* pkt[4];

    pkt[0] = shmat(shmid_1, (void*)0, 0);

    int shmid_2 = shmget(124, sizeof(packet), IPC_CREAT | 0644);
    pkt[1] = shmat(shmid_2, (void*)0, 0);

    int shmid_3 = shmget(125, sizeof(packet), IPC_CREAT | 0644);
    pkt[2] = shmat(shmid_3, (void*)0, 0);
    
    int shmid_4 = shmget(126, sizeof(packet), IPC_CREAT | 0644);
    pkt[3] = shmat(shmid_4, (void*)0, 0);

    pkt[0]->dataSize = KB_64_SIZE / 4;
    pkt[0]->dataIndex = KB_64_INDEX / 4;
    pkt[1]->dataSize = KB_64_SIZE / 4;
    pkt[1]->dataIndex = KB_64_INDEX / 4;
    pkt[2]->dataSize = KB_64_SIZE / 4;
    pkt[2]->dataIndex = KB_64_INDEX / 4;
    pkt[3]->dataSize = KB_64_SIZE / 4;
    pkt[3]->dataIndex = KB_64_INDEX / 4;

    printf("PID = %d\n", getpid());
    printf("Waiting for gathering all data at client #0...\n");

    int dummy[KB_64_INDEX / 2] = { 0, };

    int fd = open("received1", O_CREAT | O_RDWR, 0644);
    write(fd, dummy, pkt[0]->dataSize * 2);
    close(fd);
    int sfd = open("received2", O_CREAT | O_RDWR, 0644);
    write(sfd, dummy, pkt[0]->dataSize * 2);
    close(sfd);

#ifdef TIMES
    struct timespec start, end, result;
    clock_gettime(CLOCK_REALTIME, &start);
#endif

    pid_t pids[2] = { getpid(), 0 };
    int module_idx = KB_64_INDEX / 8;

    int count = 0;
    while (count < 4)
    {
        sigsuspend(&set);
        pids[1] = fork();
        if (pids[0] == getpid())
        {
            fd = open("received1", O_CREAT | O_RDWR, 0644);

            lseek(fd, sizeof(int) * pid_space[1], SEEK_SET);
            for (int i = 0; i < module_idx; i++)
            {
                write(fd, &pkt[pid_space[1]]->data[i], sizeof(int));
                lseek(fd, sizeof(int) * 3, SEEK_CUR);
            }
            close(fd);
            puts("done.");

            wait((void*)0);
            kill(pid_space[2], SIGUSR1);
            count++;
        }
        else
        {
            int fd = open("received2", O_CREAT | O_RDWR, 0644);
            lseek(fd, sizeof(int) * pid_space[1], SEEK_SET);
            for (int i = module_idx; i < module_idx * 2; i++)
            {
                write(fd, &pkt[pid_space[1]]->data[i], sizeof(int));
                lseek(fd, sizeof(int) * 3, SEEK_CUR);
            }
            close(fd);
            puts("done.");

            exit(0);
        }
    }
    puts("Bye");
#ifdef TIMES
    clock_gettime(CLOCK_REALTIME, &end);
    result = diff(start, end);

    printf("Elapsed Time : %ld.%ld sec\n", result.tv_sec, result.tv_nsec);
#endif

    return 0;
}