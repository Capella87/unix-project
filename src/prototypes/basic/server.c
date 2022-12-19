#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <sys/un.h>
#include <sys/sem.h>
#include <errno.h>
#include <stdbool.h>

#define SOCKET_NAME "root"

#define KB_1_INDEX 1024 * 1
#define KB_1_SIZE 4 * 1024 * 1

#define KB_64_INDEX 1024*64
#define KB_64_SIZE 4*1024*64

#define KB_256_INDEX 1024*256
#define KB_256_SIZE 4*1024*256

typedef struct PACKET {
    int id;
    int dataIndex;
    size_t dataSize;
    int data[KB_64_INDEX];
} packet;

void handler(int signo) {
}

void createDataFile(char * filename, int * data) {
    int out = open(filename, O_CREAT | O_TRUNC | O_RDWR, 0644);
    write(out, data, sizeof(data));
    close(out);
}

typedef union semun {
	int val;
	struct semid_ds *buf;
	ushort *array;
}semun;

int initsem() {
	semun semunarg;
	int status = 0, semid;

	semid = semget(126, 1, IPC_CREAT | IPC_EXCL | 0600);
	if (semid == -1) {
		if (errno == EEXIST)
			semid = semget(126, 1, 0);
	}
	else {
		semunarg.val = 1;
		status = semctl(semid, 0, SETVAL, semunarg);
	}

	if (semid == -1 || status == -1) {
		perror("initsem");
		return(-1);
	}
	return semid;
}

int semlock(int semid) {
	struct sembuf buf;

	buf.sem_num = 0;
	buf.sem_op = -1;
	buf.sem_flg = SEM_UNDO;
	if (semop(semid, &buf, 1) == -1) {
		perror("semlock failed");
		exit(1);
	}
	return 0;
}

int semunlock(int semid) {
	struct sembuf buf;

	buf.sem_num = 0;
	buf.sem_op = 1;
	buf.sem_flg = SEM_UNDO;
	if (semop(semid, &buf, 1) == -1) {
		perror("semunlock failed");
		exit(1);
	}
	return 0;
}

int main(int argc, char** argv) {

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

    // for Share data
    int clientId, clen, pid;
    struct sockaddr_un cli;


    // for semaphore
    int semid = initsem();

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

    printf("Receive data from input\n");
    recv(clientId, (packet *)&in, sizeof(in), 0);
    printf("%d %d %ld\n", in.id, in.dataIndex, in.dataSize);

    printf("Finish data Transmisson\n");

    int serverId, len, sid;
    struct sockaddr_un ser;
    printf("======Start data Trasmisson to output======\n");

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

    printf("Waiting for Connection ...\n");
    clientId = accept(serverId, (struct sockaddr *)&cli, &clen);
    printf("CONNECTION SUCCESS\n");

    // PID 받기
    int inid;
    printf("Recieve PID : ");
    recv(clientId, (int *)&inid, sizeof(int), 0);
    printf("%d\n", inid);

    // PID 보내기
    printf("Send PID\n");
    send(clientId, (int *)&pid, sizeof(int), 0);
    kill(inid, SIGUSR1);

    int my_number = 0;
    packet out_0, out_1, out_2, out_3;
    pid_t children[4] = { getpid(), 0, 0, 0 };

    for (int i = 1; i < 4; i++) {

        children[i] = fork();

        if (!children[i]) { // child process
            my_number = i;
            break;
        }

    }

    if (my_number == 0) {

        int nodeid = getpid();
        printf("NODE #0 PID : %d\n", nodeid);

        // PID 보내기
        send(clientId, (int *)&nodeid, sizeof(int), 0);
        kill(inid, SIGUSR1);

        // DATA 보내기
        out_0.id = 0;
        out_0.dataIndex = in.dataIndex / 4;
        out_0.dataSize = in.dataSize / 4;

        printf("Send NODE #%d Packet", out_0.id);
        for (i = 0; i < in.dataIndex; i += 4) {
            out_0.data[i] = i;
            //printf(" %d ", out_0.data[i]);
        } putchar('\n');
        createDataFile("data0", out_0.data);

        send(clientId, (packet *)&out_0, sizeof(in), 0);
        kill(inid, SIGUSR1);

    }

    /*

    if (my_number == 1) {

        semlock(semid);

        int nodeid = getpid();
        printf("NODE #1 PID : %d\n", nodeid);

        // PID 보내기
        send(clientId, (int *)&nodeid, sizeof(int), 0);
        kill(inid, SIGUSR1);
        sigsuspend(&set);

        // DATA 보내기
        kill(inid, SIGUSR1);
        out_1.id = 1;
        out_1.dataIndex = in.dataIndex / 4;
        out_1.dataSize = in.dataSize / 4;

        printf("Send NODE #%d Packet", out_1.id);
        for (i = 1; i < in.dataIndex; i += 4) {
            out_1.data[i] = in.data[i];
            //printf(" %d ", out_1.data[i]);
        } putchar('\n');
        createDataFile("data1", out_1.data);

        send(clientId, (packet *)&out_1, sizeof(in), 0);
        kill(inid, SIGUSR1);
        semunlock(semid);
    }

    if (my_number == 2) {

        semlock(semid);

        int nodeid = getpid();
        printf("NODE #2 PID : %d\n", nodeid);

        // PID 보내기
        send(clientId, (int *)&nodeid, sizeof(int), 0);
        sigsuspend(&set);

        // DATA 보내기
        kill(inid, SIGUSR1);
        out_2.id = 2;
        out_2.dataIndex = in.dataIndex / 4;
        out_2.dataSize = in.dataSize / 4;

        printf("Send NODE #%d Packet", out_2.id);
        for (i = 2; i < in.dataIndex; i += 4) {
            out_2.data[i] = in.data[i];
            //printf(" %d ", out_2.data[i]);
        } putchar('\n');
        createDataFile("data2", out_2.data);
        send(clientId, (packet *)&out_2, sizeof(in), 0);
        kill(inid, SIGUSR1);
        semunlock(semid);
    }


    if (my_number == 3) {

        semlock(semid);

        int nodeid = getpid();
        printf("NODE #3 PID : %d\n", nodeid);

        // PID 보내기
        send(clientId, (int *)&nodeid, sizeof(int), 0);
        sigsuspend(&set);

        // DATA 보내기
        kill(inid, SIGUSR1);
        out_3.id = 3;
        out_3.dataIndex = in.dataIndex / 4;
        out_3.dataSize = in.dataSize / 4;


        printf("Send NODE #%d Packet", out_3.id);
        for (i = 3; i < in.dataIndex; i += 4) {
            out_3.data[i] = in.data[i];
            //printf(" %d ", out_3.data[i]);
        } putchar('\n');
        createDataFile("data3", out_3.data);
        send(clientId, (packet *)&out_3, sizeof(in), 0);
        kill(inid, SIGUSR1);

        semunlock(semid);
    }
    */
    return 0;
}