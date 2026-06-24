#ifndef LCD_ILI9488
#define LCD_ILI9488

#include <stdbool.h>

void _lcd_initialize();
void _lcd_drawTestScreen();
void _lcd_clear();
void _lcd_addToSnapshot_print();
void _lcd_addToSnapshot_printLine(const char *str);
void _lcd_addToSnapshot_printBold(const char *str);
void _lcd_drawMenu();
void _lcd_drawMenuThroughSPI();
void DMA_Interupt_SPI2_FullTransfer();

extern bool lcd_requestedReprint;

#endif
