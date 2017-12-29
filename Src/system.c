/**********************************************************************************************************************
 * @file    system.c
 * @author  Simon Benoit
 * @date    02-12-2017
 * @brief
 *********************************************************************************************************************/

/* Includes ---------------------------------------------------------------------------------------------------------*/

#include "stm32l0xx_hal.h"
#include "pid.h"
#include "max6675.h"
#include "menu.h"
#include "system.h"
#include "pwm.h"

/* Local Defines ----------------------------------------------------------------------------------------------------*/

/* Local Typedefs ---------------------------------------------------------------------------------------------------*/

/* Forward Declarations ---------------------------------------------------------------------------------------------*/

static void     SYS_GenerateProfile (SYS_Profile_e *profile);
static void     SYS_ResetProfile    (SYS_Profile_e *profile);

/* Local Constants --------------------------------------------------------------------------------------------------*/

/* Local Variables --------------------------------------------------------------------------------------------------*/

static uint16_t         SYS_PreHeatTime = 30;
static uint16_t         SYS_PreHeatTemp = 80;
static uint16_t         SYS_SoakTime = 60;
static uint16_t         SYS_SoakTemp = 120;
static uint16_t         SYS_ReflowTime = 180;
static uint16_t         SYS_ReflowTemp = 245;
static uint16_t         SYS_CoolingTime = 60;
static bool             SYS_Started;
static uint32_t         SYS_SetpointTimer;
static uint32_t         SYS_TemperatureTimer;
static SYS_Profile_e    profile1;
volatile float          actualTemp;

// Structure to strore PID data and pointer to PID structure
struct pid_controller ctrldata;
PID_t pid;

// Control loop input,output and setpoint variables
float input = 0, output = 0;
float setpoint = 0;

// Control loop gains
float kp = 2.5, ki = 1.0, kd = 1.0;

/* Local Functions --------------------------------------------------------------------------------------------------*/

static void SYS_GenerateProfile(SYS_Profile_e *profile)
{
    uint16_t x0,x1;
    uint16_t y0,y1;
    float ratio;
    float a, b;

    profile->PreHeatTemp = SYS_PreHeatTemp;
    profile->PreHeatTime = SYS_PreHeatTime;
    profile->SoakTemp = SYS_SoakTemp;
    profile->SoakTime = SYS_SoakTime;
    profile->ReflowTemp = SYS_ReflowTemp;
    profile->ReflowTime = SYS_ReflowTime;
    profile->CoolingTime = SYS_CoolingTime;
    profile->TotalTime = profile->PreHeatTime + profile->SoakTime + profile->ReflowTime + profile->CoolingTime;
    //Time in ms before we change setpoint
    profile->SetpointTime = ((float)profile->TotalTime/(float)NB_OF_TEMP_POINTS)*RATIO_TO_MILLI_MULT;
    profile->SetpointIndex = 0;

    ratio = (float)profile->PreHeatTime/(float)profile->TotalTime;
    x0 = 0;
    y0 = 0;
    x1 = ratio *NB_OF_TEMP_POINTS;
    y1 = profile->PreHeatTemp;
    a = (float)(y1-y0)/(float)(x1-x0);
    b = y1 - (a*x1);
    for ( int x=x0; x<x1; x++ )
    {
        profile->SetpointArray[x] = a*x + b;
    }

    ratio = (float)profile->SoakTime/(float)profile->TotalTime;
    x0 = x1;
    y0 = profile->PreHeatTemp;
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
        SYS_SetpointTimer = HAL_GetTick();
        SYS_Started = true;
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
void SYS_Stop()
{
    SYS_ResetProfile(&profile1);
    SYS_Started = false;
    PWM_Stop();
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
void SYS_GetPreHeatTimePtr (uint16_t** val)
{
    *val = &SYS_PreHeatTime;
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
void SYS_GetPreHeatTempPtr (uint16_t** val)
{
  *val = &SYS_PreHeatTemp;
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
    return  (SYS_PreHeatTime + SYS_SoakTime +  SYS_ReflowTime + SYS_CoolingTime);
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

    // Prepare PID controller for operation
    pid = pid_create(&ctrldata, &input, &output, &setpoint, kp, ki, kd);
    // Set controler output limits from 0 to 100
    pid_limits(pid, 0, 100);
    // Allow PID to compute and change output
    pid_auto(pid);

}

void SYS_Process()
{
    // Get actual temperature every 300ms
    if ( HAL_GetTick() - SYS_TemperatureTimer > 300 )
    {
        actualTemp = MAX6675_readCelsius();
        SYS_TemperatureTimer = HAL_GetTick();
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

        //Update the setpoint according to the current time
        if ( HAL_GetTick() - SYS_SetpointTimer > profile1.SetpointTime )
        {
            if ( profile1.SetpointIndex < NB_OF_TEMP_POINTS )
            {
                //LOGGER_CurrentTempAdd((uint16_t)actualTemp,(uint16_t)setpoint);
                setpoint = profile1.SetpointArray[profile1.SetpointIndex];
                profile1.SetpointIndex ++;
                SYS_SetpointTimer = HAL_GetTick();
            }
            else
            {
                SYS_Stop();
                profile1.SetpointIndex = 0;
            }
            MENU_RefreshMenu();
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

float SYS_GetActualTemp()
{
    return actualTemp;
}

uint16_t SYS_GetPreHeatTime() { return SYS_PreHeatTime; }
uint16_t SYS_GetPreHeatTemp() { return SYS_PreHeatTemp; }
uint16_t SYS_GetSoakTime()    { return SYS_SoakTime; }
uint16_t SYS_GetSoakTemp()    { return SYS_SoakTemp; }
uint16_t SYS_GetReflowTime()  { return SYS_ReflowTime; }
uint16_t SYS_GetReflowTemp()  { return SYS_ReflowTemp ; }
uint16_t SYS_GetCoolingTime() { return SYS_CoolingTime; }
