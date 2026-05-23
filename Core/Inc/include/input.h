#ifndef INPUT_H
#define INPUT_H

#define PASSWORDLENGTH_AGMODE 8
#define PASSWORD_AGMODE "DDDDDDDD"

typedef enum {INPUTMODE_NAVIGATION, INPUTMODE_PASSWORD_READ, INPUTMODE_DISABLED} Input_CurrentListeningMode;

void *input_inputReadThread_terminal(void *args);

void main_chooseMainMode_Official(); 
void main_chooseMainMode_Hidden(); 
void main_chooseMainMode_AGFull(); 

#endif
