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

#define NB_OF_TEMP_POINTS   128
#define RATIO_TO_MILLI_MULT 1000

/* Global Enum ------------------------------------------------------------------------------------------------------*/

typedef struct _SYS_Profile_e
{
    uint16_t PreHeatTime;
    uint16_t PreHeatTemp;
    uint16_t PreHeatTempIndex;
    uint16_t SoakTime;
    uint16_t SoakTemp;
    uint16_t ReflowTime;
    uint16_t ReflowTemp;
    uint16_t CoolingTime;
    uint16_t TotalTime;
    uint16_t SetpointTime;
    uint16_t SetpointArray[NB_OF_TEMP_POINTS];
    uint16_t SetpointIndex;
}SYS_Profile_e;

/* Global Variables -------------------------------------------------------------------------------------------------*/

/* Global Functions Prototypes --------------------------------------------------------------------------------------*/

void            SYS_Process             (void);
bool            SYS_IsSystemStarted     (void);
void            SYS_Init                (void);
void            SYS_Start               (void);
void            SYS_ManStart            (void);
void            SYS_Stop                (void);
void            SYS_GetPreHeatTimePtr   (uint16_t** val);
void            SYS_GetPreHeatTempPtr   (uint16_t** val);
void            SYS_GetSoakTimePtr      (uint16_t** val);
void            SYS_GetSoakTempPtr      (uint16_t** val);
void            SYS_GetReflowTimePtr    (uint16_t** val);
void            SYS_GetReflowTempPtr    (uint16_t** val);
void            SYS_GetCoolingTimePtr   (uint16_t** val);
void            SYS_GetFixedTempPtr     (uint16_t** val);
uint16_t        SYS_GetTotalTime        (void);
uint16_t        SYS_GetPreHeatTime      (void);
uint16_t        SYS_GetPreHeatTemp      (void);
uint16_t        SYS_GetSoakTime         (void);
uint16_t        SYS_GetSoakTemp         (void);
uint16_t        SYS_GetReflowTime       (void);
uint16_t        SYS_GetReflowTemp       (void);
uint16_t        SYS_GetCoolingTime      (void);
uint16_t        SYS_GetFixedTemp        (void);
SYS_Profile_e   *SYS_GetProfile         (void);
float           SYS_GetActualTemp       (void);

/* ------------------------------------------------------------------------------------------------------------------*/

#endif//__SYSTEM_H__
