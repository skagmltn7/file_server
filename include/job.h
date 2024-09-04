#ifndef __JOB_H__
#define __JOB_H__

#include "message.h"
#include <pthread.h>

typedef struct{
    int client_socket;
    _Message* message;
} Job;

typedef struct Node{
    Job* job;
    struct Node* next;
} Node;

typedef struct{
    Node* head;
    Node* tail;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} JobQueue;

void init_queue(JobQueue* queue);
void push(JobQueue* queue, Job* job);
Job* pop(JobQueue* queue);

#endif