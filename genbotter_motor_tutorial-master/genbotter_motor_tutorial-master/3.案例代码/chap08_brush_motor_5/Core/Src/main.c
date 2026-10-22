/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
#include "fsmc.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdlib.h>
#include <stdio.h>
#include "bsp_lcd.h"
#include "bsp_key_led.h"
#include "bsp_brush_motor.h"
#include "bsp_encoder.h"
#include "bsp_current_sensor.h"
#include "bsp_voltage_sensor.h"
#include "bsp_temper_sensor.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
// BrushMotorConfig pm2_motor = {
//     .htim = &htim8, // 使用TIM8作为电机控制定时器
//     .tim_channel = TIM_CHANNEL_1, // 使用通道1
//     .tim_arr = 1000,              // 自动重装载值，假设为1000
//     .sd_port = PM2_SD_GPIO_Port,  //
//     .sd_pin = PM2_SD_Pin          //
// };
BrushMotorConfig pm1_motor = {
    .htim = &htim1,                   // 使用TIM1作为电机控制定时器
    .tim_channel = TIM_CHANNEL_1,     // 使用通道1
    .tim_arr = 1000,                  // 自动重装载值，假设为1000
    .sd_port = PM1_CTRL_SD_GPIO_Port, // GPIOF
    .sd_pin = PM1_CTRL_SD_Pin         // PF10
};

// 电流传感器校准相关
static uint8_t current_calibrated = 0; // 校准完成标记（0：未校准，1：已校准）
static uint8_t calibrate_request = 0;  // 校准请求（0：无请求，1：需要校准）

extern uint16_t adc_raw_data[ADC_TOTAL_SAMPLES];    // 原始ADC采样值（单通道×采样次数）
extern float adc_filtered_data[ADC_CHANNEL_NUM];    // 滤波后的ADC平均值（浮点型）
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_FSMC_Init();
  MX_TIM1_Init();
  MX_TIM3_Init();
  MX_TIM6_Init();
  MX_USART1_UART_Init();
  MX_ADC1_Init();
  /* USER CODE BEGIN 2 */
  DWT_Init(); // 初始化DWT

  Key_Init();
  Led_Init();
  lcd_init();

  lcd_show_string(100, 10, 300, 32, 32, "Dr.GAO-Motor-1", RED);
  lcd_show_string(10, 60, 450, 24, 24, "Chap08: Brushed_Motor_Sensing", BLUE);

  BSP_Encoder_Init(); // 初始化编码器模块
  // 根据实际使用的编码器进行启动
  BSP_Encoder_Start(ENCODER_PM1); // 启动PM1编码器
  BrushMotor_Init(&pm1_motor);
  BSP_Encoder_Start(ENCODER_PM1); // 若编码器计数停止，启动编码器
  HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_raw_data, ADC_TOTAL_SAMPLES); // 启动ADC1的DMA采集
  BSP_CurrentSensor_Init();
  BSP_VoltageSensor_Init();
  BSP_TemperSensor_Init();

  HAL_TIM_Base_Start_IT(&htim6); // 启动定时器6中断，用于更新EnCoder、电流采样等信息
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  KeyPressedID key_id = KEY_None;
  int duty = 0;
  uint16_t send_cnt = 0;
  char buffer[50];
  while (1)
  {
    key_id = Key_Scan();
    if (key_id == KEY0_Pressed)
    {
      lcd_show_string(10, 90, 200, 24, 24, "key 0 pressed.", BLUE);
      Led_Toggle(LED1);
      duty += 10;
      if (duty > 100)
      {
        duty = 100;
      }
    }
    else if (key_id == KEY1_Pressed)
    {
      lcd_show_string(10, 90, 200, 24, 24, "key 1 pressed.", BLUE);
      Led_Toggle(LED2);

      duty -= 10;
      if (duty < -100)
      {
        duty = -100;
      }
    }
    else if (key_id == KEY2_Pressed)
    {
      lcd_show_string(10, 90, 200, 24, 24, "key 2 pressed.", BLUE);
      Led_Toggle(LED1);
      Led_Toggle(LED2);
      duty = 0;
      BSP_Encoder_Stop(ENCODER_PM1); // 停止编码器计数
      calibrate_request = 1; // 标记需要校准（在电机停止后执行）
    }
    else
    {
      lcd_show_string(10, 90, 200, 24, 24, "No key pressed.", BLUE);
    }

    if (duty == 0)
    {
      BrushMotor_Stop(); // 停止电机
      // 电机停止时，若未校准或收到校准请求，执行零漂校准
      if (current_calibrated == 0 || calibrate_request == 1)
      {
        lcd_show_string(10, 270, 300, 24, 24, "Calibrating current offset...", RED);
        BSP_CurrentSensor_CalibrateOffset(); // 执行校准
        current_calibrated = 1;              // 标记为已校准
        calibrate_request = 0;               // 清除请求
        lcd_show_string(10, 270, 300, 24, 24, "Current offset calibrated   ", GREEN);
      }
    }
    else
    {
      BrushMotor_SetDirection(duty > 0 ? MOTOR_FORWARD : MOTOR_REVERSE);
      BrushMotor_SetSpeed(abs(duty));
      BrushMotor_Enable();
      if (!BSP_Encoder_IsRunning(ENCODER_PM1))
      {
        BSP_Encoder_Start(ENCODER_PM1);
      }
    }

    sprintf(buffer, "PWM Duty: %d   ", duty);
    lcd_show_string(10, 120, 200, 24, 24, buffer, BLUE);

    /*测速代码*/
    // 读取编码器数据
    int32_t count1 = BSP_Encoder_GetCount(ENCODER_PM1);
    float rpm1 = BSP_Encoder_GetSpeedRPM(ENCODER_PM1);
    sprintf(buffer, "PM1_Pulses: %ld, RPM: %.1f   ", (long)count1, rpm1);
    lcd_show_string(10, 150, 400, 24, 24, buffer, BLUE);

    // 读取电流数据
    float current_ma = BSP_CurrentSensor_GetCurrent();
    sprintf(buffer, "Current: %.1f mA   ", current_ma);
    lcd_show_string(10, 180, 300, 24, 24, buffer, BLUE);

    // 读取电压值并显示
    float voltage_v = BSP_VoltageSensor_GetPowerVoltage();
    sprintf(buffer, "Voltage: %.1f V", voltage_v);
    lcd_show_string(10, 210, 300, 24, 24, buffer, BLUE);

    // 读取温度值并显示
    float temper_c = BSP_TemperSensor_GetTemperature();
    sprintf(buffer, "Temperature: %.1f C", temper_c);
    lcd_show_string(10, 240, 300, 24, 24, buffer, BLUE);
    
    // 串口发送（每 10*10 + 10*可能的按键消抖所用时长 ms 一次）
    if (++send_cnt >= 10)
    { // 假设 while(1) 循环 ~50ms/次 → 10×50=500ms
      send_cnt = 0;
      sprintf(buffer, "PM1_Pulses: %ld, RPM: %.2f, Current: %.1f mA, Voltage: %.2f V, Temperature: %.2f C\r\n",
              (long)count1, rpm1, current_ma, voltage_v, temper_c);

      USART_SendString(buffer);
    }
    HAL_Delay(10);
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
// 中断回调函数
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM3 || htim->Instance == TIM2)
  {
    BSP_Encoder_HandleOverflow(htim);
  }
  else if (htim->Instance == TIM6)
  {
    BSP_Encoder_UpdateSpeed();  // 更新转速计算
    BSP_CurrentSensor_Update(); // 更新电流检测结果
    BSP_VoltageSensor_Update(); // 更新电压检测结果
    BSP_TemperSensor_Update();  // 更新温度检测结果
  }
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
