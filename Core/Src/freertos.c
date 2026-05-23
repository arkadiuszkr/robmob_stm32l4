/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * File Name          : freertos.c
 * Description        : Code for freertos applications
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2026 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "freertos_header.h"
#include "usart.h"
#include <string.h>
#include <stdio.h>
#include "main_Robot.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for TaskN_UARTDebug */
osThreadId_t TaskN_UARTDebugHandle;
const osThreadAttr_t TaskN_UARTDebug_attributes = {
  .name = "TaskN_UARTDebug",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for Queue_UART_SendDebug */
osMessageQueueId_t Queue_UART_SendDebugHandle;
const osMessageQueueAttr_t Queue_UART_SendDebug_attributes = {
  .name = "Queue_UART_SendDebug"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void Task_UART_SendDebug(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
    /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
    /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
    /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of Queue_UART_SendDebug */
  Queue_UART_SendDebugHandle = osMessageQueueNew (16, sizeof(UARTMessage), &Queue_UART_SendDebug_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
    /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of TaskN_UARTDebug */
  TaskN_UARTDebugHandle = osThreadNew(Task_UART_SendDebug, NULL, &TaskN_UARTDebug_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
    /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
    /* add events, ... */
  const char message[] = "Test test\r\n";
  UARTMessage msg;
  snprintf(msg, MAX_UART_DebugMessageLength, "%s", message);
  osMessageQueuePut(Queue_UART_SendDebugHandle, (void*)msg, 0, 1000);

  main_Robot();
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_Task_UART_SendDebug */
/**
 * @brief  Function implementing the TaskN_UARTDebug thread.
 * @param  argument: Not used
 * @retval None
 */
/* USER CODE END Header_Task_UART_SendDebug */
void Task_UART_SendDebug(void *argument)
{
  /* USER CODE BEGIN Task_UART_SendDebug */

    UARTMessage msg;
    /* Infinite loop */
    for (;;) {
        osStatus_t status = osMessageQueueGet(Queue_UART_SendDebugHandle, msg, 0, osWaitForever);
        if (status == osOK) {
            size_t size = strlen(msg);
            HAL_UART_Transmit(&huart2, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);
        } else {
            const char message[] = "Error \r\n";
            HAL_UART_Transmit(&huart2, (uint8_t *)message, strlen(message), HAL_MAX_DELAY);
        }
    }
  /* USER CODE END Task_UART_SendDebug */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

