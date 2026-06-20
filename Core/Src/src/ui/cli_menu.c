#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <termios.h>
#include <time.h>
#include <unistd.h>

#include "cli_menu.h"
#include "main_Robot.h"

// -------------------- Menu general functions --------------------

/// @brief Create a new menu instance
/// @return New Menu instance
Menu *menu_init(ItemInMenu *items, size_t itemCount, const char *menuTitle) {
    if (menuTitle == NULL) {
        menuTitle = "";
    }
    // ---------- Initialize new menu ----------
    Menu *newMenu = malloc(sizeof(Menu));
    if (!newMenu) {
        perror("Menu malloc failed!");
        exit(EXIT_FAILURE);
    }

    // ---------- Initialize and copy items ----------
    ItemInMenu *itemsCopy = malloc(itemCount * sizeof(ItemInMenu));
    if (!itemsCopy) {
        perror("Items copy malloc failed!");
        free(newMenu);
        exit(EXIT_FAILURE);
    }
    memcpy(itemsCopy, items, itemCount * sizeof(ItemInMenu));

    *newMenu = (Menu){.menuTitle = menuTitle,
                      .itemsInMenu = itemsCopy,
                      .reprintRequested = false,
                      .itemCount = itemCount,
                      .currentIndex = 0,
                      .previousMenu = NULL};

    return newMenu;
}

bool menu_changeCurrentSelected(Menu *menu, size_t index) {
    if (menu == NULL) return false;
    if (index > menu->itemCount - 1) return false;
    if (index < 0) return false;

    menu->currentIndex = index;
    return true;
}

// -------------------- Menu visual functionality --------------------
void menu_print(Menu *menu, DisplayDriver *displayDriver);

void menu_navigationGo_Up(Menu *menu) {
    if (menu == NULL) return;
    menu->currentIndex--;
    if (menu->currentIndex < 0) {
        menu->currentIndex = 0;
        return;
    }
    menu->reprintRequested = true;
}
void menu_navigationGo_Down(Menu *menu) {
    if (menu == NULL) return;
    menu->currentIndex++;
    if (menu->currentIndex > menu->itemCount - 1) {
        menu->currentIndex = menu->itemCount - 1;
        return;
    }
    menu->reprintRequested = true;
}
Menu *menu_navigationGo_Select(Menu *menu) {
    if (menu == NULL) return menu;
    ItemInMenu currentItem = menu->itemsInMenu[menu->currentIndex];
    switch (currentItem.type) {
        case MENUITEMTYPE_SUBMENU: {
            if (currentItem.dataUnionPtr.submenu_ptr == NULL) return menu;
            currentItem.dataUnionPtr.submenu_ptr->previousMenu = menu;
            currentItem.dataUnionPtr.submenu_ptr->reprintRequested = true;
            return currentItem.dataUnionPtr.submenu_ptr;
        }
        case MENUITEMTYPE_FUNCTION_CALL: {
            if (currentItem.dataUnionPtr.function_void != NULL) currentItem.dataUnionPtr.function_void();
            return menu;
        }
    }
    return menu;
}
Menu *menu_navigationGo_Back(Menu *menu, Menu *fallbackMenu) {
    if (menu == NULL) {
        fallbackMenu->reprintRequested = true;
        return fallbackMenu;
    }
    if (menu == fallbackMenu) {
        fallbackMenu->currentIndex = 0;
        return NULL;
    }
    if (menu->previousMenu == NULL) return menu;

    menu->previousMenu->reprintRequested = true;
    Menu *previousMenuPtr = menu->previousMenu;
    menu->previousMenu = NULL;

    return previousMenuPtr;
}

void menu_print(Menu *menu, DisplayDriver *displayDriver) {
    displayDriver->clear();

    if (!(menu->menuTitle == NULL || strcmp(menu->menuTitle, "") == 0)) {
        displayDriver->print("----------| ");
        displayDriver->print(menu->menuTitle);
        displayDriver->print(" |----------\n");
    }

    for (int i = 0; i < menu->itemCount; i++) {
        if (i == menu->currentIndex) {
            const char *initial = "|>| ";
            char *string = malloc(strlen(initial) + strlen(menu->itemsInMenu[i].displayName) + 1);
            if (string == NULL) {
                perror("Malloc failed");
                displayDriver->clear();
                return;
            }
            strcpy(string, initial);
            strcat(string, menu->itemsInMenu[i].displayName);

            displayDriver->printBoldLine(string);

            free(string);
            string = NULL;
            continue;
        }

        const char *initial = "| | ";
        char *string = malloc(strlen(initial) + strlen(menu->itemsInMenu[i].displayName) + 1);
        if (string == NULL) {
            perror("Malloc failed");
            displayDriver->clear();
            return;
        }
        strcpy(string, initial);
        strcat(string, menu->itemsInMenu[i].displayName);

        displayDriver->printLine(string);

        free(string);
        string = NULL;

        if (displayDriver->menuReadyToPrint != NULL) displayDriver->menuReadyToPrint(menu);
    }
}

void menu_show_reprint(DisplayDriver *driverArray, size_t driverCount, Menu *menu) {
    for (size_t i = 0; i < driverCount; i++) menu_print(menu, &driverArray[i]);
}
void menu_clear(DisplayDriver *driverArray, size_t driverCount) {
    for (size_t i = 0; i < driverCount; i++) driverArray[i].clear();
}

// -------------------- Menu navigation functionality --------------------

// void set_terminal_nonblocking_mode(bool enable) {
//     static struct termios oldt;
//     static bool initialized = false;
//     static int old_flags;
//
//     if (enable) {
//         if (!initialized) {
//             tcgetattr(STDIN_FILENO, &oldt);
//             old_flags = fcntl(STDIN_FILENO, F_GETFL);
//             initialized = true;
//         }
//         struct termios newt = oldt;
//         newt.c_lflag &= ~(ICANON | ECHO);
//         tcsetattr(STDIN_FILENO, TCSANOW, &newt);
//         fcntl(STDIN_FILENO, F_SETFL, old_flags | O_NONBLOCK);
//     } else {
//         if (initialized) {
//             tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
//             fcntl(STDIN_FILENO, F_SETFL, old_flags);
//             initialized = false;
//         }
//     }
// }

// void *reprint_menu_cache(void *args) {
//   while (1) {
//       if (_currentMenu == NULL) {
//           // ---------- Screen saver/wallpaper ----------
//           system("clear");
//           printf("Screensaver/wallpaper here\n");
//           // fflush(stdout);

//           usleep(USLEEP_MENU_REPRINT);
//           continue;
//       }
//       if (!_currentMenu->reprintRequested) {
//           usleep(USLEEP_MENU_REPRINT);
//           continue;
//       }

//       menu_show_reprint(_mainDisplayDrivers->displayDriverArray, _mainDisplayDrivers->driverCount, _currentMenu);
//       // time_t now;
//       // time(&now);
//       // printf("Reprinting requested, %s\n", ctime(&now));
//       _currentMenu->reprintRequested = false;

//       usleep(USLEEP_MENU_REPRINT);
//       continue;
//   }
//   return NULL;
// }
