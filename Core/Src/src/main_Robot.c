#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cli_menu.h"
#include "input.h"
#include "main_Robot.h"
#include "start_init_menu.h"
#include "start_init_stm32l4.h"

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

    // set_terminal_nonblocking_mode(true); // For using terminal for input

    // pthread_t input_terminal_tid, reprint_menu_tid;
    // pthread_create(&input_terminal_tid, NULL, input_inputReadThread_terminal, NULL);
    // pthread_create(&reprint_menu_tid, NULL, reprint_menu_cache, NULL);

    // pthread_join(input_terminal_tid, NULL);
    // pthread_join(reprint_menu_tid, NULL);

    // set_terminal_nonblocking_mode(false);

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
