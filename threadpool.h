#ifndef _THREADPOOL_H
#define _THREADPOOL_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
//����
typedef struct Task
{
    void (*function) (void* arg);
    void* arg;
}Task;

//�̳߳�
typedef struct Threadpool
{
    //�������
    Task* taskQ;
    int queueCapacity;           //����
    int queueSize;               //��ǰ�������
    int queueFront;              //��ͷ��ȡ����
    int queueRear;               //��β��������

    pthread_t managerID;         //�������߳�ID
    pthread_t* threadIDs;        //�������߳�ID
    int minNum;                  //��С�߳�����
    int maxNum;                  //����߳�����
    int busyNum;                 //æ���̸߳���
    int liveNum;                 //�����̸߳���
    int exitNum;                 //Ҫ���ٵ��̸߳���
    pthread_mutex_t mutexPool;   //���������̳߳�
    pthread_mutex_t mutexBusy;   //��busyNum����
    pthread_cond_t notFull;      //��������Ƿ�����
    pthread_cond_t notEmpty;     //��������Ƿ����

    int shutdown;                //�Ƿ������̳߳أ�����Ϊ1��������Ϊ0
}ThreadPool;


//�����̳߳ز���ʼ��
ThreadPool *threadPoolCreate(int min, int max, int queueSize);

//�����̳߳�
int threadPoolDestroy(ThreadPool* pool);

//���̳߳��������
void threadPoolAdd(ThreadPool* pool, void(*func)(void*), void* arg);

//��ȡ�̳߳��й������̸߳���
int threadPoolBusyNum(ThreadPool* pool);

//�����̳߳��л��ŵ��̸߳���
int threadPoolLiveNum(ThreadPool* pool);

void* worker(void* arg);
void* manager(void* arg);
void threadExit(ThreadPool* pool);


#endif  //_THREADPOOL_H