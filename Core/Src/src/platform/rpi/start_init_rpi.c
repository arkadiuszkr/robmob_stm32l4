#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "start_init_stm32l4.h"
#include "displayDriver.h"

MainDisplayDrivers* _mainDisplayDrivers_init = NULL;

MainDisplayDrivers* init_getMainDisplayDrivers()
{
    if (_mainDisplayDrivers_init != NULL) return _mainDisplayDrivers_init;

    _mainDisplayDrivers_init = malloc(sizeof(MainDisplayDrivers));
    if (!_mainDisplayDrivers_init) { perror("_mainDisplayDrivers_init malloc failed!"); exit(EXIT_FAILURE); }
    
    // ---------- Add terminal UART driver ----------
    DisplayDriver* terminalDriver_ptr = getDisplayDriver(TERMINAL_UART);
    if (!terminalDriver_ptr) { perror("Terminal driver not found!"); exit(EXIT_FAILURE); }
    
    // ---------- Add LCD screen driver ----------

    // ---------- Allocate driver array ----------
    size_t driverCount = 1;
    DisplayDriver* driversArray = malloc(driverCount * sizeof(DisplayDriver));
    if (!driversArray) { perror("Malloc failed for driversArray!"); free(_mainDisplayDrivers_init); exit(EXIT_FAILURE); }
    memcpy(&driversArray[0], terminalDriver_ptr, sizeof(DisplayDriver));


    _mainDisplayDrivers_init->displayDriverArray = driversArray;
    _mainDisplayDrivers_init->driverCount = driverCount;
    return _mainDisplayDrivers_init;
}
