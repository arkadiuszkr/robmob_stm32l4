#include <stdlib.h>

#include "cli_menu.h"
#include "main_Robot.h"
#include "start_init_menu.h"

// ---------- Constant strings ----------
const char* const MENU_NAME_CONTROLLOOP_START = "Start the machine";
const char *const MENU_NAME_CONTROLLOOP_STOP = "Stop the machine";


Menu* _mainMenu_init = NULL;


Menu* init_getMainMenu()
{
    if (_mainMenu_init != NULL) return _mainMenu_init;

    ItemInMenu items2[] = {
        {"AG Full", MENUITEMTYPE_FUNCTION_CALL, .dataUnionPtr.function_void = main_chooseMainMode_AGFull}, // Password protected
        {"Oficial", MENUITEMTYPE_FUNCTION_CALL, .dataUnionPtr.function_void = main_chooseMainMode_Official}, // For engineering presentation
        {"Hidden", MENUITEMTYPE_FUNCTION_CALL, .dataUnionPtr.function_void = main_chooseMainMode_Hidden}, // Minimal function
    };

    Menu *secondMenu = menu_init(items2, 3, NULL);

    ItemInMenu items[] = {
        {MENU_NAME_CONTROLLOOP_START, MENUITEMTYPE_UNASSINGED, .dataUnionPtr.function_void = NULL},
        {"Choose behaviour mode", MENUITEMTYPE_SUBMENU, .dataUnionPtr.submenu_ptr = secondMenu},
        {"Log sensor data", MENUITEMTYPE_UNASSINGED, .dataUnionPtr.function_void = NULL},
    };
    _mainMenu_init = menu_init(items, 3, NULL);

    return _mainMenu_init;
}
