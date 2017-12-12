/**********************************************************************************************************************
 * @file    pwm.c
 * @author  Simon Benoit
 * @date    10-12-2017
 * @brief
 *********************************************************************************************************************/

/* Includes ---------------------------------------------------------------------------------------------------------*/

#include "pwm.h"

/* Local Defines ----------------------------------------------------------------------------------------------------*/

/* Local Typedefs ---------------------------------------------------------------------------------------------------*/

typedef enum __PWM_State_e
{
    PWM_INIT,
    PWM_HIGH,
    PWM_LOW
}PWM_State_e;

/* Forward Declarations ---------------------------------------------------------------------------------------------*/

static void PWM_SetGPIO();

/* Local Constants --------------------------------------------------------------------------------------------------*/

/* Local Variables --------------------------------------------------------------------------------------------------*/

GPIO_TypeDef *pwmGPIO;
uint16_t pwmGPIO_Pin;
PWM_State_e pwmState;
uint8_t frequency;
uint8_t dutyCycle;
uint32_t pwmTOn;
uint32_t pwmTimer;
uint32_t pwmPeriod;
bool pwmStarted;

/* Local Functions --------------------------------------------------------------------------------------------------*/

static void PWM_SetGPIO()
{
    GPIO_InitTypeDef GPIO_InitStruct;

    /**TIMx GPIO Configuration
     PA2     ------> PWM_Output
    */
    GPIO_InitStruct.Pin = GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
}

/* Global Functions -------------------------------------------------------------------------------------------------*/

void PWM_Process()
{
    if (pwmStarted)
    {
        switch (pwmState)
        {
            case PWM_INIT:
                pwmTimer = HAL_GetTick();
                if ( dutyCycle > 0 )
                {
                    HAL_GPIO_WritePin(pwmGPIO, pwmGPIO_Pin, GPIO_PIN_SET);
                    pwmState = PWM_HIGH;
                }
            break;

            case PWM_HIGH:
                if (HAL_GetTick() - pwmTimer > pwmTOn) //PwmPeriod is half a period
                {
                    if ( dutyCycle < 100 )
                    {
                        HAL_GPIO_WritePin(pwmGPIO, pwmGPIO_Pin, GPIO_PIN_RESET);
                    }
                    pwmState = PWM_LOW;
                    pwmTimer = HAL_GetTick();
                }
            break;

            case PWM_LOW:
                if (HAL_GetTick() - pwmTimer > (pwmPeriod - pwmTOn)) //PwmPeriod is half a period
                {
                    pwmState = PWM_INIT;
                }
            break;
        }
    }
}

/**
  *--------------------------------------------------------------------------------------------------------------------
  * @brief
  *
  * @param  none
  *
  * @retval none
  *
  *--------------------------------------------------------------------------------------------------------------------
  */
void PWM_Init(void)
{
    pwmState = PWM_INIT;
    pwmGPIO = GPIOC;
    pwmGPIO_Pin = GPIO_PIN_6;
    PWM_SetGPIO();
    pwmStarted = false;
    frequency = 5;
    dutyCycle = 0;
    pwmPeriod = ((float)1000.0/(float)frequency);
}

/**
  *--------------------------------------------------------------------------------------------------------------------
  * @brief
  *
  * @param  none
  *
  * @retval none
  *
  *--------------------------------------------------------------------------------------------------------------------
  */
void PWM_SetFrequency(uint32_t freq)
{
    if (freq < 500)
    {
        frequency = freq;
        pwmPeriod = ((float)1000.0/(float)freq);
    }
}

/**
  *--------------------------------------------------------------------------------------------------------------------
  * @brief
  *
  * @param  none
  *
  * @retval none
  *
  *--------------------------------------------------------------------------------------------------------------------
  */
void PWM_SetDutyCycle(uint8_t duty)
{
    if(duty >= 0 && duty <= 100)
    {
        dutyCycle = duty;
        pwmTOn = (pwmPeriod*dutyCycle)/100;
    }
}

void PWM_Start()
{
    pwmStarted = true;
}

void PWM_Stop()
{
    pwmStarted = false;
    pwmState = PWM_INIT;
    HAL_GPIO_WritePin(pwmGPIO, pwmGPIO_Pin, GPIO_PIN_RESET);
}
