/**
  ******************************************************************************
  * File Name          : main.c
  * Description        : Main program body
  ******************************************************************************
  ** This notice applies to any and all portions of this file
  * that are not between comment pairs USER CODE BEGIN and
  * USER CODE END. Other portions of this file, whether
  * inserted by the user or by software development tools
  * are owned by their respective copyright owners.
  *
  * COPYRIGHT(c) 2017 STMicroelectronics
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/

#include "main.h"
#include "stm32l0xx_hal.h"
#include "ssd1306.h"
#include "fonts.h"
#include "menu.h"
#include "system.h"
#include "PID.h"
#include "pwm.h"

typedef enum __ENCODER_State_e
{
    ENC_STATE_00,
    ENC_STATE_01,
    ENC_STATE_10,
    ENC_STATE_11,
    ENC_STATE_UNKNOWN,
}ENCODER_State_e;

typedef enum __ADC_CHANNEL_e
{
    ADC_REF,
    ADC_TEMP
}ADC_CHANNEL_e;

/* Private variables ---------------------------------------------------------*/

ADC_HandleTypeDef adc1;
I2C_HandleTypeDef hi2c1;
SPI_HandleTypeDef hspi1;
ENCODER_State_e encoderState;
ENCODER_State_e lastState;
uint32_t plus = 0;
uint32_t moin = 0;
volatile uint32_t adcValue;
volatile uint32_t adcRef;
ADC_CHANNEL_e CurrentSample;
uint32_t adcTempSum;
uint16_t adcTempCount;
uint16_t temperature;

/* Private function prototypes -----------------------------------------------*/

void SystemClock_Config(void);
static void EncoderRead(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_SPI1_Init(void);
static void MX_ADC1_Init(void);

extern void SSD1306LibTest();

/* Private function prototypes -----------------------------------------------*/

int main(void)
{
    /* MCU Configuration----------------------------------------------------------*/
    uint32_t encoderTimer;
    uint32_t encoderSwitchTimer;
    uint32_t encoderSwitchPeriod;
    uint32_t AdcTimer;
    ADC_ChannelConfTypeDef sConfig;

    encoderState = ENC_STATE_UNKNOWN;
    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init();
    /* Configure the system clock */
    SystemClock_Config();
    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_I2C1_Init();
    MX_SPI1_Init();

    MENU_Init();
    SYS_Init();
    PWM_Init();
    encoderSwitchPeriod = 100;
  /*  adcTempSum = 0;
    adcTempCount = 0;
    HAL_ADC_Start_IT(&adc1);
*/
    while (1)
    {
  /*      if ( adcTempCount >= 16 )
        {
            uint32_t t;
            t = adcTempSum/adcTempCount;
            adcTempCount = 0;
            adcTempSum = 0;
            temperature = (uint32_t)(-0.0431*(float)t + (float)192.41);
        }
    */
     /*   if (HAL_GetTick() - AdcTimer > 20)
        {
            HAL_ADC_Start_IT(&adc1);
            AdcTimer = HAL_GetTick();
        }
*/
        MENU_Process();
        SYS_Process();
        PWM_Process();

        if (HAL_GetTick() - encoderSwitchTimer > encoderSwitchPeriod)
        {
            encoderSwitchTimer = HAL_GetTick();
            encoderSwitchPeriod = 100;
            if ( HAL_GPIO_ReadPin(ENCODER_SW_Port, ENCODER_SW_Pin) == GPIO_PIN_SET )
            {
                MENU_Action(ACTION_CLICK);
                encoderSwitchPeriod = 500;
            }
        }
    }
}

GPIO_PinState lastVal1;
static void EncoderRead()
{
    GPIO_PinState val1, val2;
    val1 = HAL_GPIO_ReadPin(ENCODER_A_Port,ENCODER_A_Pin);
    val2 = HAL_GPIO_ReadPin(ENCODER_B_Port,ENCODER_B_Pin);

   switch (encoderState)
    {
        case ENC_STATE_UNKNOWN :
            if ( !val1 && !val2 )
                encoderState = ENC_STATE_00;
        break;
        case ENC_STATE_00 :
            if ( !val1 && val2)
            {
                if (lastState != ENC_STATE_01)
                {
                    lastState = encoderState;
                    encoderState = ENC_STATE_01;
                    MENU_Action(ACTION_DOWN);
                    moin++;
                }
            }
            else if ( val1 && !val2 )
            {
                if (lastState != ENC_STATE_10)
                {
                    lastState = encoderState;
                    encoderState = ENC_STATE_10;
                    MENU_Action(ACTION_UP);
                    plus++;
                }
            }
        break;
        case ENC_STATE_01 :
            if ( !val1 && !val2 )
            {
                encoderState = ENC_STATE_00;
            }
            else if ( val1 && val2 )
            {
                encoderState = ENC_STATE_11;
            }
        break;
        case ENC_STATE_11 :
            if ( val1 && !val2 )
            {
                encoderState = ENC_STATE_10;
            }
            else if ( !val1 && val2 )
            {
                encoderState = ENC_STATE_01;
            }
        break;
        case ENC_STATE_10 :
            if ( val1 && val2 )
            {
                encoderState = ENC_STATE_11;
            }
            else if ( !val2 && !val1 )
            {
                encoderState = ENC_STATE_00;
            }
        break;
    }
}

/** System Clock Configuration
*/
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct;
    RCC_ClkInitTypeDef RCC_ClkInitStruct;
    RCC_PeriphCLKInitTypeDef PeriphClkInit;
      /**Configure the main internal regulator output voltage
      */
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

      /**Initializes the CPU, AHB and APB busses clocks
      */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = 16;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLLMUL_4;
    RCC_OscInitStruct.PLL.PLLDIV = RCC_PLLDIV_2;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
      _Error_Handler(__FILE__, __LINE__);
    }

      /**Initializes the CPU, AHB and APB busses clocks
      */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                                |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
    {
      _Error_Handler(__FILE__, __LINE__);
    }

    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART2|RCC_PERIPHCLK_I2C1;
    PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
    PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_PCLK1;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
      _Error_Handler(__FILE__, __LINE__);
    }

      /**Configure the Systick interrupt time
      */
    HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

      /**Configure the Systick
      */
    HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

    /* SysTick_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}

static void MX_ADC1_Init(void)
{
    //ADC_ChannelConfTypeDef conf;

    ADC_ChannelConfTypeDef sConfig;
    __HAL_RCC_ADC1_CLK_ENABLE();

        /**Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
        */
      adc1.Instance = ADC1;
      adc1.Init.OversamplingMode = DISABLE;
      adc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
      adc1.Init.Resolution = ADC_RESOLUTION_12B;
      adc1.Init.SamplingTime = ADC_SAMPLETIME_79CYCLES_5;
      adc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
      adc1.Init.ContinuousConvMode = DISABLE;
      adc1.Init.DiscontinuousConvMode = DISABLE;
      adc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
      adc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
      adc1.Init.DMAContinuousRequests = DISABLE;
      adc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
      adc1.Init.Overrun = ADC_OVR_DATA_PRESERVED;
      adc1.Init.LowPowerAutoWait = DISABLE;
      adc1.Init.LowPowerFrequencyMode = DISABLE;
      adc1.Init.LowPowerAutoPowerOff = DISABLE;
      if (HAL_ADC_Init(&adc1) != HAL_OK)
      {
        _Error_Handler(__FILE__, __LINE__);
      }

        /**Configure for the selected ADC regular channel to be converted.
        */
      sConfig.Channel = ADC_CHANNEL_14;
      sConfig.Rank = ADC_RANK_CHANNEL_NUMBER;
      if (HAL_ADC_ConfigChannel(&adc1, &sConfig) != HAL_OK)
      {
        _Error_Handler(__FILE__, __LINE__);
      }
      HAL_NVIC_EnableIRQ(ADC1_COMP_IRQn);

      /**Configure for the selected ADC regular channel to be converted.
        */
     /* sConfig.Channel = ADC_CHANNEL_VREFINT;
      if (HAL_ADC_ConfigChannel(&adc1, &sConfig) != HAL_OK)
      {
        _Error_Handler(__FILE__, __LINE__);
      }*/
    /*
    adc1.Instance = ADC1;
    adc1.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV4;
    adc1.Init.Resolution = ADC_RESOLUTION_12B;
    adc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    adc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    adc1.Init.ContinuousConvMode = DISABLE;
    adc1.Init.DiscontinuousConvMode = DISABLE;
    adc1.Init.SamplingTime = ADC_SAMPLETIME_79CYCLES_5;
    adc1.Init.OversamplingMode = DISABLE;
    if (HAL_ADC_Init(&adc1) != HAL_OK)
    {
      _Error_Handler(__FILE__, __LINE__);
    }
    conf.Channel = ADC_CHANNEL_10;
    conf.Rank = ADC_RANK_CHANNEL_NUMBER;
    HAL_ADC_ConfigChannel(&adc1, &conf);
*/
}

/* I2C1 init function */
static void MX_I2C1_Init(void)
{
    hi2c1.Instance = I2C1;
    hi2c1.Init.Timing = 0x00707CBB;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2 = 0;
    hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&hi2c1) != HAL_OK)
    {
      _Error_Handler(__FILE__, __LINE__);
    }

      /**Configure Analogue filter
      */
    if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
    {
      _Error_Handler(__FILE__, __LINE__);
    }

      /**Configure Digital filter
      */
    if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
    {
      _Error_Handler(__FILE__, __LINE__);
    }
}

/* SPI1 init function */
static void MX_SPI1_Init(void)
{
    /* SPI2 parameter configuration*/
    hspi1.Instance = SPI2;
    hspi1.Init.Mode = SPI_MODE_MASTER;
    hspi1.Init.Direction = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi1.Init.NSS = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
    hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi1.Init.CRCPolynomial = 7;
    if (HAL_SPI_Init(&hspi1) != HAL_OK)
    {
        _Error_Handler(__FILE__, __LINE__);
    }
}

/** Configure pins as
        * Analog
        * Input
        * Output
        * EVENT_OUT
        * EXTI
*/
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

    /*Configure GPIO pin : B1_Pin */
    GPIO_InitStruct.Pin = B1_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

    /*Configure GPIO pin : LD2_Pin */
    GPIO_InitStruct.Pin = LD2_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

    /*Configure GPIO pin : ENCODER SWITCH */
    GPIO_InitStruct.Pin = ENCODER_SW_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(ENCODER_SW_Port, &GPIO_InitStruct);

    /*Configure GPIO pin : ENCODER KNOB */
    GPIO_InitStruct.Pin = ENCODER_A_Pin | ENCODER_B_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(ENCODER_A_Port, &GPIO_InitStruct);

    /*Configure GPIO pin : MAX31855 CS */
    GPIO_InitStruct.Pin = MAX31855_CS_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(MAX31855_CS_Port, &GPIO_InitStruct);

    /*Configure GPIO pin : ADC PC4 */
    GPIO_InitStruct.Pin = GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    HAL_NVIC_EnableIRQ(EXTI0_1_IRQn);
    HAL_NVIC_EnableIRQ(EXTI2_3_IRQn);
}

/**
  * @brief  EXTI line detection callbacks.
  * @param  GPIO_Pin: Specifies the pins connected to the EXTI line.
  * @retval None
  */

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    switch (GPIO_Pin)
    {
        case  GPIO_PIN_1:
            break;
        case  GPIO_PIN_2:
        case  GPIO_PIN_3:
            EncoderRead();
            break;
        default:
            break;
    }
}

void ADC1_COMP_IRQHandler (void)
{
    HAL_ADC_IRQHandler(&adc1);
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
    uint32_t value;
    ADC_ChannelConfTypeDef sConfig;

    if( __HAL_ADC_GET_FLAG(&adc1, ADC_FLAG_EOC))
    {
            adcValue = HAL_ADC_GetValue(&adc1);
            adcTempSum += adcValue;
            adcTempCount ++;
    }
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
void _Error_Handler(char * file, int line)
{
    /* USER CODE BEGIN Error_Handler_Debug */
    /* User can add his own implementation to report the HAL error return state */
    while(1)
    {
    }
    /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT

/**
   * @brief Reports the name of the source file and the source line number
   * where the assert_param error has occurred.
   * @param file: pointer to the source file name
   * @param line: assert_param error line source number
   * @retval None
   */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
    ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */

}

#endif

/**
  * @}
  */

/**
  * @}
*/

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
