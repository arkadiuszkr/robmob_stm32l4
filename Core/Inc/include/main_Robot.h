#ifndef MAIN_ROBOT_H
#define MAIN_ROBOT_H

#include "cli_menu.h"
#include "start_init_stm32l4.h"

typedef enum MainMode {MODE_AGFULL, MODE_OFFICIAL, MODE_HIDDEN} MainMode;

extern MainMode _Main_currentMode;
extern Menu *_mainMenu;
extern Menu *_currentMenu;
extern MainDisplayDrivers *_mainDisplayDrivers;

void main_chooseMainMode_Official(); 
void main_chooseMainMode_Hidden(); 
void main_chooseMainMode_AGFull(); 

#endif
