#include <stdio.h>
#include "threadpool.h"
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>

void testFunc(void* arg)
{
    int num = *(int*)arg;
    printf("thread %ld is working, number = %d\n", 
        pthread_self(), num);
    usleep(1000);
}

int main(int argc, char** argv)
{
    ThreadPool* pool = threadPoolCreate(3, 10, 100);
    for(int i = 0; i < 100; i++)
    {
        int* num = (int*)malloc(sizeof(int));
        *num = i;
        threadPoolAdd(pool, testFunc, num);
    }

    sleep(2);

    // threadPoolDestroy(pool);
    return 0;
}