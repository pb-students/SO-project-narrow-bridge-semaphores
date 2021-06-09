#pragma once

#include <semaphore.h>

struct LongQueueElement {
    struct Car* car;
    struct LongQueueElement* next;
};

struct LongQueue {
    sem_t* sem;
    pthread_mutex_t mut;
    struct LongQueueElement* start;
    struct LongQueueElement* end;
};

struct LongQueue* createQueue ();
struct Car* pop (struct LongQueue* q);
void push (struct LongQueue* q, struct Car* car);
