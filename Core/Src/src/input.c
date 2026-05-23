#include "input.h"
#include "cli_menu.h"
#include "main_Robot.h"
#include "thread_settings_rpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ---------- Password entering / navigation mode ----------
char _password_AGMode_buffer[PASSWORDLENGTH_AGMODE + 1] = {0};
int _password_AGMode_index = 0;
Input_CurrentListeningMode _input_currentListeningMode = INPUTMODE_NAVIGATION;

// ---------- Forward declarations ----------
void mainMode_reapplyChanges();
void password_CheckPasswordAndEnableMode();
void resetPasswordBufferAndReturn_WithDelay(const char *message, bool withDelay);

// ---------- Input actions ----------
void inputAction_PasswordReadFunction(char key) {
    if (_password_AGMode_index > PASSWORDLENGTH_AGMODE - 1) {
        // Treat as wrong password
        resetPasswordBufferAndReturn_WithDelay("Incorrect password!", true);
        _currentMenu->reprintRequested = true;
        return;
    }
    _password_AGMode_buffer[_password_AGMode_index] = key;
    printf("*");
    fflush(stdout);
    _password_AGMode_index++;
    if (_password_AGMode_index > PASSWORDLENGTH_AGMODE - 1) {
        password_CheckPasswordAndEnableMode();
    }
}
void inputAction_Down() {
    switch (_input_currentListeningMode) {
        case INPUTMODE_NAVIGATION: menu_navigationGo_Down(_currentMenu); break;
        case INPUTMODE_PASSWORD_READ: inputAction_PasswordReadFunction('D'); break;
        case INPUTMODE_DISABLED: return;
    }
}
void inputAction_Up() {
    switch (_input_currentListeningMode) {
        case INPUTMODE_NAVIGATION: menu_navigationGo_Up(_currentMenu); break;
        case INPUTMODE_PASSWORD_READ: inputAction_PasswordReadFunction('U'); break;
        case INPUTMODE_DISABLED: return;
    }
}
void inputAction_Back() { _currentMenu = menu_navigationGo_Back(_currentMenu, _mainMenu); }
void inputAction_Select() { _currentMenu = menu_navigationGo_Select(_currentMenu); }
void inputAction_Quit() {
    _currentMenu = NULL;
    _mainMenu->currentIndex = 0;
    resetPasswordBufferAndReturn_WithDelay(NULL, false);
    _input_currentListeningMode = INPUTMODE_NAVIGATION;
}
void password_CheckPasswordAndEnableMode() {
    if (strcmp(_password_AGMode_buffer, PASSWORD_AGMODE) != 0) return;
    resetPasswordBufferAndReturn_WithDelay("Applying chosen mode!", true);
    _Main_currentMode = MODE_AGFULL;
    mainMode_reapplyChanges();
}
void resetPasswordBufferAndReturn_WithDelay(const char *message, bool withDelay) {
    _input_currentListeningMode = INPUTMODE_DISABLED;
    // Reset password buffer
    _password_AGMode_index = 0;
    memset(_password_AGMode_buffer, 0, PASSWORDLENGTH_AGMODE + 1);
    if (message != NULL) {
        system("clear");
        printf("%s\n", message);
        fflush(stdout);
    }

    // if (withDelay) usleep(1000 * 1000);
    // Return to menu mode
    _input_currentListeningMode = INPUTMODE_NAVIGATION;
}

void *input_inputReadThread_terminal(void *args) {
    char pressedCharacter;
    while (1) {
        if (read(STDIN_FILENO, &pressedCharacter, 1) < 0) {
            // usleep(USLEEP_BUTTONINPUT_TERMINAL);
            continue;
        }
        switch (pressedCharacter) {
            case 'j': inputAction_Down(); break;
            case 'k': inputAction_Up(); break;
            case 'l': inputAction_Select(); break;
            case 'h': inputAction_Back(); break;
            case 'q': inputAction_Quit(); break;
        }

        // usleep(USLEEP_BUTTONINPUT_TERMINAL);
    }
    return NULL;
}

void mainMode_reapplyChanges() { _currentMenu->reprintRequested = true; }
void main_chooseMainMode_Official() {
    _Main_currentMode = MODE_OFFICIAL;
    mainMode_reapplyChanges();
}
void main_chooseMainMode_Hidden() {
    _Main_currentMode = MODE_HIDDEN;
    mainMode_reapplyChanges();
}
void main_chooseMainMode_AGFull() {
    // ---------- Get password for password protected mode ----------
    resetPasswordBufferAndReturn_WithDelay(NULL, false);
    _input_currentListeningMode = INPUTMODE_PASSWORD_READ;
    menu_clear(_mainDisplayDrivers->displayDriverArray, _mainDisplayDrivers->driverCount);
}
