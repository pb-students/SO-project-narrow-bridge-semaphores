#pragma once

#define CITY_NONE  -1
#define CITY_A 0
#define CITY_B 1

struct Car {
    int city;
    int queue;
    int id;

    sem_t* sem;
};

struct Car* createCar(int id);
