#ifndef DISPLAY_DRIVER_H
#define DISPLAY_DRIVER_H

typedef struct {
    void (*print)(const char *str);
    void (*printLine)(const char *str);
    void (*printBoldLine)(const char *str);
    void (*clear)();
} DisplayDriver;

typedef enum {TERMINAL_UART, LCD_SPI} DisplayType; 


// ---------- Functions ----------
DisplayDriver* getDisplayDriver(DisplayType displayType);

#endif
