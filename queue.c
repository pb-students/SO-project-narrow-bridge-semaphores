#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <pthread.h>
#include "queue.h"
struct LongQueue* createQueue () {
    static int i = 0;
    struct LongQueue* q = malloc(sizeof(struct LongQueue));
    q->start = NULL;

    int length = snprintf( NULL, 0, "%d", i ) + 6;
    char* str = malloc(length);
    snprintf(str, length + 5, "/wlq-%d", i);

    sem_unlink(str);
    q->sem = sem_open(str, O_CREAT, 0660, 0);
    if (q->sem == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    free(str);

    pthread_mutex_init(&q->mut, NULL);
    return q;
}

void push (struct LongQueue* q, struct Car* value) {
    struct LongQueueElement* el = malloc(sizeof(struct LongQueueElement));
    el->next = NULL;
    el->car = value;

    pthread_mutex_lock(&q->mut);
    if (q->end == NULL) {
        q->start = el;
        q->end = el;

        pthread_mutex_unlock(&q->mut);
        sem_post(q->sem);
        return;
    }

    q->end->next = el;
    q->end = el;
    pthread_mutex_unlock(&q->mut);

    // Bump up the semaphore
    sem_post(q->sem);
}

struct Car* pop (struct LongQueue* q) {
    // Wait for the semaphore
    sem_wait(q->sem);

    if (q->start == NULL) {
        return NULL;
    }

    pthread_mutex_lock(&q->mut);

    if (q->start == q->end) {
        q->end = NULL;
    }

    struct Car* popped = q->start->car;
    q->start = q->start->next;
    pthread_mutex_unlock(&q->mut);

    // TODO: free the popped element
    return popped;
}
