#include "mainISR.h"
#include "cmsis_os2.h"
#include "freertos_header.h"
#include "input.h"
#include "lcd_ILI9488.h"
#include "spi.h"
#include "stm32l4xx_hal.h"
#include "stm32l4xx_hal_spi.h"
#include "stm32l4xx_hal_uart.h"
#include "usart.h"

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart == &huart1) {
        char pressedCharacter = RxBuffer_USART1;

        InputAction_Press action = InputPress_Undefined;
        switch (pressedCharacter) {
            case 'j': action = InputPress_Down; break;
            case 'k': action = InputPress_Up; break;
            case 'l': action = InputPress_Right; break;
            case 'h': action = InputPress_Left; break;
            case 'q': action = InputPress_Quit; break;
        }
        if (action != InputPress_Undefined) osMessageQueuePut(Queue_InputActionHandle, &action, 0, 0);
        HAL_UART_Receive_IT(&huart1, &RxBuffer_USART1, 1);
    }
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi) {
    if (hspi == &hspi2) {
        DMA_Interupt_SPI2_FullTransfer();
    }
}
