#ifndef INPUT_H
#define INPUT_H

#define PASSWORDLENGTH_AGMODE 8
#define PASSWORD_AGMODE "DDDDDDDD"

typedef enum { INPUTMODE_NAVIGATION, INPUTMODE_PASSWORD_READ, INPUTMODE_DISABLED } Input_CurrentListeningMode;
typedef enum { InputPress_Down, InputPress_Up, InputPress_Left, InputPress_Right, InputPress_Quit, InputPress_Undefined } InputAction_Press;

void inputAction_Down();
void inputAction_Up();
void inputAction_Back();
void inputAction_Select();
void inputAction_Quit();

void main_chooseMainMode_Official();
void main_chooseMainMode_Hidden();
void main_chooseMainMode_AGFull();

#endif
