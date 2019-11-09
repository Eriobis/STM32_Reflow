/**********************************************************************************************************************
 * @file    system.c
 * @author  Simon Benoit
 * @date    02-12-2017
 * @brief
 *********************************************************************************************************************/

/* Includes ---------------------------------------------------------------------------------------------------------*/

#include "stm32l0xx_hal.h"
#include "PID.h"
#include "max31855.h"
#include "menu.h"
#include "system.h"
#include "pwm.h"

/* Local Defines ----------------------------------------------------------------------------------------------------*/

#define TEMP_POLL_TIME_MS    200

/* Local Typedefs ---------------------------------------------------------------------------------------------------*/

typedef enum __SYS_StartType_e
{
    SYS_RAMP,
    SYS_FIXED_TEMP,
}SYS_StartType_e;

typedef enum __SYS_State_e
{
  SYS_STATE_IDLE,
    SYS_STATE_INIT,
    SYS_STATE_RAMP_TO_SOAK,
    SYS_STATE_SOAKING,
    SYS_STATE_RAMP_TO_PEAK,
  SYS_STATE_REFLOWING,
    SYS_STATE_COOLDOWN,
}SYS_State_e;

/* Forward Declarations ---------------------------------------------------------------------------------------------*/

static void     SYS_GenerateProfile (SYS_Profile_e *profile);
static void     SYS_ResetProfile    (SYS_Profile_e *profile);

/* Local Constants --------------------------------------------------------------------------------------------------*/

/* Local Variables --------------------------------------------------------------------------------------------------*/

static uint16_t         SYS_FixedTemp = 100;
static uint16_t         SYS_SoakTime = 90;
static uint16_t         SYS_SoakTemp = 100;
static uint16_t         SYS_SoakTempMax = 140;
static uint16_t         SYS_ReflowTime = 100;
static uint16_t         SYS_ReflowTemp = 183;
static uint16_t         SYS_PeakTemp = 235;
static uint16_t         SYS_CoolingTime = 240;
static bool             SYS_Started;
static uint32_t         SYS_TemperatureTimer;
static uint32_t         SYS_TemperatureRateTimer;
static uint32_t         SYS_CooldownTimer = 0;
static uint8_t          SYS_SoakRate = 3;    // deg/s
static SYS_Profile_e    profile1;   // For the moment only 1 profile is handled
static SYS_StartType_e  SYS_StartType;

static float            actualTemp;
static float            lastTemp;
static float            tempRate;

// Structure to strore PID data and pointer to PID structure
static struct pid_controller ctrldata;
static PID_t pid;
// Control loop input,output and setpoint variables
static float input = 0, output = 0;
static float setpoint = 0;
// Control loop gains
static float kp = 2, ki = 1, kd = 2;

static SYS_State_e state;
static uint32_t stateTimer;

// Must fit the SYS_State_e states
char *stateStrPtr[] = {"Idle/Stopped", "Init", "Soak/Ramp", "Soaking", "Reflow/Ramp", "Reflowing", "Cooldown"};

extern uint16_t temperature;

/* Local Functions --------------------------------------------------------------------------------------------------*/

static void SYS_GenerateProfile(SYS_Profile_e *profile)
{
    uint16_t x0,x1;
    uint16_t y0,y1;
    float ratio;
    float a, b;

    profile->SoakTemp = SYS_SoakTemp;
    profile->SoakTempMax = SYS_SoakTempMax;
    profile->SoakTime = SYS_SoakTime;
    profile->ReflowTemp = SYS_ReflowTemp;
    profile->ReflowTime = SYS_ReflowTime;
    profile->PeakTemp = SYS_PeakTemp;
    profile->CoolingTime = SYS_CoolingTime;
    profile->TotalTime = profile->SoakTime + profile->ReflowTime + profile->CoolingTime;
    //Time in ms before we change setpoint
    profile->SetpointTime = ((float)profile->TotalTime/(float)NB_OF_TEMP_POINTS)*RATIO_TO_MILLI_MULT;
    profile->SetpointIndex = 0;


    // ratio = (float)profile->PreHeatTime/(float)profile->TotalTime;
    // x0 = 0;
    // y0 = 0;
    // x1 = ratio *NB_OF_TEMP_POINTS;
    // y1 = profile->PreHeatTemp;
    // a = (float)(y1-y0)/(float)(x1-x0);
    // b = y1 - (a*x1);
    // // Fill preheat with max temp, if the time is too short we will wait for the temp at the end of the cycle
    // for ( int x=x0; x<x1; x++ )
    // {
    //     profile->SetpointArray[x] = a*x + b;
    // }
    // profile->PreHeatTempIndex = x1;

    ratio = (float)profile->SoakTime/(float)profile->TotalTime;
    x0 = 0;
    y0 = 0;
    x1 = ratio * NB_OF_TEMP_POINTS + x0;
    y1 = profile->SoakTemp;
    a = (float)(y1-y0)/(float)(x1-x0);
    b = y1 - (a*x1);
    for ( int x=x0; x<x1; x++ )
    {
        profile->SetpointArray[x] = a*x + b;
    }

    ratio = (float)profile->ReflowTime/(float)profile->TotalTime;
    x0 = x1;
    y0 = profile->SoakTemp;
    x1 = ratio * NB_OF_TEMP_POINTS /2 + x0;
    y1 = profile->ReflowTemp;
    a = (float)(y1-y0)/(float)(x1-x0);
    b = y1 - (a*x1);
    for ( int x=x0; x<x1; x++ )
    {
        profile->SetpointArray[x] = a*x + b;
    }
    x0 = x1;
    x1 += (x1/2);
    a = (float)(y0 - y1)/(float)(x1-x0);
    b = y0 - (a*x1);
    for ( int x=x0; x<x1; x++ )
    {
        profile->SetpointArray[x] = a*x + b;
    }

    x0 = x1;
    x1 = 128;
    y1 = 0;
    a = (float)(y1-y0)/(float)(x1-x0);
    b = y1 - (a*x1);
    for ( int x=x0; x<x1; x++ )
    {
        profile->SetpointArray[x] = a*x + b;
    }
}

static void SYS_ResetProfile(SYS_Profile_e *profile)
{
    profile->SetpointIndex = 0;
}

static void SYS_InitGPIO()
{
    GPIO_InitTypeDef GPIO_InitStruct;

    // PC5 = Fan triac driver
    GPIO_InitStruct.Pin = GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    // PC6 = PWM OUTPUT
    GPIO_InitStruct.Pin = GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
}

/* Global Functions -------------------------------------------------------------------------------------------------*/

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
void SYS_Start()
{
    if (!SYS_IsSystemStarted())
    {
        SYS_GenerateProfile(&profile1);
        SYS_Started = true;
        state = SYS_STATE_INIT;
        SYS_StartType = SYS_RAMP;
        PWM_Start();
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
  void SYS_ManStart()
  {
      if (!SYS_IsSystemStarted())
      {
          SYS_Started = true;
          SYS_StartType = SYS_FIXED_TEMP;
          PWM_Start();
          SYS_FanStart();
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
void SYS_Stop()
{
    SYS_ResetProfile(&profile1);
    SYS_Started = false;
    PWM_Stop();
    state = SYS_STATE_INIT;
    setpoint = 0;
    if(actualTemp > 50)
    {
      SYS_FanStart();
    // Start the fan cooldown timer
      SYS_CooldownTimer = HAL_GetTick();
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
// void SYS_GetPreHeatTimePtr (uint16_t** val)
// {
//     *val = &SYS_PreHeatTime;
// }

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
// void SYS_GetPreHeatTempPtr (uint16_t** val)
// {
//   *val = &SYS_PreHeatTemp;
// }

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
void SYS_GetSoakTimePtr    (uint16_t** val)
{
  *val = &SYS_SoakTime;
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
void SYS_GetSoakTempPtr    (uint16_t** val)
{
  *val = &SYS_SoakTemp;
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
void SYS_GetReflowTimePtr  (uint16_t** val)
{
  *val = &SYS_ReflowTime;
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
void SYS_GetReflowTempPtr  (uint16_t** val)
{
  *val = &SYS_ReflowTemp;
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
  void SYS_GetFixedTempPtr  (uint16_t** val)
  {
    *val = &SYS_FixedTemp;
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
void SYS_GetPeakTempPtr (uint16_t** val)
{
  *val = &SYS_PeakTemp;
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
void SYS_GetCoolingTimePtr (uint16_t** val)
{
  *val = &SYS_CoolingTime;
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
uint16_t SYS_GetTotalTime()
{
    return  (SYS_SoakTime +  SYS_ReflowTime + SYS_CoolingTime);
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
bool SYS_IsSystemStarted()
{
    return SYS_Started;
}

void SYS_Init()
{
    SYS_Started = false;

    SYS_InitGPIO();
    // Prepare PID controller for operation
    pid = pid_create(&ctrldata, &input, &output, &setpoint, kp, ki, kd);
    // Set controler output limits from 0 to 100
    pid_limits(pid, 0, 100);
    // Allow PID to compute and change output
    pid_auto(pid);

}

void SYS_Process()
{
    float tj;
    static uint32_t soakStepTimer = 0;

    // Get actual temperature every 300ms
    if ( HAL_GetTick() - SYS_TemperatureTimer >= TEMP_POLL_TIME_MS )
    {
        actualTemp = MAX31855_readCelsius(); //MAX6675_readCelsius();
        tj = MAX31855_readCJCelsius();
        SYS_TemperatureTimer = HAL_GetTick();
    }
    // Every seconds, update the rate of change
    if ( HAL_GetTick() - SYS_TemperatureRateTimer >= 1000 )
  {
            tempRate = actualTemp - lastTemp;
          lastTemp = actualTemp;
            SYS_TemperatureRateTimer = HAL_GetTick();
  }
    if(SYS_Started)
    {
        // Check if need to compute PID
        if (pid_need_compute(pid))
        {
            // Read process feedback
            input = actualTemp;
            // Compute new PID output value
            pid_compute(pid);
            //Change actuator value
            PWM_SetDutyCycle((uint8_t)output);
        }

        switch (SYS_StartType)
        {
            case SYS_RAMP:

                switch(state)
                {
                  case SYS_STATE_IDLE:
                    break;
                    case SYS_STATE_INIT:
                        state = SYS_STATE_RAMP_TO_SOAK;
                        soakStepTimer = HAL_GetTick();
                        // Max 4 deg/sec
                        setpoint = actualTemp;
                        break;
                    case SYS_STATE_RAMP_TO_SOAK:

                      // Ramping stops when we enter =/- 2 deg around setpoint
                        if ((actualTemp >= (profile1.SoakTemp - 2)) && (actualTemp <= (profile1.SoakTemp + 2)))
                        {
                            state = SYS_STATE_SOAKING;
                            stateTimer = HAL_GetTick();
                        }
                        else if(HAL_GetTick() - soakStepTimer >= 1000)
            {
                          soakStepTimer = HAL_GetTick();
              if(setpoint < (profile1.SoakTemp - SYS_SoakRate) &&
                ((actualTemp >= (setpoint - 3)) && (actualTemp <= (setpoint + 3))))
              {
                setpoint += SYS_SoakRate;
              }
            }
                        break;
                    case SYS_STATE_SOAKING:
                        // Soaking is time based
                        if(HAL_GetTick() - stateTimer >= (profile1.SoakTime*1000))
                        {
                            setpoint = profile1.PeakTemp;
                            state = SYS_STATE_RAMP_TO_PEAK;
                        }
                        else if(HAL_GetTick() - soakStepTimer >= 1000)
            {
                          soakStepTimer = HAL_GetTick();
              if(setpoint < (profile1.SoakTempMax))
              {
                setpoint ++;
              }
            }
                        break;
                    case SYS_STATE_RAMP_TO_PEAK:
                        if (actualTemp >= profile1.ReflowTemp)
                        {
                            stateTimer = HAL_GetTick();
                            state = SYS_STATE_REFLOWING;
                        }
                        break;
                    case SYS_STATE_REFLOWING:
                        if (actualTemp >= profile1.PeakTemp)
                        {
                            setpoint = profile1.ReflowTemp;
                        }
                        if(HAL_GetTick() - stateTimer >= (profile1.ReflowTime*1000))
                        {
                            state = SYS_STATE_COOLDOWN;
                        }
                        break;
                    case SYS_STATE_COOLDOWN:
                        SYS_Stop();
                        state = SYS_STATE_IDLE;
                        setpoint = 0;
                        break;
                }
                MENU_RefreshMenu();
                break;

            case SYS_FIXED_TEMP:
                setpoint = SYS_FixedTemp;
                break;
        }
    }
    else // System stopped
    {
        if (SYS_CooldownTimer && (HAL_GetTick() - SYS_CooldownTimer > 120000))
        {
            SYS_FanStop();
            SYS_CooldownTimer = 0;
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
SYS_Profile_e *SYS_GetProfile()
{
    return &profile1;
}

/**
  *--------------------------------------------------------------------------------------------------------------------
  * @brief Start the fan
  *
  * @param  none
  *
  * @retval none
  *
  *--------------------------------------------------------------------------------------------------------------------
  */
void SYS_FanStart()
{
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_SET);
}

/**
  *--------------------------------------------------------------------------------------------------------------------
  * @brief  Stop the fan
  *
  * @param  none
  *
  * @retval none
  *
  *--------------------------------------------------------------------------------------------------------------------
  */

void SYS_FanStop()
{
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_RESET);
}

float SYS_GetActualTemp()
{
    return actualTemp;
}

float SYS_GetActualSetpoint()
{
    return setpoint;
}

float SYS_GetActualRate()
{
    return tempRate;
}

char *SYS_GetCurrentStateStr()
{
  return stateStrPtr[state];
}
// uint16_t SYS_GetPreHeatTime() { return SYS_PreHeatTime; }
// uint16_t SYS_GetPreHeatTemp() { return SYS_PreHeatTemp; }
uint16_t SYS_GetSoakTime()    { return SYS_SoakTime;    }
uint16_t SYS_GetSoakTemp()    { return SYS_SoakTemp;    }
uint16_t SYS_GetReflowTime()  { return SYS_ReflowTime;  }
uint16_t SYS_GetReflowTemp()  { return SYS_ReflowTemp ; }
uint16_t SYS_GetCoolingTime() { return SYS_CoolingTime; }
uint16_t SYS_GetFixedTemp()   { return SYS_FixedTemp;   }
