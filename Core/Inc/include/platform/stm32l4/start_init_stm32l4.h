#ifndef START_INIT_STM32L4
#define START_INIT_STM32L4

#include "displayDriver.h"
#include <stddef.h>

typedef struct MainDisplayDrivers {
    DisplayDriver* displayDriverArray;
    size_t driverCount;
} MainDisplayDrivers;

MainDisplayDrivers* init_getMainDisplayDrivers();

#endif
