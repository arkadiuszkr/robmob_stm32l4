#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "displayDriver.h"
#include "main_Robot.h"
#include "start_init_stm32l4.h"

MainDisplayDrivers _mainDisplayDrivers_init = {.driverCount = 2};
DisplayDriver _driverArray[2];

MainDisplayDrivers *init_getMainDisplayDrivers() {
    // ---------- Add terminal UART driver ----------
    DisplayDriver *terminalDriver_ptr = getDisplayDriver(TERMINAL_UART);
    if (!terminalDriver_ptr) {
        perror("Terminal driver not found!");
        exit(EXIT_FAILURE);
    }

    // ---------- Add LCD screen driver ----------
    DisplayDriver *lcdDriver_ptr = getDisplayDriver(LCD_SPI);
    if (!lcdDriver_ptr) {
        perror("LCD driver not found!");
        exit(EXIT_FAILURE);
    }

    memcpy(&_driverArray[0], terminalDriver_ptr, sizeof(DisplayDriver));
    memcpy(&_driverArray[1], lcdDriver_ptr, sizeof(DisplayDriver));

    _mainDisplayDrivers_init.displayDriverArray = _driverArray;
    return &_mainDisplayDrivers_init;
}
