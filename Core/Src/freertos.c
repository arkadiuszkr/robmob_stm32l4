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
#include "input.h"
#include "main_Robot.h"
#include "thread_settings_stm32l4.h"
#include "lcd_ILI9488.h"
#include "usart.h"
#include <stdio.h>
#include <string.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
typedef StaticSemaphore_t osStaticSemaphoreDef_t;
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
  .stack_size = 1024 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for TaskN_Menu_Repr */
osThreadId_t TaskN_Menu_ReprHandle;
const osThreadAttr_t TaskN_Menu_Repr_attributes = {
  .name = "TaskN_Menu_Repr",
  .stack_size = 1024 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for TaskN_InputComp */
osThreadId_t TaskN_InputCompHandle;
const osThreadAttr_t TaskN_InputComp_attributes = {
  .name = "TaskN_InputComp",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for TaskN_SPI_LCD */
osThreadId_t TaskN_SPI_LCDHandle;
const osThreadAttr_t TaskN_SPI_LCD_attributes = {
  .name = "TaskN_SPI_LCD",
  .stack_size = 1024 * 4,
  .priority = (osPriority_t) osPriorityLow4,
};
/* Definitions for Queue_UART_SendDebug */
osMessageQueueId_t Queue_UART_SendDebugHandle;
const osMessageQueueAttr_t Queue_UART_SendDebug_attributes = {
  .name = "Queue_UART_SendDebug"
};
/* Definitions for Queue_InputAction */
osMessageQueueId_t Queue_InputActionHandle;
const osMessageQueueAttr_t Queue_InputAction_attributes = {
  .name = "Queue_InputAction"
};
/* Definitions for semBin_menuSnapshot */
osSemaphoreId_t semBin_menuSnapshotHandle;
osStaticSemaphoreDef_t semBin_menuSnapshotControlBlock;
const osSemaphoreAttr_t semBin_menuSnapshot_attributes = {
  .name = "semBin_menuSnapshot",
  .cb_mem = &semBin_menuSnapshotControlBlock,
  .cb_size = sizeof(semBin_menuSnapshotControlBlock),
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void Task_UART_SendDebug(void *argument);
void Task_Menu_Reprint(void *argument);
void Task_InputCompute(void *argument);
void Task_TransmitSPI_LCD(void *argument);

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

  /* Create the semaphores(s) */
  /* creation of semBin_menuSnapshot */
  semBin_menuSnapshotHandle = osSemaphoreNew(1, 1, &semBin_menuSnapshot_attributes);

  /* USER CODE BEGIN RTOS_SEMAPHORES */
    /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
    /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of Queue_UART_SendDebug */
  Queue_UART_SendDebugHandle = osMessageQueueNew (16, sizeof(UARTMessage), &Queue_UART_SendDebug_attributes);

  /* creation of Queue_InputAction */
  Queue_InputActionHandle = osMessageQueueNew (16, sizeof(InputAction_Press), &Queue_InputAction_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
    /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of TaskN_UARTDebug */
  TaskN_UARTDebugHandle = osThreadNew(Task_UART_SendDebug, NULL, &TaskN_UARTDebug_attributes);

  /* creation of TaskN_Menu_Repr */
  TaskN_Menu_ReprHandle = osThreadNew(Task_Menu_Reprint, NULL, &TaskN_Menu_Repr_attributes);

  /* creation of TaskN_InputComp */
  TaskN_InputCompHandle = osThreadNew(Task_InputCompute, NULL, &TaskN_InputComp_attributes);

  /* creation of TaskN_SPI_LCD */
  TaskN_SPI_LCDHandle = osThreadNew(Task_TransmitSPI_LCD, NULL, &TaskN_SPI_LCD_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
    /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
    /* add events, ... */

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
            HAL_UART_Transmit(&huart1, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);
        } else {
            const char message[] = "Error \r\n";
            HAL_UART_Transmit(&huart1, (uint8_t *)message, strlen(message), HAL_MAX_DELAY);
        }
    }
  /* USER CODE END Task_UART_SendDebug */
}

/* USER CODE BEGIN Header_Task_Menu_Reprint */
/**
 * @brief Function implementing the TaskN_Menu_Repr thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_Task_Menu_Reprint */
void Task_Menu_Reprint(void *argument)
{
  /* USER CODE BEGIN Task_Menu_Reprint */
    /* Infinite loop */
    for (;;) {
        if (_currentMenu == NULL) {
            // ---------- Screen saver/wallpaper ----------
            // system("clear");
            // printf("Screensaver/wallpaper here\n");
            // fflush(stdout);

            osDelay(SLEEP_MS_MENU_REPRINT);
            continue;
        }
        if (!_currentMenu->reprintRequested) {
            osDelay(SLEEP_MS_MENU_REPRINT);
            continue;
        }

        menu_show_reprint(_mainDisplayDrivers->displayDriverArray, _mainDisplayDrivers->driverCount, _currentMenu);
        // time_t now;
        // time(&now);
        // printf("Reprinting requested, %s\n", ctime(&now));
        _currentMenu->reprintRequested = false;
        HAL_Delay(4000);

        osDelay(SLEEP_MS_MENU_REPRINT);
        continue;
    }
  /* USER CODE END Task_Menu_Reprint */
}

/* USER CODE BEGIN Header_Task_InputCompute */
/**
 * @brief Function implementing the TaskN_InputComp thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_Task_InputCompute */
void Task_InputCompute(void *argument)
{
  /* USER CODE BEGIN Task_InputCompute */
    InputAction_Press pressedAction;
    /* Infinite loop */
    for (;;) {
        osStatus_t status = osMessageQueueGet(Queue_InputActionHandle, &pressedAction, 0, osWaitForever);
        if (status != osOK) {
            continue;
        }
        switch (pressedAction) {
            case InputPress_Down: inputAction_Down(); break;
            case InputPress_Up: inputAction_Up(); break;
            case InputPress_Right: inputAction_Select(); break;
            case InputPress_Left: inputAction_Back(); break;
            case InputPress_Quit: inputAction_Quit(); break;
        }
    }
  /* USER CODE END Task_InputCompute */
}

/* USER CODE BEGIN Header_Task_TransmitSPI_LCD */
/**
 * @brief Function implementing the TaskN_SPI_LCD thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_Task_TransmitSPI_LCD */
void Task_TransmitSPI_LCD(void *argument)
{
  /* USER CODE BEGIN Task_TransmitSPI_LCD */
    /* Infinite loop */
    for (;;) {
        if (!lcd_requestedReprint) {
            osDelay(50);
        }
        _lcd_drawMenuThroughSPI();
    }
  /* USER CODE END Task_TransmitSPI_LCD */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

