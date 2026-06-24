#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cli_menu.h"
#include "main_Robot.h"
#include "start_init_menu.h"
#include "start_init_stm32l4.h"
#include "lcd_ILI9488.h"

// -------------------- Global variables --------------------
MainDisplayDrivers *_mainDisplayDrivers = NULL;
Menu *_mainMenu = NULL;
Menu *_currentMenu = NULL; // Used for navigation in and out the menu tree

MainMode _Main_currentMode = MODE_OFFICIAL;

// -------------------- Forward declarations --------------------
static void _initialize();
static void _open_MainMenu();

// -------------------- Functions --------------------
int main_Robot() {
    _initialize();
    _open_MainMenu(); // Open main menu on startup

    return 0;
}

static void _initialize() {
    _mainDisplayDrivers = init_getMainDisplayDrivers();
    _mainMenu = init_getMainMenu();
}
static void _open_MainMenu() {
    menu_show_reprint(_mainDisplayDrivers->displayDriverArray, _mainDisplayDrivers->driverCount, _mainMenu);
    _currentMenu = _mainMenu;
}
