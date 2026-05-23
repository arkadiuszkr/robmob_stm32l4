#include <stdio.h>
#include <stdlib.h>

#include "cmsis_os.h"
#include "freertos_header.h"
#include <displayDriver.h>

const char *clear = "\033[2J\033[H";
void terminalUART_clear() {
    UARTMessage msg;
    snprintf(msg, MAX_UART_DebugMessageLength, "%s", clear);
    osMessageQueuePut(Queue_UART_SendDebugHandle, &msg, 0, 0);
}

void terminalUART_print(const char *str) {
    UARTMessage msg;
    snprintf(msg, MAX_UART_DebugMessageLength, "%s", str);
    osMessageQueuePut(Queue_UART_SendDebugHandle, &msg, 0, 0);
}
void terminalUART_printLine(const char *str) {
    UARTMessage msg;
    snprintf(msg, MAX_UART_DebugMessageLength, "%s\r\n", str);
    osMessageQueuePut(Queue_UART_SendDebugHandle, &msg, 0, 0);
}
void terminalUART_printBoldLine(const char *str) {
    UARTMessage msg;
    snprintf(msg, MAX_UART_DebugMessageLength, "\033[1m%s\033[0m\r\n", str);
    osMessageQueuePut(Queue_UART_SendDebugHandle, &msg, 0, 0);
}

DisplayDriver TERMINAL_UART_DRIVER = {
    .print = terminalUART_print,
    .printLine = terminalUART_printLine,
    .printBoldLine = terminalUART_printBoldLine,
    .clear = terminalUART_clear,
};

DisplayDriver *getDisplayDriver(DisplayType displayType) {
    if (displayType == TERMINAL_UART) return &TERMINAL_UART_DRIVER;
    return NULL;
}
