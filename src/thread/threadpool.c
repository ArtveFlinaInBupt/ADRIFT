#include "threadpool.h"


const int NUMBER = 2;

ThreadPool *threadPoolCreate(int min, int max, int queueCapacity)
{
    ThreadPool* pool = (ThreadPool*)malloc(sizeof(ThreadPool));
    do
    {
        if(pool == NULL)
        {
            printf("malloc threadpool fail...\n");
            break;
        }

        pool->threadIDs = (pthread_t*)malloc(sizeof(pthread_t) * max);
        if(pool->threadIDs == NULL)
        {
            printf("malloc threadIDs fail...\n");
            break;
        }
        memset(pool->threadIDs, 0, sizeof(pthread_t) * max);
        pool->minNum = min;
        pool->maxNum = max;
        pool->busyNum = 0;
        pool->liveNum = min;
        pool->exitNum = 0;

        if (pthread_mutex_init(&pool->mutexPool, NULL) != 0 ||
            pthread_mutex_init(&pool->mutexBusy, NULL) != 0 ||
            pthread_cond_init(&pool->notFull, NULL) != 0 ||
            pthread_cond_init(&pool->notEmpty, NULL) != 0)
        {
            printf("mutex or condition init fail...\n");
            break;
        }

        //任务队列
        pool->taskQ = (Task*)malloc(sizeof(Task) * queueCapacity);
        pool->queueCapacity = queueCapacity;
        pool->queueSize = 0;
        pool->queueFront = 0;
        pool->queueRear = 0;

        pool->shutdown = 0;

        //创建线程
        pthread_create(&pool->managerID, NULL, manager, pool);
        for(int i = 0; i < min; i++){
            pthread_create(&pool->threadIDs[i], NULL, worker, pool);
        }

        return pool;
    }while(0);

    //释放资源
    if(pool && pool->threadIDs) free(pool->threadIDs);
    if(pool && pool->taskQ) free(pool->taskQ);
    if(pool) free(pool);

    return NULL;
}

int threadPoolDestroy(ThreadPool* pool)
{
    if(pool == NULL)
    {
        return -1;
    }

    //关闭线程池
    pool->shutdown = 1;
    //阻塞回收管理者线程
    pthread_join(pool->managerID, NULL);
    //唤醒被阻塞的工作线程（一旦被唤醒，由于shutdown==1，在workder里面将被销毁）
    for(int i = 0; i < pool->liveNum; ++i)
    {
        pthread_cond_signal(&pool->notEmpty);
    }
    //释放堆内存
    if(pool->taskQ)
    {
        free(pool->taskQ);
    }
    if(pool->threadIDs)
    {
        free(pool->threadIDs);
    }
    
    pthread_mutex_destroy(&pool->mutexPool);
    pthread_mutex_destroy(&pool->mutexBusy);
    pthread_cond_destroy(&pool->notEmpty);
    pthread_cond_destroy(&pool->notFull);

    free(pool);
    pool = NULL;

    return 0;
}

void threadPoolAdd(ThreadPool* pool, void*func(void*), void* arg)
{
    pthread_mutex_lock(&pool->mutexPool);
    while(pool->queueSize == pool->queueCapacity && !pool->shutdown)
    {
        //当任务队列满的时候，阻塞线程
        pthread_cond_wait(&pool->notFull, &pool->mutexPool);
    }

    if(pool->shutdown)
    {
        pthread_mutex_unlock(&pool->mutexPool);
        return;
    }

    //添加任务
    pool->taskQ[pool->queueRear].function = func;
    pool->taskQ[pool->queueRear].arg = arg;
    pool->queueRear = (pool->queueRear + 1) % pool->queueCapacity;
    pool->queueSize++;

    pthread_cond_signal(&pool->notEmpty);
    pthread_mutex_unlock(&pool->mutexPool);
}

int threadPoolBusyNum(ThreadPool* pool)
{
    pthread_mutex_lock(&pool->mutexBusy);
    int busyNum = pool->busyNum;
    pthread_mutex_unlock(&pool->mutexBusy);
    return busyNum;
}

int threadPoolLiveNum(ThreadPool* pool)
{
    pthread_mutex_lock(&pool->mutexPool);
    int liveNum = pool->liveNum;
    pthread_mutex_unlock(&pool->mutexPool);
    return liveNum;
}

void* worker(void* arg)
{
    ThreadPool* pool = (ThreadPool*)arg;

    while(1)
    {
        pthread_mutex_lock(&pool->mutexPool);
        //当前任务队列是否为空
        while(pool->queueSize == 0 && !pool->shutdown)
        {
            //阻塞工作线程，等待条件满足
            pthread_cond_wait(&pool->notEmpty, &pool->mutexPool);

            //判断是否要销毁线程
            if(pool->exitNum > 0)
            {
                pool->exitNum--;
                if(pool->liveNum > pool->minNum)
                {
                    pool->liveNum--;
                    //线程解除阻塞后已经获得锁，在退出线程前需要解锁
                    pthread_mutex_unlock(&pool->mutexPool);
                    threadExit(pool);
                }
                
            }
        }

        //判断线程池是否被关闭
        if(pool->shutdown)
        {
            pthread_mutex_unlock(&pool->mutexPool); //避免死锁
            threadExit(pool);
        }

        //从任务队列中取出一个任务
        Task task;
        task.function = pool->taskQ[pool->queueFront].function;
        task.arg = pool->taskQ[pool->queueFront].arg;
        //移动队列头结点
        pool->queueFront = (pool->queueFront + 1) % pool->queueCapacity;
        pool->queueSize--;
        //解锁
        pthread_cond_signal(&pool->notFull);
        pthread_mutex_unlock(&pool->mutexPool);

        printf("thread %ld start working...\n", pthread_self());
        pthread_mutex_lock(&pool->mutexBusy);
        pool->busyNum++;
        pthread_mutex_unlock(&pool->mutexBusy);
        task.function(task.arg);
        free(task.arg);
        task.arg = NULL;

        printf("thread %ld end working...\n", pthread_self());
        pthread_mutex_lock(&pool->mutexBusy);
        pool->busyNum--;
        pthread_mutex_unlock(&pool->mutexBusy);

    }
    return NULL;
}

void* manager(void* arg)
{
    ThreadPool* pool = (ThreadPool*)arg;
    while(!pool->shutdown)
    {
        //每隔3s检测一次
        sleep(3);

        //取出线程池中任务的数量和当前线程的数量
        pthread_mutex_lock(&pool->mutexPool);
        int queueSize = pool->queueSize;
        int liveNum = pool->liveNum;
        pthread_mutex_unlock(&pool->mutexPool);

        //取出忙的线程的数量
        pthread_mutex_lock(&pool->mutexBusy);
        int busyNum = pool->busyNum;
        pthread_mutex_unlock(&pool->mutexBusy);

        //添加线程
        //任务的个数>存活的线程 && 存活的线程数<最大线程数
        if(queueSize > liveNum && liveNum < pool->maxNum)
        {
            pthread_mutex_lock(&pool->mutexPool);
            int counter = 0;
            for(int i = 0; i < pool->maxNum && counter < NUMBER
                && pool->liveNum < pool->maxNum; ++i)
            {
                if(pool->threadIDs[i] == 0)
                {
                    pthread_create(&pool->threadIDs[i], NULL, worker, pool);
                    counter++;
                    pool->liveNum++;
                }
            }
            pthread_mutex_unlock(&pool->mutexPool);

        }

        //销毁线程
        //忙的线程*2 < 存活的线程数 && 存活的线程数 > 最小线程数
        if(busyNum * 2 < liveNum && liveNum > pool->minNum)
        {
            pthread_mutex_lock(&pool->mutexPool);
            pool->exitNum = NUMBER;
            pthread_mutex_unlock(&pool->mutexPool);
            //让工作的线程自杀（多的工作线程由阻塞状态主动退出）
            for(int i = 0; i < NUMBER; ++i)
            {
                pthread_cond_signal(&pool->notEmpty);//唤醒线程，并重新申请获取互斥锁
            }
        } 

    }
    return NULL;
}

//把需要销毁的线程id在threadIDs数组里变为0
void threadExit(ThreadPool* pool)
{
    pthread_t tid = pthread_self();
    for(int i = 0; i < pool->maxNum; ++i)
    {
        if(pool->threadIDs[i] == tid)
        {
            pool->threadIDs[i] = 0;
            printf("threadExit() called, %ld exiting...\n", tid);
            break;
        }
    }
    pthread_exit(NULL);
}