#include <stdio.h>
#include <dirent.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define PATHBUFFERSIZE 0x100
#define STRBUFFERSIZE 0x10000

int isValidPid(char *pid)
{
    for (int i = 0; pid[i] != '\0'; ++i)
    {
        if (pid[i] < '0' || pid[i] > '9')
        {
            return 0;
        }
    }
    return 1;
}

char getS(char *stat)
{
    int spaceCount = 0, i = 0;
    for (; spaceCount < 2; ++i)
    {
        if (stat[i] == ' ')
        {
            ++spaceCount;
        }
    }
    return stat[i];
}

int main(int argc, char *argv[])
{
    char pathbuf[PATHBUFFERSIZE] = "/proc", strbuf[STRBUFFERSIZE];
    int index1 = strlen(pathbuf);
    DIR *dir = opendir(pathbuf);
    struct dirent *dirent;
    if (dir == NULL)
    {
        fprintf(stderr, "cannot open proc\n");
        return -1;
    }
    printf("  PID S CMD\n");
    while ((dirent = readdir(dir)))
    {
        if (dirent->d_type == 4 && isValidPid(dirent->d_name))
        {
            pathbuf[index1] = '/';
            strcpy(pathbuf + index1 + 1, dirent->d_name);
            int index2 = strlen(pathbuf), pidlen = strlen(dirent->d_name);
            DIR *subdir = opendir(pathbuf);
            if (pidlen < 5 && subdir != NULL)
            {
                pathbuf[index2] = '/';
                for (int i = 0; i < 5 - pidlen; ++i)
                {
                    printf(" ");
                }
                printf("%s", dirent->d_name);
                strcpy(pathbuf + index2 + 1, "stat");
                memset((void *)strbuf, 0, sizeof(strbuf));
                int fd = open(pathbuf, O_RDONLY);
                if (fd > 0)
                {
                    if (read(fd, strbuf, STRBUFFERSIZE) != STRBUFFERSIZE)
                    {
                        printf(" %c ", getS(strbuf));
                    }
                }
                close(fd);
                memset((void *)strbuf, 0, sizeof(strbuf));
                int flag = 0;
                strcpy(pathbuf + index2 + 1, "cmdline");
                if ((fd = open(pathbuf, O_RDONLY)) >= 0)
                {
                    int len = read(fd, strbuf, STRBUFFERSIZE);
                    if (len != 0 && len != STRBUFFERSIZE)
                    {
                        printf("%s", strbuf);
                        flag = 1;
                    }
                    close(fd);
                }
                strcpy(pathbuf + index2 + 1, "comm");
                memset((void *)strbuf, 0, sizeof(strbuf));
                if (!flag && (fd = open(pathbuf, O_RDONLY)) >= 0)
                {
                    if (read(fd, strbuf, STRBUFFERSIZE) != STRBUFFERSIZE)
                    {
                        strbuf[strlen(strbuf) - 1] = '\0';
                        printf("[%s]", strbuf);
                    }
                    close(fd);
                }
                printf("\n");
            }
            closedir(subdir);
        }
    }
    closedir(dir);
    return 0;
}