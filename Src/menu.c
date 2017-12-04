/**********************************************************************************************************************
 * @file    menu.c
 * @author  Simon Benoit
 * @date    02-12-2017
 * @brief   Menu framework for displays
 *********************************************************************************************************************/

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "menu.h"
#include "system.h"

//Menu configuration, enter all your pages here
//          MENU TITLE,             MENU ID,      PARENTID,     MenuFonction        Item List
#define X_MENU_CONFIG   \
    X_MENU( "-- Main Menu --",      MAIN_MENU,    NULL,         MENU_MainMenu,      MENU_MainMenuItems      )   \
    X_MENU( "-- Settings --",       SETTING_MENU, MAIN_MENU,    MENU_SettingMenu,   MENU_SettingsMenuItems  )

    #define X_UNIT_TYPE   \
    X_UNIT( "°C",   UNIT_DEG )   \
    X_UNIT( "s",    UNIT_SECONDS )

// Menu ID creation, each page will have its own number from o to 255
typedef enum __MENU_LIST_e
{
    #define X_MENU(TITLE, ID, PARENT, FNC, ITEMS) ID,
    X_MENU_CONFIG
    #undef X_MENU

    MENU_TOTAL_PAGES,
}MENU_LIST_e;


/* Includes ---------------------------------------------------------------------------------------------------------*/

#include "ssd1306.h"
#include "Fonts.h"

/* Local Defines ----------------------------------------------------------------------------------------------------*/

/* Local Typedefs ---------------------------------------------------------------------------------------------------*/

/*
    ITEM_NAVIGATION     - The item is used to change page
    ITEM_EDIT_VARIABLE  - The item is used to modify a variable
*/
typedef enum __MENU_ItemType_e
{
    ITEM_NAVIGATION,
    ITEM_EDIT_VARIABLE,
}MENU_ItemType_e;

typedef enum MENU_ItemUnit_e
{
    UNIT_NO_UNIT,
    #define X_UNIT(SYMBOL, ID) ID,
    X_UNIT_TYPE
    #undef X_UNIT
}MENU_ItemUnit_e;

typedef enum __MENU_CursorMode_e
{
    MODE_NAVIGATE,
    MODE_EDIT,
}MENU_CursorMode_e;

typedef struct __MENU_Item_t
{
    char* title;
    void (*callbackFnc)(uint16_t** val);
    MENU_LIST_e nextMenu;
    MENU_ItemType_e actionType;
    MENU_ItemUnit_e unit;
}MENU_Item_t;

typedef struct __MENU_page_t
{
    uint8_t pageID;
    uint8_t parentID;
    uint8_t itemNb;
}MENU_page_t;

/* Forward Declarations ---------------------------------------------------------------------------------------------*/

static void MENU_PrintMenu          (MENU_LIST_e menu);
static void MENU_Goto               (MENU_LIST_e menu);
static void MENU_MainMenu           (void);
static void MENU_SettingMenu        (void);
static void MENU_EditVariableMenu   (void);
/* Local Constants --------------------------------------------------------------------------------------------------*/

const char cursorSymbol = '>';

// Definition for the 'Main Menu'
const MENU_Item_t MENU_MainMenuItems[] =
{
    { "Settings",   NULL,       SETTING_MENU,   ITEM_NAVIGATION,    UNIT_NO_UNIT },
    { "Start",      SYS_Start,  MAIN_MENU,      ITEM_NAVIGATION,    UNIT_NO_UNIT },
    { "Stop",       SYS_Stop,   MAIN_MENU,      ITEM_NAVIGATION,    UNIT_NO_UNIT }
};

// Definition for the 'Settings'
const MENU_Item_t MENU_SettingsMenuItems[] =
{
    { "../Main Menu",  NULL,                            MAIN_MENU,       ITEM_NAVIGATION,       UNIT_NO_UNIT    },
    { "Preheat time",  (void*)SYS_GetPreHeatTimePtr,    SETTING_MENU,    ITEM_EDIT_VARIABLE,    UNIT_SECONDS    },
    { "Preheat temp",  (void*)SYS_GetPreHeatTempPtr,    SETTING_MENU,    ITEM_EDIT_VARIABLE,    UNIT_DEG        },
    { "Soak time",     (void*)SYS_GetSoakTimePtr,       SETTING_MENU,    ITEM_EDIT_VARIABLE,    UNIT_SECONDS    },
    { "Soak temp",     (void*)SYS_GetSoakTempPtr,       SETTING_MENU,    ITEM_EDIT_VARIABLE,    UNIT_DEG        },
    { "Reflow time",   (void*)SYS_GetReflowTimePtr,     SETTING_MENU,    ITEM_EDIT_VARIABLE,    UNIT_SECONDS    },
    { "Reflow temp",   (void*)SYS_GetReflowTempPtr,     SETTING_MENU,    ITEM_EDIT_VARIABLE,    UNIT_DEG        },
    { "Cooling time",  (void*)SYS_GetCoolingTimePtr,    SETTING_MENU,    ITEM_EDIT_VARIABLE,    UNIT_SECONDS    }
};

// Pages and items automatic creation
const char* unitSymbol[] =
{
    NULL,
    #define X_UNIT(SYMBOL, ID) SYMBOL,
    X_UNIT_TYPE
    #undef X_UNIT
};

const uint8_t menuNbOfItems[] =
{
    #define X_MENU(TITLE, ID, PARENT, FNC, ITEMS) (sizeof(ITEMS) / sizeof(MENU_Item_t)),
    X_MENU_CONFIG
    #undef X_MENU
};

const MENU_Item_t* menuItemList[] =
{
    #define X_MENU(TITLE, ID, PARENT, FNC, ITEMS) (MENU_Item_t*)&ITEMS,
    X_MENU_CONFIG
    #undef X_MENU
};

const void* menuCallbackFnc[] =
{
    #define X_MENU(TITLE, ID, PARENT, FNC, ITEMS) FNC,
    X_MENU_CONFIG
    #undef X_MENU
};

char* menuTitle[] =
{
    #define X_MENU(TITLE, ID, PARENT, FNC, ITEMS) TITLE,
    X_MENU_CONFIG
    #undef X_MENU
};

/* Local Variables --------------------------------------------------------------------------------------------------*/

static MENU_CursorMode_e cursorMode;
static MENU_LIST_e currentPage;
static uint8_t currentPosition;
static uint8_t lastPosition;
static bool menuNeedRefresh;
static bool cursorPosChanged;
static uint16_t *editVariablePtr;

/* Local Functions --------------------------------------------------------------------------------------------------*/


void MENU_PrintMenu(MENU_LIST_e page)
{
    uint8_t yPos = 0;
    uint8_t xPos;

    FontDef Font = Font_7x10;
    xPos = Font.FontWidth + 1;
    MENU_Item_t *itemList = (MENU_Item_t *)menuItemList[page];
    //Clear screen
    ssd1306_Fill(Black);
    //Write Page Title
    xPos = (SSD1306_WIDTH/2) - (strlen(menuTitle[page])/2)*Font.FontWidth; //Center the string
    ssd1306_SetCursor(xPos,yPos);
    ssd1306_WriteString(menuTitle[page], Font_7x10, White);
    yPos += Font.FontHeight;

    if ( cursorMode == MODE_NAVIGATE )
    {
        for ( int x=0; x<menuNbOfItems[page]; x++)
        {
            ssd1306_SetCursor(xPos,yPos);
            ssd1306_WriteString(itemList[x].title, Font_7x10, White);
            yPos += Font.FontHeight;
        }

        //Print cursor
        ssd1306_SetCursor(0, (lastPosition+1)*Font.FontHeight);
        ssd1306_WriteString(" ", Font_7x10, White);
        ssd1306_SetCursor(0, (currentPosition+1)*Font.FontHeight);
        ssd1306_WriteString(">", Font_7x10, White);
    }
    else if ( cursorMode == MODE_EDIT )
    {
        MENU_EditVariableMenu();
    }
    ssd1306_UpdateScreen();
}

void MENU_Goto(MENU_LIST_e nextPage)
{
    currentPage = nextPage;
    menuNeedRefresh = true;
}

void MENU_EditVariableMenu()
{
    MENU_Item_t *itemList = (MENU_Item_t *)menuItemList[currentPage];
    MENU_Item_t *item = (MENU_Item_t *)&itemList[currentPosition];
    char valueStr[16];
    uint8_t xPos;
    sprintf(valueStr,"%u",*editVariablePtr);
    //Draw the frame overlay
    ssd1306_DrawRect(5, 11, SSD1306_WIDTH-10, 50, White);
    xPos = (SSD1306_WIDTH/2) - (strlen(item->title)/2)*Font_7x10.FontWidth; //Center the string
    ssd1306_SetCursor(xPos, 19);
    ssd1306_WriteString(item->title, Font_7x10, White);

    strcat(valueStr,unitSymbol[item->unit]);
    xPos = (SSD1306_WIDTH/2) - (strlen(valueStr)/2)*Font_11x18.FontWidth; //Center the string
    ssd1306_SetCursor(xPos, 40);
    ssd1306_WriteString(valueStr, Font_11x18, White);
}

void MENU_MainMenu()
{

}

void MENU_SettingMenu()
{


}

/* Global Functions -------------------------------------------------------------------------------------------------*/

void MENU_Init()
{
    //Init the oled module
    ssd1306_Init();
    HAL_Delay(250);

    currentPage = MAIN_MENU;
    menuNeedRefresh = true;
    cursorPosChanged = false;
    cursorMode = MODE_NAVIGATE;
    editVariablePtr = NULL;
}

void MENU_Process()
{

    if ( menuNeedRefresh || cursorPosChanged)
    {
        MENU_PrintMenu(currentPage);
        menuNeedRefresh = false;
        cursorPosChanged = false;
    }
}

void MENU_Action(MENU_Action_e action)
{
    MENU_Item_t *itemList = (MENU_Item_t *)menuItemList[currentPage];
    MENU_Item_t *item = (MENU_Item_t *)&itemList[currentPosition];
    if ( cursorMode == MODE_NAVIGATE)
    {
        switch (action)
        {
            case  ACTION_UP:
                if (currentPosition > 1)
                {
                    lastPosition = currentPosition;
                    currentPosition --;
                    cursorPosChanged = true;
                }
                break;
            case  ACTION_DOWN:
                if (currentPosition < menuNbOfItems[currentPage])
                {
                    lastPosition = currentPosition;
                    currentPosition ++;
                    cursorPosChanged = true;
                }
                break;
            case  ACTION_CLICK:
                switch (item->actionType)
                {
                    case  ITEM_NAVIGATION:
                    MENU_Goto(item->nextMenu);
                    break;
                    case  ITEM_EDIT_VARIABLE:
                    if ( item->callbackFnc != NULL )
                    {
                        item->callbackFnc(&editVariablePtr);
                        cursorMode = MODE_EDIT;
                        menuNeedRefresh = true;
                    }
                    break;
                    default:
                    break;
                }
                break;
            default:
            break;
        }
    }
    else if ( cursorMode == MODE_EDIT )
    {
        if ( editVariablePtr != NULL )
        {
            switch (action)
            {
                case  ACTION_UP:
                    if (*editVariablePtr < 65530)
                    {
                        *editVariablePtr += 10;
                    }
                    break;
                case  ACTION_DOWN:
                    if (*editVariablePtr > 10)
                    {
                        *editVariablePtr -= 10;
                    }
                    break;
                case  ACTION_CLICK:
                    //Items are 1 layer deep, after editing go back to navigation
                    cursorMode = MODE_NAVIGATE;
                    // Release the variable
                    editVariablePtr = NULL;
                    MENU_Goto(item->nextMenu);
                    break;
                default:
                    break;
            }
        }
    }
}
