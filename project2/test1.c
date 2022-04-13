#include <unistd.h>
#include <stdio.h>

#define __USE_GNU
#include <sched.h>
#include <pthread.h>

int main()
{
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(0, &set);
    if (sched_setaffinity(0, sizeof(set), &set) == -1)
    {
        return -1;
    }
    pid_t pid = getpid();
    struct sched_param param;
    param.sched_priority = 1;
    sched_setscheduler(pid, SCHED_RR, &param);
    for (;;)
        ;
    return 0;
}