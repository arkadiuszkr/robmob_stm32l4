#ifndef FREERTOS_CUSTOM_HEADER
#define FREERTOS_CUSTOM_HEADER

#include "cmsis_os2.h"


#define MAX_UART_DebugMessageLength 100
typedef char UARTMessage[MAX_UART_DebugMessageLength];

extern osMessageQueueId_t Queue_UART_SendDebugHandle;
extern osThreadId_t TaskN_UARTDebugHandle;

#endif
