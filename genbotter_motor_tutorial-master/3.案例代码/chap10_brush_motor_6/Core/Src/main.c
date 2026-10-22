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
#include "speed_loop.h"
#include "pid_controller.h"
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

/** 本例程新增的代码 **/
typedef enum
{
  MODE_OPEN_LOOP = 0,        // 开环模式，直接设置PWM占空比
  MODE_SPEED_CLOSED_LOOP = 1 // 闭环模式，设置目标RPM
} RunMode_t;

RunMode_t current_mode = MODE_SPEED_CLOSED_LOOP; // 默认当前模式为开环模式
float target_val = 0.0f;                         // 默认目标值为0(可能是Duty或RPM)
extern PID_Handle_t hspeed_pid; // 这样你才能在 main 里的 LCD 显示函数读取 pid 数据

uint8_t lcd_buf[50];                             // LCD显示缓冲区
/** 本例程新增的代码 **/

// 电流传感器校准相关，本例程暂时不用
//static uint8_t current_calibrated = 0; // 校准完成标记（0：未校准，1：已校准）
//static uint8_t calibrate_request = 0;  // 校准请求（0：无请求，1：需要校准）

extern uint16_t adc_raw_data[ADC_TOTAL_SAMPLES]; // 原始ADC采样值（单通道×采样次数）
extern float adc_filtered_data[ADC_CHANNEL_NUM]; // 滤波后的ADC平均值（浮点型）
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
  lcd_show_string(10, 60, 450, 24, 24, "Chap10: Speed Loop PID", BLUE);

  BSP_Encoder_Init(); // 初始化编码器模块
  // 根据实际使用的编码器进行启动
  BSP_Encoder_Start(ENCODER_PM1); // 启动PM1编码器
  BrushMotor_Init(&pm1_motor);
  BSP_Encoder_Start(ENCODER_PM1); // 若编码器计数停止，启动编码器
  HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_raw_data, ADC_TOTAL_SAMPLES); // 启动ADC1的DMA采集
  BSP_CurrentSensor_Init();
  BSP_VoltageSensor_Init();
  BSP_TemperSensor_Init();

  SpeedLoop_Init(); // 初始化速度闭环控制模块

  HAL_TIM_Base_Start_IT(&htim6); // 启动定时器6中断，用于更新EnCoder、电流采样等信息
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  KeyPressedID key_id = KEY_None;
  int duty = 0;
  uint16_t send_cnt = 0; // 用于控制串口发送频率的计数器

  while (1)
  {
    /* --- 1. 按键处理 --- */
    key_id = Key_Scan();
    if (key_id == KEY0_Pressed) //k0,用于切换模式（开环控制 or 闭环控制）
    {
      // 切换模式前，先停车；
      target_val = 0.0f;
      BrushMotor_Stop();

      if (current_mode == MODE_OPEN_LOOP)
      {
        current_mode = MODE_SPEED_CLOSED_LOOP;
        lcd_show_string(10, 60, 270, 24, 24, "Mode: Speed Closed Loop", GREEN);
      }
      else
      {
        current_mode = MODE_OPEN_LOOP;
        lcd_show_string(10, 60, 270, 24, 24, "Mode: Open Loop         ", GREEN);
      }
    }
    else if (key_id == KEY1_Pressed) // k1，加速 / 反转减速
    {
      if (current_mode == MODE_OPEN_LOOP)
      {
        target_val += 10.0f; // 开环模式下，增加PWM占空比
        if (target_val > 100.0f)
          target_val = 100.0f; // 限幅
      }
      else // 闭环模式下，增加目标RPM
      {
        target_val += 20.0f; // 增加50 RPM
        if (target_val > 200.0f)
          target_val = 200.0f; // 限制最高目标转速
      }
      BrushMotor_Enable();
    }
    else if (key_id == KEY2_Pressed)  // k2，减速 / 反转加速
    {
      if (current_mode == MODE_OPEN_LOOP)
      {
        target_val -= 10.0f; // 开环模式下，减少PWM占空比
        if (target_val < -100.0f)
          target_val = -100.0f; // 限幅
      }
      else // 闭环模式下，减少目标RPM
      {
        target_val -= 20.0f; // 减少50 RPM
        if (target_val < -200.0f)
          target_val = -200.0f; // 限幅
      }
      BrushMotor_Enable();
    }

    /* --- 2. 执行电机控制逻辑 --- */
    if (current_mode == MODE_OPEN_LOOP)
    {
      // 开环模式，直接设置PWM占空比
      duty = (int)target_val;
      BrushMotor_SetSpeed(abs(duty)); // 设置速度
      if (duty > 0)
        BrushMotor_SetDirection(MOTOR_FORWARD); // 正转
      else if (duty < 0)
        BrushMotor_SetDirection(MOTOR_REVERSE); // 反转
      else
        BrushMotor_Stop(); // 停止
    }
    else
    {
      // 闭环模式，只需要设置目标RPM
      SpeedLoop_SetTargetRPM(target_val);
    }

    /* --- 3. 屏幕刷新 & 串口发送 (每 200ms 刷一次，避免闪烁) --- */
    static uint32_t last_disp_time = 0;
    static float current_duty = 0;
    if (HAL_GetTick() - last_disp_time > 200)
    {
      // 1. 显示目标值
      sprintf((char *)lcd_buf, "Target: %6.1f   ", target_val);
      lcd_show_string(10, 100, 240, 24, 24, (char *)lcd_buf, BLUE);

      // 2. 显示实际实际转速
      float real_rpm = BSP_Encoder_GetSpeedRPM(ENCODER_PM1);
      sprintf((char *)lcd_buf, "Actual: %6.1f RPM   ", real_rpm);
      lcd_show_string(10, 140, 240, 24, 24, (char *)lcd_buf, BLACK);

      // 3. 显示PWM
      if (current_mode == MODE_OPEN_LOOP)
      {
        current_duty = target_val; // 开环模式下，目标值是pwm输出值
      }
      else
      {
        // 闭环模式下，占空比是PID的计算输出结果
        extern PID_Handle_t hspeed_pid;
        current_duty = hspeed_pid.Output;
      }
      sprintf((char *)lcd_buf, "Duty  : %6.1f %% ", current_duty);
      lcd_show_string(10, 180, 240, 24, 24, (char *)lcd_buf, RED);
      last_disp_time = HAL_GetTick();
    }

    // 读取编码器数据
    float rpm1 = BSP_Encoder_GetSpeedRPM(ENCODER_PM1);

    // 读取电流数据
    float current_ma = BSP_CurrentSensor_GetCurrent();

    // 读取电压值并显示
    float voltage_v = BSP_VoltageSensor_GetPowerVoltage();

    // 读取温度值并显示
    float temper_c = BSP_TemperSensor_GetTemperature();
    // 串口发送（每 10*10 + 10*可能的按键消抖所用时长 ms 一次）
    if (++send_cnt >= 100)
    { // 假设 while(1) 循环 ~50ms/次 → 10×50=500ms
      send_cnt = 0;
      sprintf((char *)lcd_buf, "PM1_Pulses: %.1f, RPM: %.2f\r\n",
              current_duty, rpm1);

      USART_SendString((char *)lcd_buf);
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
    // 1.无论什么模式，都要更新速度计算
    BSP_Encoder_UpdateSpeed();  // 更新转速计算

    // 2.闭环模式下，执行速度闭环任务
    if (current_mode == MODE_SPEED_CLOSED_LOOP)
    {
      SpeedLoop_Task();
    }
    
    // 3. 更新各传感器数据（无论开环还是闭环都更新，保持数据最新）
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
