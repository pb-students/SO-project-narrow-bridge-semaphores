#include <stdio.h>
#include <inttypes.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>
#include <getopt.h>

#include "urandom.h"
#include "queue.h"
#include "car.h"

//#define DEBUG
#define DEBUG_NUM_CARS 7

struct LongQueue* carQueue = NULL;
struct Car* currentCar = NULL;
struct Car** cars = NULL;
int num = 0;
int debug = 0;

pthread_mutex_t mutCurrentCar = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutBridge = PTHREAD_MUTEX_INITIALIZER;

// Counts cars in the desired city
int countCarsInCity (int city) {
    int res = 0;
    for (int i = 0; i < num; ++i) {
        if (cars[i] != NULL && cars[i]->city == city) {
            res += 1;
        }
    }

    return res;
}

// Counts cars in queue to the desired city
int countCarsInQueue (int city) {
    int res = 0;
    for (int i = 0; i < num; ++i) {
        if (cars[i] != NULL && cars[i]->queue == city) {
            res += 1;
        }
    }

    return res;
}

void printQueue (int city) {
    struct LongQueueElement* curr = carQueue->start;
    while (curr != NULL) {
        if (curr->car->queue == city) {
            printf("%d ", curr->car->id);
        }

        curr = curr->next;
    }
}

void print () {
    // NOTE: Well there's a possibility for race condition just for currentCar variable
    //       Since we do not care for any logic here, its acceptable that we move it
    //       to the local scope and just ignore it.
    struct Car* curr = currentCar;

    int inCityA = countCarsInCity(CITY_A);
    int inCityB = countCarsInCity(CITY_B);
    int inQueueToB = countCarsInQueue(CITY_B);
    int inQueueToA = countCarsInQueue(CITY_A);

    if (curr == NULL) {
        printf(
                "A-%d %d>>> [       ] <<<%d %d-B\n",
                inCityA - inQueueToB,
                inQueueToB,
                inQueueToA,
                inCityB - inQueueToA
        );

        if (debug) {
            printf("A->B: ");
            printQueue(CITY_B);
            printf("\n");
            printf("B->A: ");
            printQueue(CITY_A);
            printf("\n");
        }

        fflush(stdout);
        return;
    }

    int id = curr->id;
    char* dir = curr->queue == CITY_B ? ">>" : "<<";

    printf(
        "A-%d %d>>> [%s %d %s] <<<%d %d-B\n",
        inCityA - inQueueToB,
        inQueueToB,
        dir, id, dir,
        inQueueToA,
        inCityB - inQueueToA
    );


    if (debug) {
        printf("A->B: ");
        printQueue(CITY_B);
        printf("\n");
        printf("B->A: ");
        printQueue(CITY_A);
        printf("\n");
    }

    fflush(stdout);
}

void city () {
    // Some city logic
    sleep(2 + (urandom() % 7));
}

_Noreturn void* carRoutine (void* arg) {
    long i = (long) arg;
    struct Car* car = cars[i];

    for (;;) {
        // Car enters the city
        sem_wait(car->sem);
        city();

        // Car goes to the bridge queue
        if (pthread_mutex_lock(&mutCurrentCar) != 0) {
            perror("mutCurrentCar lock");
            exit(EXIT_FAILURE);
        }

        car->queue = !car->city;
        push(carQueue, car);

        if (pthread_mutex_unlock(&mutCurrentCar) != 0) {
            perror("mutCurrentCar unlock");
            exit(EXIT_FAILURE);
        }
    }
}

_Noreturn void* bridgeRoutine (void* arg) {
    for (;;) {
        // We're getting first car from the car queue
        struct Car* car = pop(carQueue);

        // Bridge is busy
        if (pthread_mutex_lock(&mutBridge) != 0) {
            perror("mutBridge lock");
            exit(EXIT_FAILURE);
        }

        currentCar = car;

        // NOTE: Let's say that going through bridge takes 1 sec
        //       We can comment out this expression and it would still work
        sleep(1);
        print();

        // Car enters the city
        car->queue = CITY_NONE;
        car->city = !car->city;
        if (sem_post(car->sem) != 0) {
            perror("car sem_post");
            exit(EXIT_FAILURE);
        }

        // Bridge is free to go
        currentCar = NULL;
        if (pthread_mutex_unlock(&mutBridge) != 0) {
            perror("mutBridge unlock");
            exit(EXIT_FAILURE);
        }
    }
}

int main (int argc, char** argv) {
#ifndef DEBUG
    if (argc < 2) {
        fprintf(stderr, "Usage: %s [-debug] <number of cars>\n", argv[0]);
        return 1;
    }

    static struct option long_options[] = {
            {"debug", 0, NULL, 'd'},
            {NULL, 0, NULL, 0}
    };

    int c;
    while (optind < argc) {
        if ((c = getopt_long(argc, argv, "d:", long_options, NULL)) != -1) {
            switch (c) {
                case 'd':
                    debug = 1;
                    break;

                default:
                    abort();
            }
        } else {
            num = strtoumax(argv[optind++], NULL, 10);
            if (num == UINTMAX_MAX && errno == ERANGE || num < 1) {
                fprintf(stderr, "Number of cars should be more than 0\n");
                return 2;
            }
        }
    }

#else
    num = DEBUG_NUM_CARS;
    debug = 1;
#endif

    // Create global car queue
    carQueue = createQueue();

    // Create cars
    cars = malloc(sizeof(struct Car) * num);
    if (cars == NULL) {
        perror("malloc - cars");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < num; ++i) {
        cars[i] = createCar(i);
    }

    // Put cars to the random cities
    for (int i = 0; i < num; ++i) {
        cars[i]->city = urandom() % 2 == 0
                ? CITY_A
                : CITY_B;

        // Show the update
        print();
    }

    // Create and start car and bridge threads
    pthread_attr_t attr;
    if (pthread_attr_init(&attr) != 0) {
        perror("pthread_attr_init");
        exit(EXIT_FAILURE);
    }

    if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE) != 0) {
        perror("pthread_attr_setdetachstate");
        exit(EXIT_FAILURE);
    }

    pthread_t threads[num];
    for (int i = 0; i < num; ++i) {
        pthread_t thread;
        if (pthread_create(&thread, &attr, &carRoutine, (void *) (long) i) != 0) {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }

        threads[i] = thread;
    }

    pthread_t bridgeThread;
    if (pthread_create(&bridgeThread, &attr, &bridgeRoutine, NULL)) {
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < num; ++i) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("pthread_join");
            exit(EXIT_FAILURE);
        }
    }

    if (pthread_join(bridgeThread, NULL) != 0) {
        perror("pthread_join");
        exit(EXIT_FAILURE);
    }

    // NOTE: Cleanup is useless as we do not end the threads
    // pthread_mutex_destroy(&mutBridge);
    // pthread_mutex_destroy(&mutCurrentCar);

    return 0;
}
