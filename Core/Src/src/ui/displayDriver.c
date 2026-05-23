#include <stdio.h>
#include <stdlib.h>

#include <displayDriver.h>
#include "FreeRTOS.h"

void terminalUART_clear() {
    system("clear");
}

void terminalUART_print(const char* str) { printf("%s", str); }
void terminalUART_printLine(const char* str) { printf("%s\n", str); }
void terminalUART_printBoldLine(const char* str) { printf("\033[1m%s\033[0m\n", str); }


DisplayDriver TERMINAL_DRIVER = {
    .print = terminalUART_print,
    .printLine = terminalUART_printLine,
    .printBoldLine = terminalUART_printBoldLine,
    .clear = terminalUART_clear,
};

DisplayDriver* getDisplayDriver(DisplayType displayType)
{
    if (displayType == TERMINAL_UART) return &TERMINAL_DRIVER;
    return NULL;
}
