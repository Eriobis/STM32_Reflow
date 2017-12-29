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
    X_MENU( "",                     NO_MENU,      NULL,         MENU_MainMenu,      MENU_NoItems            )   \
    X_MENU( "-- Main Menu --",      MAIN_MENU,    NULL,         MENU_MainMenu,      MENU_MainMenuItems      )   \
    X_MENU( "-- Settings --",       SETTING_MENU, MAIN_MENU,    MENU_SettingMenu,   MENU_SettingsMenuItems  )   \
    X_MENU( "-- Info --",           INFO_MENU,    MAIN_MENU,    MENU_InfoMenu,      MENU_InfoItems          )   \
    X_MENU( "",                     RUN_PAGE,     MAIN_MENU,    MENU_RunMenu,       MENU_NoItems            )   \

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

#define MAX_ITEM_PER_PAGE   5   // 5 Items with a font of 10 pixel high ( + 10 pixels for title )
#define MENU_FONT           Font_7x10
#define GRAPH_MAX_HEIGHT    60  //Nb of pixels the highest point on the grapĥ should be
#define REFRESH_PERIOD      100

/* Local Typedefs ---------------------------------------------------------------------------------------------------*/

/*
    ITEM_NAVIGATION     - The item is used to change page, its callback is run first
    ITEM_EDIT_VARIABLE  - The item is used to modify a variable
    ITEM_ACTION         - Launch the item callback only
    */
typedef enum __MENU_ItemType_e
{
    ITEM_NAVIGATION,
    ITEM_EDIT_VARIABLE,
    ITEM_ACTION,
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
    void (*callbackFnc)(void* arg);
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
static void MENU_InfoMenu           (void);
static void MENU_SettingMenu        (void);
static void MENU_EditVariableMenu   (void);
static void MENU_PrintGraph         (void);
static void MENU_RunMenu            (void);
static void MENU_PrintTempLine      (uint16_t x, uint16_t y);

/* Local Constants --------------------------------------------------------------------------------------------------*/

const char cursorSymbol = '>';

// Definition for the 'Main Menu'
const MENU_Item_t MENU_MainMenuItems[] =
{
    { "Edit Settings",      NULL,               SETTING_MENU,   ITEM_NAVIGATION,    UNIT_NO_UNIT },
    { "Start",              (void*)SYS_Start,   RUN_PAGE,       ITEM_NAVIGATION,    UNIT_NO_UNIT },
    { "Stop",               (void*)SYS_Stop,    MAIN_MENU,      ITEM_ACTION,        UNIT_NO_UNIT }
};

// Definition for the 'Settings'
const MENU_Item_t MENU_SettingsMenuItems[] =
{
    { "..Back",        NULL,                            MAIN_MENU,       ITEM_NAVIGATION,       UNIT_NO_UNIT    },
    { "View Settings", NULL,                            SETTING_MENU,    ITEM_NAVIGATION,       UNIT_NO_UNIT    },
    { "Preheat time",  (void*)SYS_GetPreHeatTimePtr,    SETTING_MENU,    ITEM_EDIT_VARIABLE,    UNIT_SECONDS    },
    { "Preheat temp",  (void*)SYS_GetPreHeatTempPtr,    SETTING_MENU,    ITEM_EDIT_VARIABLE,    UNIT_DEG        },
    { "Soak time",     (void*)SYS_GetSoakTimePtr,       SETTING_MENU,    ITEM_EDIT_VARIABLE,    UNIT_SECONDS    },
    { "Soak temp",     (void*)SYS_GetSoakTempPtr,       SETTING_MENU,    ITEM_EDIT_VARIABLE,    UNIT_DEG        },
    { "Reflow time",   (void*)SYS_GetReflowTimePtr,     SETTING_MENU,    ITEM_EDIT_VARIABLE,    UNIT_SECONDS    },
    { "Reflow temp",   (void*)SYS_GetReflowTempPtr,     SETTING_MENU,    ITEM_EDIT_VARIABLE,    UNIT_DEG        },
    { "Cooling time",  (void*)SYS_GetCoolingTimePtr,    SETTING_MENU,    ITEM_EDIT_VARIABLE,    UNIT_SECONDS    }
};

const MENU_Item_t MENU_InfoItems[] =
{
    { "..Back",        NULL,                            MAIN_MENU,       ITEM_NAVIGATION,       UNIT_NO_UNIT    },
};

const MENU_Item_t MENU_NoItems[] =
{
    /*No items but go back on click*/
    { "",              NULL,                            MAIN_MENU,       ITEM_NAVIGATION,       UNIT_NO_UNIT    },
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

void* (*menuCallbackFnc[])() =
{
    #define X_MENU(TITLE, ID, PARENT, FNC, ITEMS) (void*)(*FNC),
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
static MENU_LIST_e lastPage;
static uint8_t currentPosition;
static uint8_t lastPosition;
static bool menuNeedRefresh;
static bool cursorPosChanged;
static uint16_t *editVariablePtr;

/* Local Functions --------------------------------------------------------------------------------------------------*/

void MENU_PrintMenu(MENU_LIST_e page)
{
    static uint8_t firstItem = 0;
    static uint8_t lastItem = MAX_ITEM_PER_PAGE;
    static MENU_LIST_e lastPrintedPage = NO_MENU;
    uint8_t yPos = 0;
    uint8_t xPos;

    xPos = MENU_FONT.FontWidth + 1;
    MENU_Item_t *itemList = (MENU_Item_t *)menuItemList[page];
    //Clear screen
    ssd1306_Fill(Black);
    //Write Page Title
    xPos = (SSD1306_WIDTH/2) - (strlen(menuTitle[page])/2)*MENU_FONT.FontWidth; //Center the string
    ssd1306_SetCursor(xPos,yPos);
    ssd1306_WriteString(menuTitle[page], MENU_FONT, White);

    yPos += MENU_FONT.FontHeight;
    if ( page != lastPrintedPage)
    {
        lastPrintedPage = page;
        firstItem = 0;
        lastItem = menuNbOfItems[page] > MAX_ITEM_PER_PAGE ? MAX_ITEM_PER_PAGE : menuNbOfItems[page];

    }
    if ( cursorMode == MODE_NAVIGATE )
    {
        // Roam into the 1st page
        if ( currentPosition < lastItem && currentPosition >= firstItem )
        {
            ssd1306_SetCursor(0, (lastPosition + 1 - firstItem)*MENU_FONT.FontHeight);
            ssd1306_WriteString(" ", MENU_FONT, White);
            ssd1306_SetCursor(0, (currentPosition + 1 - firstItem)*MENU_FONT.FontHeight);
            ssd1306_WriteString(">", MENU_FONT, White);
        }
        // Scroll Down
        else if (currentPosition >= lastItem && currentPosition > firstItem)
        {
            firstItem = currentPosition - (MAX_ITEM_PER_PAGE - 1);
            lastItem = currentPosition + 1;
            ssd1306_SetCursor(0, MAX_ITEM_PER_PAGE*MENU_FONT.FontHeight);
            ssd1306_WriteString(">", MENU_FONT, White);
        }
        // Scroll Up
        else if (currentPosition < lastItem && currentPosition <= firstItem)
        {
            firstItem = currentPosition;
            lastItem = (currentPosition + MAX_ITEM_PER_PAGE + 1) > menuNbOfItems[page] ? menuNbOfItems[page] : firstItem + MAX_ITEM_PER_PAGE;
            ssd1306_SetCursor(0, MENU_FONT.FontHeight);
            ssd1306_WriteString(">", MENU_FONT, White);
        }

        //Print the items
        xPos = MENU_FONT.FontWidth + 1; //Leave a small space between the cursor
        for ( int x=firstItem; x<lastItem; x++)
        {
            ssd1306_SetCursor(xPos,yPos);
            ssd1306_WriteString(itemList[x].title, MENU_FONT, White);
            yPos += MENU_FONT.FontHeight;
        }

    }
    else if ( cursorMode == MODE_EDIT )
    {
        MENU_EditVariableMenu();
    }
    //ssd1306_UpdateScreen();
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
    sprintf(valueStr,"% 4u",*editVariablePtr);
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
    static char TempInfoStr1[16];
    uint16_t temp;
    uint8_t xPos;
    char valueStr[16];
    SYS_Profile_e *profile;

    profile = SYS_GetProfile();
    //Print the current temp
    temp = (uint16_t)SYS_GetActualTemp();
    sprintf(TempInfoStr1, "T=% 3u*C/% 3u*C", temp, profile->SetpointArray[profile->SetpointIndex]);
    xPos = (SSD1306_WIDTH/2) - (strlen(TempInfoStr1)/2)*Font_7x10.FontWidth; //Center the string
    ssd1306_SetCursor(xPos, 4*Font_7x10.FontHeight);
    ssd1306_WriteString(TempInfoStr1, Font_7x10, White);

    //Print the system status on the last row
    if ( SYS_IsSystemStarted() )
    {
        sprintf(valueStr,"- RUNNING -");
    }
    else
    {
        sprintf(valueStr,"- STOPPED -");
    }
    xPos = (SSD1306_WIDTH/2) - (strlen(valueStr)/2)*Font_7x10.FontWidth; //Center the string
    ssd1306_SetCursor(xPos, 5*Font_7x10.FontHeight);
    ssd1306_WriteString(valueStr, Font_7x10, White);
    //ssd1306_UpdateScreen();
}

void MENU_SettingMenu()
{


}

void MENU_RunMenu()
{
    static char TempInfoStr1[32];
    static char TempInfoStr2[32];
    SYS_Profile_e *profile;
    float ratio;
    uint16_t temp;

    profile = SYS_GetProfile();
    MENU_PrintDots(profile->SetpointArray, NB_OF_TEMP_POINTS);
    ratio = (float)GRAPH_MAX_HEIGHT/(float)profile->ReflowTemp;
    for(int x=0; x<=profile->SetpointIndex; x++)
    {
        MENU_PrintTempLine(x, (uint16_t)((float)profile->SetpointArray[x]*(float)ratio));
    }

    temp = (uint16_t)SYS_GetActualTemp();
    sprintf(TempInfoStr1, "T =% 3u*C", temp);
    ssd1306_SetCursor(0, 0);
    ssd1306_WriteString(TempInfoStr1, Font_7x10, White);

    sprintf(TempInfoStr2, "S =% 3u*C", profile->SetpointArray[profile->SetpointIndex]);
    ssd1306_SetCursor(0, 10);
    ssd1306_WriteString(TempInfoStr2, Font_7x10, White);

    //ssd1306_UpdateScreen();
}

void MENU_InfoMenu()
{
    MENU_PrintGraph();

}

static void MENU_PrintGraph()
{
    uint8_t x, x0;
    uint8_t y, y0;

    uint16_t highTemp;
    uint16_t totalTime;
    uint16_t temp, t;
    float ratio;
    float graphWidthRatio;
    float graphHeightRatio;
    totalTime = SYS_GetTotalTime();
    highTemp = SYS_GetReflowTemp();
    graphWidthRatio = (float)SSD1306_WIDTH/(float)totalTime;
    graphHeightRatio = (float)45/(float)highTemp;

    //Preheat slope
    t = SYS_GetPreHeatTime();
    ratio = (float)t/(float)totalTime;
    x = ratio * totalTime * graphWidthRatio;
    temp = SYS_GetPreHeatTemp();
    ratio = (float)temp/(float)highTemp;
    y = ratio*highTemp*graphHeightRatio;
    ssd1306_DrawLine(0,SSD1306_HEIGHT-0,x,SSD1306_HEIGHT-y,White);
    ssd1306_DrawLine(x,SSD1306_HEIGHT,x,SSD1306_HEIGHT-y,White);

    //Soak slope
    x0 = x;
    y0 = y;
    t = SYS_GetSoakTime();
    ratio = (float)t/(float)totalTime;
    x = (ratio * totalTime*graphWidthRatio) + x0;
    temp = SYS_GetSoakTemp();
    ratio = (float)temp/(float)highTemp;
    y = (ratio*highTemp*graphHeightRatio);
    ssd1306_DrawLine(x0,SSD1306_HEIGHT-y0,x,SSD1306_HEIGHT-y,White);
    ssd1306_DrawLine(x,SSD1306_HEIGHT,x,SSD1306_HEIGHT-y,White);

    //Reflow slope
    x0 = x;
    y0 = y;
    t = SYS_GetReflowTime();
    ratio = (float)t/(float)totalTime/2.0;
    x = ((ratio * totalTime)*graphWidthRatio) + x0;
    temp = SYS_GetReflowTemp();
    ratio = (float)temp/(float)highTemp;
    y = ratio*highTemp*graphHeightRatio;
    ssd1306_DrawLine(x0,SSD1306_HEIGHT-y0,x,SSD1306_HEIGHT-y,White);

    ratio = (float)t/(float)totalTime/2.0;
    x0 = (ratio * totalTime*graphWidthRatio) + x;
    ratio = (float)temp/(float)highTemp;
    y = ratio*highTemp*graphHeightRatio;
    ssd1306_DrawLine(x,SSD1306_HEIGHT-y,x0,SSD1306_HEIGHT-y0,White);
    ssd1306_DrawLine(x0,SSD1306_HEIGHT,x0,SSD1306_HEIGHT-y0,White);

    //Cool slope
    x = totalTime * graphWidthRatio;
    ssd1306_DrawLine(x0,SSD1306_HEIGHT-y0,x,SSD1306_HEIGHT,White);


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
    static uint32_t refreshTimer;

    if ( HAL_GetTick() - refreshTimer > REFRESH_PERIOD )
    {
        menuNeedRefresh = true;
        refreshTimer = HAL_GetTick();
    }

    if ( menuNeedRefresh || cursorPosChanged )
    {
        if ( currentPage != lastPage || cursorPosChanged )
        {
            MENU_PrintMenu(currentPage);
            lastPage = currentPage;
        }
        menuCallbackFnc[currentPage]();
        menuNeedRefresh = false;
        cursorPosChanged = false;
        ssd1306_UpdateScreen();
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
                if (currentPosition + 1 < menuNbOfItems[currentPage])
                {
                    lastPosition = currentPosition;
                    currentPosition ++;
                    cursorPosChanged = true;
                }
                break;
            case  ACTION_DOWN:
                if (currentPosition > 0)
                {
                    lastPosition = currentPosition;
                    currentPosition --;
                    cursorPosChanged = true;
                }
                break;
            case  ACTION_CLICK:
                switch (item->actionType)
                {
                    case  ITEM_NAVIGATION:
                        lastPosition = currentPosition;
                        currentPosition = 0;
                        MENU_Goto(item->nextMenu);
                        if (item->callbackFnc != NULL)
                        {
                            item->callbackFnc(NULL);
                        }
                    break;

                    case  ITEM_EDIT_VARIABLE:
                    if ( item->callbackFnc != NULL )
                    {
                        // The callback fnc must set the address of the variable to be modified
                        item->callbackFnc((void *)&editVariablePtr);
                        cursorMode = MODE_EDIT;
                        menuNeedRefresh = true;
                        cursorPosChanged = true;
                    }
                    break;

                    case ITEM_ACTION :
                        item->callbackFnc(NULL);
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
                        menuNeedRefresh = true;
                        cursorPosChanged = true;
                    }
                    break;
                case  ACTION_DOWN:
                    if (*editVariablePtr > 10)
                    {
                        *editVariablePtr -= 10;
                        menuNeedRefresh = true;
                        cursorPosChanged = true;
                    }
                    break;
                case  ACTION_CLICK:
                    //Items are 1 layer deep, after editing go back to navigation
                    cursorMode = MODE_NAVIGATE;
                    // Release the variable
                    editVariablePtr = NULL;
                    MENU_Goto(item->nextMenu);
                    cursorPosChanged = true;
                    break;
                default:
                    break;
            }
        }
    }
}

void MENU_PrintDots(uint16_t *data, uint8_t size)
{
    float ratio;
    //Clear screen
    ratio = 64.0/255.0;
    for(int x=0; x<SSD1306_WIDTH; x++)
    {
        ssd1306_DrawPixel(x, 64-(data[x]*ratio), White);
    }
}

void MENU_PrintTempLine(uint16_t x, uint16_t y)
{
    ssd1306_DrawLine(x, 64, x, 64-y, White);
}

void MENU_RefreshMenu()
{
    menuNeedRefresh = true;
}
