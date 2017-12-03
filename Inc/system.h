/**********************************************************************************************************************
 * @file    system.h
 * @author  Simon Benoit
 * @date    dd-mm-20yy
 * @brief   
 *********************************************************************************************************************/

#ifndef __SYSTEM_H__
#define __SYSTEM_H__

/* Includes ---------------------------------------------------------------------------------------------------------*/

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

/* Global Defines ---------------------------------------------------------------------------------------------------*/

/* Global Enum ------------------------------------------------------------------------------------------------------*/

/* Global Variables -------------------------------------------------------------------------------------------------*/

/* Global Functions Prototypes --------------------------------------------------------------------------------------*/

void SYS_Start();
void SYS_Stop();
void SYS_GetPreHeatTimePtr (uint16_t** val);
void SYS_GetPreHeatTempPtr (uint16_t** val);
void SYS_GetSoakTimePtr    (uint16_t** val);
void SYS_GetSoakTempPtr    (uint16_t** val);
void SYS_GetReflowTimePtr  (uint16_t** val);
void SYS_GetReflowTempPtr  (uint16_t** val);
void SYS_GetCoolingTimePtr (uint16_t** val);

/* ------------------------------------------------------------------------------------------------------------------*/

#endif//__SYSTEM_H__