/**********************************************************************************************************************
 * @file    max31855.h
 * @author  Simon Benoit
 * @date    dd-mm-20yy
 * @brief   
 *********************************************************************************************************************/

#ifndef __MAX31855_H__
#define __MAX31855_H__

/* Includes ---------------------------------------------------------------------------------------------------------*/

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

/* Global Defines ---------------------------------------------------------------------------------------------------*/

/* Global Enum ------------------------------------------------------------------------------------------------------*/

/* Global Variables -------------------------------------------------------------------------------------------------*/

/* Global Functions Prototypes --------------------------------------------------------------------------------------*/

float   MAX31855_readCelsius    (void);
float   MAX31855_readCJCelsius  (void);
char    MAX31855_readStatus     (void);

/* ------------------------------------------------------------------------------------------------------------------*/

 #endif//__MAX31855_H__