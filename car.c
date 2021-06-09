#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>
#include <fcntl.h>
#include "car.h"

struct Car* createCar (int id) {
    struct Car* car = malloc(sizeof(struct Car));
    car->city = CITY_NONE;
    car->queue = CITY_NONE;
    car->id = id;

    // Create named semaphore for each car
    int length = snprintf( NULL, 0, "%d", id ) + 5;
    char* str = malloc(length);
    snprintf(str, length + 4, "/wc-%d", id);

    sem_unlink(str);
    car->sem = sem_open(str, O_CREAT, 0660, 1);
    if (car->sem == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    free(str);

    return car;
}