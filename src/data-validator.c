#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        puts("Not enough arguments");
        exit(0);
    }

    int n = 0;
    int buffer;
    for (int i = 1; i < argc; i++)
    {
        int fd = open(argv[i], O_RDONLY);

        printf("====== File #%d : %s ======\n", i - 1, argv[i]);
        while ((n = read(fd, &buffer, sizeof(int))) > 0)
        {
            printf(" %d ", buffer);
        }
        printf("\n\n");
        close(fd);
    }

    puts("Done.");

    return 0;
}