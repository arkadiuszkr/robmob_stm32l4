#ifndef CUSTOM_MENU_H
#define CUSTOM_MENU_H

#include <stddef.h>
#include <string.h>
#include <stdbool.h>

#include "displayDriver.h"

// -------------------- Menu declarations --------------------
typedef enum {
    MENUITEMTYPE_SUBMENU,
    MENUITEMTYPE_FUNCTION_CALL,
    MENUITEMTYPE_VALUE,
    MENUITEMTYPE_PLACEHOLDER,
    MENUITEMTYPE_UNASSINGED
} MenuItemType;

typedef struct Menu Menu;

typedef struct ItemInMenu{
    const char *displayName;
    MenuItemType type;
    union{
        struct Menu *submenu_ptr;
        int *value_ptr;
        void (*function_void)(void);
    } dataUnionPtr;
} ItemInMenu;

typedef struct Menu{
    const char *menuTitle;
    ItemInMenu *itemsInMenu;
    bool reprintRequested;
    int itemCount;
    int currentIndex;
    Menu* previousMenu;
} Menu;

Menu* menu_init(ItemInMenu *items, size_t itemCount, const char *menuTitle);
void menu_show_reprint(DisplayDriver* driverArray, size_t driverCount, Menu* menu);
void menu_clear(DisplayDriver *driverArray, size_t driverCount);

void menu_navigationGo_Up(Menu* menu); 
void menu_navigationGo_Down(Menu* menu); 
Menu *menu_navigationGo_Select(Menu *menu);
Menu *menu_navigationGo_Back(Menu *menu, Menu *fallbackMenu);


// -------------------- Button/input handling --------------------

// void set_terminal_nonblocking_mode(bool enable); 
void *reprint_menu_cache(void *args);

#endif
