/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
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
#include "bsp_current_sensor.h"
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
/* PM1电机配置：TIM1 CH1 (PA8) / CH1N (PB13)，IR2104半桥驱动 */
BrushMotorConfig pm1_motor = {
    .htim         = &htim1,
    .tim_channel  = TIM_CHANNEL_1,
    .tim_arr      = 8367,            /* Period=8366, ARR+1=8367，用于占空比%计算 */
    .sd_port      = PM1_CTRL_SD_GPIO_Port,
    .sd_pin       = PM1_CTRL_SD_Pin  /* PF10，IR2104 shutdown */
};

int16_t duty_percent  = 0;    /* 当前占空比，正数=正转，负数=反转，0=停止 */
int8_t  duty_step     = 20;   /* 每次按键的占空比步进值 */
uint8_t motor_running = 0;    /* 电机运行标志 */
extern uint32_t adc_injected_data; /* adc.c中定义的注入通道转换结果 */
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
  MX_FSMC_Init();
  MX_USART1_UART_Init();
  MX_ADC1_Init();
  MX_TIM1_Init();
  /* USER CODE BEGIN 2 */
  DWT_Init();
  Key_Init();
  Led_Init();
  lcd_init();

  lcd_show_string(100, 10, 300, 32, 32, "Dr.GAO-Motor-1", RED);
  lcd_show_string(10, 60, 450, 24, 24, "Chap11: Open-Loop Current Demo", BLUE);

  /*
   * 初始化流程说明：
   * 1. 启动TIM1基本定时器，TRGO（更新事件）触发ADC注入转换
   * 2. 初始化电机（SD拉低、PWM停止）
   * 3. 设置正转方向 + 0%占空比，使能SD让下桥臂导通 → 电流采样回路闭合
   * 4. 校准电流零漂（软件触发阻塞采集）
   * 5. 校准完成后关闭电机，等待按键操作
   */

  HAL_TIM_Base_Start(pm1_motor.htim);
  HAL_Delay(10);

  BrushMotor_Init(&pm1_motor);

  BrushMotor_SetDirection(MOTOR_FORWARD);
  BrushMotor_SetSpeed(0);

  BrushMotor_Enable();
  HAL_Delay(500);

  lcd_show_string(10, 240, 400, 24, 24, "Calibrating zero offset...", RED);
  BSP_CurrentSensor_Init();
  lcd_show_string(10, 240, 400, 24, 24, "Zero offset calibrated.    ", GREEN);

  BrushMotor_Disable();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  KeyPressedID key_id = KEY_None;
  char buffer[50];
  uint16_t send_cnt = 0;
  uint8_t prev_running = 0; /* 上一帧运行状态，避免重复调HAL */
  uint8_t prev_duty    = 0; /* 上一帧占空比 */

  while (1)
  {
    key_id = Key_Scan();

    /* ---- KEY0：启动/调速，占空比在 -100 ~ +100 之间循环 ---- */
    if (key_id == KEY0_Pressed)
    {
      Led_Toggle(LED1);
      motor_running = 1;

      duty_percent += duty_step;
      if (duty_percent >= 100)
      {
        duty_percent = 100;
        duty_step = -20;
      }
      else if (duty_percent <= -100)
      {
        duty_percent = -100;
        duty_step = 20;
      }

      if (duty_percent > 0)
        lcd_show_string(10, 90, 400, 24, 24, "KEY0: Forward             ", BLUE);
      else if (duty_percent < 0)
        lcd_show_string(10, 90, 400, 24, 24, "KEY0: Reverse             ", BLUE);
      else
        lcd_show_string(10, 90, 400, 24, 24, "KEY0: Stop                ", BLUE);
    }
    /* ---- KEY1：停止电机并重新校准零漂 ---- */
    else if (key_id == KEY1_Pressed)
    {
      lcd_show_string(10, 90, 400, 24, 24, "KEY1: Motor Stop         ", BLUE);
      Led_Toggle(LED2);
      duty_percent = 0;
      motor_running = 0;

      /* 占空比置0、SD拉高，使下桥臂导通→电流回路闭合→可校准 */
      BrushMotor_SetSpeed(0);
      BrushMotor_Enable();
      HAL_Delay(10);

      BSP_CurrentSensor_CalibrateOffset();

      BrushMotor_Disable();
    }
    /* ---- KEY2：运行中校准零漂（电机不停） ---- */
    else if (key_id == KEY2_Pressed)
    {
      lcd_show_string(10, 90, 400, 24, 24, "KEY2: CalibrateOffset    ", BLUE);

      BrushMotor_SetSpeed(0);
      BrushMotor_Enable();
      HAL_Delay(10);

      BSP_CurrentSensor_CalibrateOffset();

      if (!motor_running)
      {
        BrushMotor_Disable();
      }
    }

    /* ---- 电机控制：仅在状态变化时执行，避免每50ms重复调HAL ---- */
    if (motor_running)
    {
      if (duty_percent != 0)
      {
        if (!prev_running || prev_duty != duty_percent)
        {
          /* 先设方向（内部Stop→Stop→Start），再设速度。
             如果先SetSpeed再SetDirection，SetDirection内部会把CCR清零。 */
          if (duty_percent > 0)
          {
            BrushMotor_SetDirection(MOTOR_FORWARD);
          }
          else
          {
            BrushMotor_SetDirection(MOTOR_REVERSE);
          }

          BrushMotor_SetSpeed(abs(duty_percent));

          BrushMotor_Enable();
        }
      }
      else
      {
        BrushMotor_Disable();
      }
    }
    else
    {
      BrushMotor_Disable();
    }

    prev_running = motor_running;
    prev_duty    = duty_percent;

    /* ---- LCD显示，每50ms刷新 ---- */
    {
      float current_ma = BSP_CurrentSensor_GetCurrent();

      sprintf(buffer, "Duty: %d%%            ", duty_percent);
      lcd_show_string(10, 120, 400, 24, 24, buffer, BLUE);

      sprintf(buffer, "Offset: %.1f    ", BSP_CurrentSensor_GetOffset());
      lcd_show_string(10, 150, 400, 24, 24, buffer, BLUE);

      sprintf(buffer, "Current: %.1f mA    ", motor_running ? current_ma : 0.0f);
      lcd_show_string(10, 180, 400, 24, 24, buffer, BLUE);

      sprintf(buffer, "ADC Raw: %u         ", adc_injected_data);
      lcd_show_string(10, 210, 400, 24, 24, buffer, BLUE);
    }

    /* ---- 串口发送，每500ms一次 ---- */
    if (++send_cnt >= 10)
    {
      send_cnt = 0;
      float current_ma = BSP_CurrentSensor_GetCurrent();
      sprintf(buffer, "Duty:%d%%, Current:%.1f mA\r\n", duty_percent, current_ma);
      USART_SendString(buffer);
    }

    HAL_Delay(50);
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
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM3 || htim->Instance == TIM2)
  {
    /* 编码器溢出回调（后续案例使用） */
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
