#include <unistd.h>
#include <stdio.h>

#define __USE_GNU
#include <sched.h>
#include <pthread.h>

void *func(void *arg)
{
    for (;;)
        ;
}

int main()
{
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(0, &set);
    if (sched_setaffinity(0, sizeof(set), &set) == -1)
    {
        return -1;
    }
    pthread_t threads[5];
    for (int i = 0; i < 5; ++i)
    {
        pthread_create(&threads[i], NULL, &func, NULL);
    }
    for (int i = 0; i < 5; ++i)
    {
        pthread_join(threads[i], NULL);
    }
    return 0;
}