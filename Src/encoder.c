
/**********************************************************************************************************************
 * @file    encoder.c
 * @author  Simon Benoit
 * @date    20-09-2017
 * @brief   Encoder interrupt and position handling
 **********************************************************************************************************************

/* Includes ---------------------------------------------------------------------------------------------------------*/

#include "stm32l0xx_hal.h"
#include "main.h"
#include "encoder.h"
#include "menu.h"

/* Local Defines ----------------------------------------------------------------------------------------------------*/

/* Local Typedefs ---------------------------------------------------------------------------------------------------*/

typedef enum __ENCODER_State_e
{
    ENC_STATE_UNKNOWN,
    ENC_STATE_11,
    ENC_STATE_01,
    ENC_STATE_00,
    ENC_STATE_10,
}ENCODER_State_e;

/* Forward Declarations ---------------------------------------------------------------------------------------------*/

/* Local Constants --------------------------------------------------------------------------------------------------*/

/* Local Variables --------------------------------------------------------------------------------------------------*/

ENCODER_State_e encoderState = ENC_STATE_UNKNOWN;

/* Local Functions --------------------------------------------------------------------------------------------------*/

/* Global Functions -------------------------------------------------------------------------------------------------*/

// TODO : Remove menu call, insert clockwise/ccw callback functions

void EncoderHandle()
{
    static int8_t clockwiseCnt = 0;
    static int8_t counterClockwiseCnt = 0;
    GPIO_PinState val1, val2;
    
    val1 = HAL_GPIO_ReadPin(ENCODER_A_Port,ENCODER_A_Pin);
    val2 = HAL_GPIO_ReadPin(ENCODER_B_Port,ENCODER_B_Pin);

    switch (encoderState)
    {
        case ENC_STATE_UNKNOWN :
            if ( val1 && val2 )
            {
                encoderState = ENC_STATE_11;
            }
            else if ( val1 )
            {
                encoderState = ENC_STATE_01;
            }
            else if ( val2 )
            {
                encoderState = ENC_STATE_10;
            }
            else
            {
                encoderState = ENC_STATE_00;
            }
        break;
        case ENC_STATE_00 :
            if ( val1 )
            {
                encoderState = ENC_STATE_01;
                counterClockwiseCnt = 0;
                clockwiseCnt++;
            }
            else if ( val2 )
            {
                encoderState = ENC_STATE_10;
                clockwiseCnt = 0;
                counterClockwiseCnt++;
            }
        break;
        case ENC_STATE_01 :
            if ( !val1 )
            {
                encoderState = ENC_STATE_00;
                counterClockwiseCnt = 0;
                clockwiseCnt++;
            }
            else if ( val2 )
            {
                encoderState = ENC_STATE_11;
                clockwiseCnt = 0;
                counterClockwiseCnt++;
            }
        break;
        case ENC_STATE_11 :
            if ( !val1 )
            {
                encoderState = ENC_STATE_10;
                counterClockwiseCnt = 0;
                clockwiseCnt++;
            }
            else if ( !val2 )
            {
                encoderState = ENC_STATE_01;
                clockwiseCnt = 0;
                counterClockwiseCnt++;
            }
        break;
        case ENC_STATE_10 :
            if ( val1 )
            {
                encoderState = ENC_STATE_11;
                counterClockwiseCnt = 0;
                clockwiseCnt++;
            }
            else if ( !val2 )
            {
                encoderState = ENC_STATE_00;
                clockwiseCnt = 0;
                counterClockwiseCnt++;
            }
        break;
    }


    if (clockwiseCnt > 2)
    {
        MENU_Action(ACTION_DOWN);
        clockwiseCnt = 0;
        counterClockwiseCnt = 0;
    }
    else if (counterClockwiseCnt > 2)
    {
        MENU_Action(ACTION_UP);
        clockwiseCnt = 0;
        counterClockwiseCnt = 0;
    }
}
