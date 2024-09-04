#include "job.h"

void init_queue(JobQueue *queue) {
    queue->head = NULL;
    queue->tail = NULL;
    pthread_mutex_init(&queue->mutex, NULL);
    pthread_cond_init(&queue->cond, NULL);
}

void push(JobQueue* queue, Job* job) {
    Node* new_node = (Node *)malloc(sizeof(Node));

    new_node->job = job;
    new_node->next = NULL;
    pthread_mutex_lock(&queue->mutex);
    if (queue->tail) {
        queue->tail->next = new_node;
    } else {
        queue->head = new_node;
    }
    queue->tail = new_node;
    pthread_cond_signal(&queue->cond);
    pthread_mutex_unlock(&queue->mutex);
}

Job* pop(JobQueue* queue) {
    pthread_mutex_lock(&queue->mutex);
    while (queue->head == NULL) {
        pthread_cond_wait(&queue->cond, &queue->mutex);
    }
    Node* node = queue->head;
    Job* job = node->job;
    queue->head = node->next;
    if (queue->head == NULL) {
        queue->tail = NULL;
    }
    free(node);
    pthread_mutex_unlock(&queue->mutex);
    return job;
}
