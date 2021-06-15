#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>
#include "queue.h"
struct CarQueue* createQueue () {
    static int i = 0;
    struct CarQueue* q = malloc(sizeof(struct CarQueue));
    if (q == NULL) {
        perror("malloc - queue");
        exit(EXIT_FAILURE);
    }
    q->start = NULL;

    int length = snprintf( NULL, 0, "%d", i ) + 6;
    char* str = malloc(length);
    if (str == NULL) {
        perror("malloc - queueName");
        exit(EXIT_FAILURE);
    }
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

void push (struct CarQueue* q, struct Car* value) {
    struct CarQueueElement* el = malloc(sizeof(struct CarQueueElement));
    if (el == NULL) {
        perror("malloc - queueElement");
        exit(EXIT_FAILURE);
    }
    el->next = NULL;
    el->car = value;

    if ((errno = pthread_mutex_lock(&q->mut)) != 0) {
        perror("queue lock");
        exit(EXIT_FAILURE);
    }

    if (q->end == NULL) {
        q->start = el;
        q->end = el;

        if ((errno = pthread_mutex_unlock(&q->mut)) != 0) {
            perror("queue unlock");
            exit(EXIT_FAILURE);
        }

        // Bump up the semaphore
        if ((errno = sem_post(q->sem)) != 0) {
            perror("queue sem_post");
            exit(EXIT_FAILURE);
        }
        return;
    }

    q->end->next = el;
    q->end = el;

    if ((errno = pthread_mutex_unlock(&q->mut)) != 0) {
        perror("queue unlock");
        exit(EXIT_FAILURE);
    }

    // Bump up the semaphore
    if ((errno = sem_post(q->sem)) != 0) {
        perror("queue sem_post");
        exit(EXIT_FAILURE);
    }
}

struct Car* pop (struct CarQueue* q) {
    // Wait for the semaphore
    sem_wait(q->sem);

    if (q->start == NULL) {
        return NULL;
    }

    if ((errno = pthread_mutex_lock(&q->mut)) != 0) {
        perror("queue lock");
        exit(EXIT_FAILURE);
    }

    if (q->start == q->end) {
        q->end = NULL;
    }

    struct Car* car = q->start->car;
    struct CarQueueElement* popped = q->start;
    q->start = q->start->next;

    if ((errno = pthread_mutex_unlock(&q->mut)) != 0) {
        perror("queue unlock");
        exit(EXIT_FAILURE);
    }

    free(popped);
    return car;
}
