/**********************************************************************************************************************
 * @file    menu.h
 * @author  Simon Benoit
 * @date    02-12-2017
 * @brief
 *********************************************************************************************************************/

 #ifndef __MENU_H__
 #define __MENU_H__

 /* Includes ---------------------------------------------------------------------------------------------------------*/

 #include <stdint.h>
 #include <stdio.h>
 #include <stdbool.h>

 /* Global Defines ---------------------------------------------------------------------------------------------------*/

 /* Global Enum ------------------------------------------------------------------------------------------------------*/

 typedef enum __MENU_Action_e
 {
     ACTION_UP,
     ACTION_DOWN,
     ACTION_CLICK,
     ACTION_GOBACK,
 }MENU_Action_e;

/* Global Variables -------------------------------------------------------------------------------------------------*/

/* Global Functions Prototypes --------------------------------------------------------------------------------------*/

void MENU_Init          (void);
void MENU_Process       (void);
void MENU_Action        (MENU_Action_e action);
void MENU_PrintDots     (uint16_t *data, uint8_t size);
void MENU_RefreshMenu   (void);

/* ------------------------------------------------------------------------------------------------------------------*/

 #endif//__MENU_H__
