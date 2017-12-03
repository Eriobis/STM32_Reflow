/**********************************************************************************************************************
 * @file    menu.c
 * @author  Simon Benoit
 * @date    02-12-2017
 * @brief   Menu framework for displays
 *********************************************************************************************************************/

#include <stdlib.h>
#include <stdbool.h>
#include "menu.h"

void MENU_MainMenu();
void MENU_SettingMenu();

//          MENU ID,      PARENTID,     MenuFonction        Item List
#define X_MENU_CONFIG   \
    X_MENU( "MAIN",     MAIN_MENU,    NULL,         MENU_MainMenu,      MENU_MainMenuItems      )   \
    X_MENU( "Settings", SETTING_MENU, MAIN_MENU,    MENU_SettingMenu,   MENU_SettingsMenuItems  )

typedef enum __MENU_LIST_e
{
    #define X_MENU(TITLE, ID, PARENT, FNC, ITEMS) ID,
    X_MENU_CONFIG
    #undef X_MENU

    MENU_TOTAL_PAGES,
}MENU_LIST_e;





typedef struct __MENU_Item_t
{
    char* title;
    void*  callbackFnc;
    MENU_LIST_e nextMenu;
}MENU_Item_t;

typedef struct __MENU_page_t
{
    uint8_t pageID;
    uint8_t parentID;
    uint8_t itemNb;
}MENU_page_t;

MENU_LIST_e currentPage;
uint8_t currentPosition;
uint8_t lastPosition;
bool menuNeedRefresh;

/* Includes ---------------------------------------------------------------------------------------------------------*/

#include "ssd1306.h"
#include "Fonts.h"

/* Local Defines ----------------------------------------------------------------------------------------------------*/

/* Local Typedefs ---------------------------------------------------------------------------------------------------*/

/* Forward Declarations ---------------------------------------------------------------------------------------------*/

void MENU_PrintMenu(MENU_LIST_e menu);
void MENU_Goto(MENU_LIST_e menu);

/* Local Constants --------------------------------------------------------------------------------------------------*/

const char cursorSymbol = '>';

// Definition for the 'Main Menu'
const MENU_Item_t MENU_MainMenuItems[] =
{
    { "Settings",   NULL,   SETTING_MENU  },
    { "Start",      NULL,   MAIN_MENU     },
    { "Stop",       NULL,   MAIN_MENU     }
};

// Definition for the 'Settings'
const MENU_Item_t MENU_SettingsMenuItems[] =
{
    { "../Main Menu",  NULL,   MAIN_MENU    },
    { "Preheat time",  NULL,   SETTING_MENU },
    { "Preheat temp",  NULL,   SETTING_MENU },
    { "Soak time",     NULL,   SETTING_MENU },
    { "Soak temp",     NULL,   SETTING_MENU },
    { "Reflow time",   NULL,   SETTING_MENU },
    { "Reflow temp",   NULL,   SETTING_MENU },
    { "Cooling time",  NULL,   SETTING_MENU }
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

/* Local Functions --------------------------------------------------------------------------------------------------*/


void MENU_PrintMenu(MENU_LIST_e menu)
{
    uint8_t yPos = 0;
    uint8_t xPos;

    FontDef Font = Font_7x10;
    xPos = Font.FontWidth + 1;
    MENU_Item_t *itemList = menuItemList[menu];
    ssd1306_SetCursor(xPos,yPos);
    ssd1306_WriteString(menuTitle[menu], Font_7x10, White);
    yPos += Font.FontHeight;
    for ( int x=0; x<menuNbOfItems[menu]; x++)
    {
        ssd1306_SetCursor(xPos,yPos);
        ssd1306_WriteString(itemList[x].title, Font_7x10, White);
        yPos += Font.FontHeight;
    }

    //Print cursor
    ssd1306_SetCursor(0, currentPosition*Font.FontHeight);
    ssd1306_WriteString(">", Font_7x10, White);

    ssd1306_UpdateScreen();
}

void MENU_Goto(MENU_LIST_e menu)
{
    if ( menu != currentPage )
    {
        currentPage = menu;
        menuNeedRefresh = true;
    }
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
    currentPage = MAIN_MENU;
    menuNeedRefresh = true;
}

void MENU_Process()
{

    if ( menuNeedRefresh )
    {
        MENU_PrintMenu(currentPage);
        menuNeedRefresh = false;
    }
}

void MENU_Action(MENU_Action_e action)
{

}
