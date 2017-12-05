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

void        SYS_Start();
void        SYS_Stop();
void        SYS_GetPreHeatTimePtr   (uint16_t** val);
void        SYS_GetPreHeatTempPtr   (uint16_t** val);
void        SYS_GetSoakTimePtr      (uint16_t** val);
void        SYS_GetSoakTempPtr      (uint16_t** val);
void        SYS_GetReflowTimePtr    (uint16_t** val);
void        SYS_GetReflowTempPtr    (uint16_t** val);
void        SYS_GetCoolingTimePtr   (uint16_t** val);
uint16_t    SYS_GetTotalTime        (void);
uint16_t    SYS_GetPreHeatTime      (void);
uint16_t    SYS_GetPreHeatTemp      (void);
uint16_t    SYS_GetSoakTime         (void);
uint16_t    SYS_GetSoakTemp         (void);
uint16_t    SYS_GetReflowTime       (void);
uint16_t    SYS_GetReflowTemp       (void);
uint16_t    SYS_GetCoolingTime      (void);
/* ------------------------------------------------------------------------------------------------------------------*/

#endif//__SYSTEM_H__
