#pragma once

#include <semaphore.h>

struct CarQueueElement {
    struct Car* car;
    struct CarQueueElement* next;
};

struct CarQueue {
    sem_t* sem;
    pthread_mutex_t mut;
    struct CarQueueElement* start;
    struct CarQueueElement* end;
};

struct CarQueue* createQueue ();
struct Car* pop (struct CarQueue* q);
void push (struct CarQueue* q, struct Car* car);
