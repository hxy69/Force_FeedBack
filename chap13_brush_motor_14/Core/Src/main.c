/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "tim.h"
#include "usart.h"
#include "gpio.h"
#include "fsmc.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "bsp_lcd.h"
#include "bsp_key_led.h"
#include "bsp_delay.h"
#include "bsp_brush_motor.h"
#include "bsp_current_sensor.h"
#include "bsp_encoder.h"
#include "cur_speed_loop.h"
#include "user_config.h"
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

BrushMotorConfig pm1_motor = {
    .htim         = &htim1,
    .tim_channel  = TIM_CHANNEL_1,
    .tim_arr      = 2625,   /* Period = 2625-1 = 2624, 168MHz/1/(2624+1)/2 = 16kHz center-aligned */
    .sd_port      = PM1_CTRL_SD_GPIO_Port,
    .sd_pin       = PM1_CTRL_SD_Pin
};

CurSpeedLoopCtrl cur_speed_loop;  /* 双环串级控制实例，一个实例控制一个电机 */

extern uint32_t adc_injected_data;
volatile int16_t  dbg_cur_duty = 0;  /* VOFA+ 显示：电流 PI 输出占空比 */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static void Sys_LCD_Update(void);
static void Sys_Serial_Output(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

#define KEY_STEP_RPM  20.0f    /**< 按键加减速步长 (rpm) */
#define KEY_RPM_MAX   200.0f   /**< 转速上限 */
#define KEY_RPM_MIN   -200.0f  /**< 转速下限 */

/* 按键回调：每次按下 ±20rpm，首次按下自动 Start 双环 */
static void Key0_SpeedUp(void)
{
    /* KEY0: 加速 +STEP，上限 MAX */
    if (cur_speed_loop.mode != CUR_SPEED_LOOP_FULL)
    {
        cur_speed_loop.target_rpm = 0.0f;           /* 首次按下：从 0 开始加速 */
        CurSpeedLoop_Start(&cur_speed_loop);        /* 启动双环串级控制 */
        Led_On(LED1);                                /* LED1 亮 = 运行中 */
    }
    cur_speed_loop.target_rpm += KEY_STEP_RPM;       /* 目标转速 +20rpm */
    if (cur_speed_loop.target_rpm > KEY_RPM_MAX)     /* 上限钳位 */
        cur_speed_loop.target_rpm = KEY_RPM_MAX;
}

static void Key1_SpeedDown(void)
{
    /* KEY1: 减速 -STEP，下限 MIN */
    if (cur_speed_loop.mode != CUR_SPEED_LOOP_FULL)
    {
        cur_speed_loop.target_rpm = 0.0f;           /* 首次按下：从 0 开始减速 */
        CurSpeedLoop_Start(&cur_speed_loop);        /* 启动双环串级控制 */
        Led_On(LED1);                                /* LED1 亮 = 运行中 */
    }
    cur_speed_loop.target_rpm -= KEY_STEP_RPM;       /* 目标转速 -20rpm */
    if (cur_speed_loop.target_rpm < KEY_RPM_MIN)     /* 下限钳位 */
        cur_speed_loop.target_rpm = KEY_RPM_MIN;
}

static void Key2_Stop(void)
{
    /* KEY2: 停止 */
    CurSpeedLoop_Stop(&cur_speed_loop);              /* 停止双环控制（PWM清零 + SD拉低） */
    cur_speed_loop.target_rpm = 0.0f;                /* 目标转速归零 */
    Led_Off(LED1);                                   /* LED1 灭 = 停止 */
}

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
  MX_TIM3_Init();
  MX_TIM6_Init();
  /* USER CODE BEGIN 2 */
  DWT_Init();   /* DWT 延时，配合 bsp_delay 使用 */
  Key_Init();   /* 按键初始化（KEY0/KEY1/KEY2） */
  Led_Init();   /* LED 初始化（LED1=运行指示） */
  lcd_init();   /* LCD 初始化（FSMC 接口） */

  lcd_show_string(100, 10, 300, 32, 32, "Dr.GAO-Motor-1", RED);    /* 标题 */
  lcd_show_string(10, 60, 450, 24, 24, "Chap13: Cur-Speed PID", BLUE); /* 章节 */

  HAL_TIM_Base_Start(&htim1);   /* 启动 TIM1 基计数器，TRGO=Update 触发 ADC 注入转换（16kHz） */
  HAL_TIM_Base_Start_IT(&htim6); /* 启动 TIM6 周期中断（1kHz），内部软件分频至 100Hz */
  HAL_Delay(10);                                  /* 等待定时器稳定 */

  BrushMotor_Init(&pm1_motor);           /* 绑定 TIM1 CH1 到 pm1_motor */
  BrushMotor_BootstrapCharge();          /* IR2104 自举电容充电（必须先执行） */

  BrushMotor_SetDirection(MOTOR_FORWARD); /* 设置初始方向 */
  BrushMotor_SetSpeed(0);                 /* PWM 占空比 = 0（电机不转） */
  BrushMotor_Enable();                    /* SD 拉高，使能 IR2104 */
  HAL_Delay(500);                         /* 等待电机驱动自举电压稳定 */

  lcd_show_string(10, 240, 450, 24, 24, "Calibrating zero offset...", RED);
  BSP_CurrentSensor_Init(); /* 读取 ADC 零偏，校准 offset */
  lcd_show_string(10, 240, 450, 24, 24, "Zero offset calibrated.    ", GREEN);

  BrushMotor_Disable(); /* 校准完零偏后先禁用，等按键触发才使能 */

  BSP_Encoder_Init();                  /* 初始化编码器 GPIO 和变量 */
  BSP_Encoder_Start(ENCODER_PM1);      /* 启动 TIM3 编码器模式计数 */

  CurSpeedLoop_Init(&cur_speed_loop, &pm1_motor); /* 初始化双环控制器（参数来自 user_config.h） */

  Led_Off(LED1);  /* 初始化完成，灭灯 */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  {
    KeyPressedID key_id;
    uint16_t lcd_cnt  = 0;   /* LCD 刷新计数器：每 100 次循环（~500ms）刷新 */
    uint16_t send_cnt = 0;   /* 串口发送计数器：每 20 次循环（~100ms）发送 */

    while (1)
    {
      key_id = Key_Scan();   /* 扫描按键（消抖在 bsp_key_led.c 内部完成） */

      if (key_id == KEY0_Pressed)       Key0_SpeedUp();
      else if (key_id == KEY1_Pressed)  Key1_SpeedDown();
      else if (key_id == KEY2_Pressed)  Key2_Stop();

      if (++lcd_cnt >= 100)  { lcd_cnt = 0;  Sys_LCD_Update(); }   /* 100 × 5ms = 500ms */
      if (++send_cnt >= 20) { send_cnt = 0; Sys_Serial_Output(); } /* 20 × 5ms = 100ms */

      HAL_Delay(5);  /* 主循环节拍 = 5ms，降低按键扫描 CPU 占用 */
    }
  }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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

/*==============================================================================
 * HAL 回调函数 — 都写在 main.c USER CODE 4 区域，不被 CubeMX 覆盖
 *============================================================================*/

/**
 * @brief TIM 周期+溢出回调
 * @note  TIM3 溢出 → 编码器溢出处理（16位计数器翻转）
 *        TIM6 溢出 → 每 1kHz 触发，cnt 累加到 SPEED_DIVIDER(10) 后执行速度外环
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM3)
    {
        BSP_Encoder_HandleOverflow(htim);  /* 处理 16 位编码器计数器溢出（正转 +65536，反转 -65536） */
    }
    else if (htim->Instance == TIM6)
    {
        static uint32_t speed_cnt = 0;
        CurSpeedLoop_CurrentISR(&cur_speed_loop); /* 1kHz：电流 PI（current_mA 由 ADC 回调更新） */
        if (++speed_cnt >= SPEED_DIVIDER)
        {
            speed_cnt = 0;
            BSP_Encoder_UpdateSpeed();
            CurSpeedLoop_SpeedTask(&cur_speed_loop);
        }
    }
}

void HAL_ADCEx_InjectedConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1)
    {
        adc_injected_data = HAL_ADCEx_InjectedGetValue(hadc, ADC_INJECTED_RANK_1);
        BSP_CurrentSensor_Update();            /* 16kHz 过采样累加 → 每16次平均+LPF */
    }
}

/* LCD 双组对齐布局（字体 size=24，每字符 width = size/2 = 12px，屏幕 480×320）
 *
 *   格式："Label:值单位" 左右两组固定列对齐
 *     "Target:  200rpm      Actual:  200rpm"
 *     "I_set: +0.120A       I_meas: +0.120A"
 *     "I_err: +0.005A       Sp_err:  +200rpm"
 *
 *   左组 x=5  右组 x=245，值用 sprintf 固宽格式化保证列对齐
 *   最右 ~445px，480px 内有余量
 */
#define LCD_FONT_SZ   24                        /* 数据字体大小（字库支持 12/16/24/32） */
#define LCD_X_LEFT     5                        /* 左组 x */
#define LCD_X_RIGHT  245                        /* 右组 x */
#define LCD_ROW1_Y   120                        /* 行1 y：速度行 */
#define LCD_ROW2_Y   154                        /* 行2 y：电流行（120+24+10间距） */
#define LCD_ROW3_Y   188                        /* 行3 y：误差行 */

static void Sys_LCD_Update(void)
{
    char buf[20];
    uint16_t y;

    if (cur_speed_loop.mode == CUR_SPEED_LOOP_FULL)
    {
        lcd_show_string(10, 90, 470, 24, 24,
            "Mode: CUR-SPEED [RUN]      ", GREEN);

        /* Row 1: 速度 — Target:xxxrpm  Actual:xxxrpm */
        y = LCD_ROW1_Y;
        sprintf(buf, "Target:%5.0frpm ", cur_speed_loop.target_rpm);
        lcd_show_string(LCD_X_LEFT,  y, 240, LCD_FONT_SZ, LCD_FONT_SZ, buf, BLUE);
        sprintf(buf, "Actual:%5.0frpm", cur_speed_loop.spd_measured);
        lcd_show_string(LCD_X_RIGHT, y, 240, LCD_FONT_SZ, LCD_FONT_SZ, buf, BLUE);

        /* Row 2: 电流给定 / 实测 */
        y = LCD_ROW2_Y;
        sprintf(buf, "I_set:%+5.0fmA", cur_speed_loop.cur_setpoint);
        lcd_show_string(LCD_X_LEFT,  y, 240, LCD_FONT_SZ, LCD_FONT_SZ, buf, BLUE);
        sprintf(buf, "I_meas:%+5.0fmA", cur_speed_loop.cur_measured);
        lcd_show_string(LCD_X_RIGHT, y, 240, LCD_FONT_SZ, LCD_FONT_SZ, buf, BLUE);

        /* Row 3: 误差 */
        y = LCD_ROW3_Y;
        sprintf(buf, "I_err:%+5.0fmA", CurSpeedLoop_GetCurrentError(&cur_speed_loop));
        lcd_show_string(LCD_X_LEFT,  y, 240, LCD_FONT_SZ, LCD_FONT_SZ, buf, BLUE);
        sprintf(buf, "Sp_err:%+5.0frpm", CurSpeedLoop_GetSpeedError(&cur_speed_loop));
        lcd_show_string(LCD_X_RIGHT, y, 240, LCD_FONT_SZ, LCD_FONT_SZ, buf, BLUE);
    }
    else if (cur_speed_loop.mode == CUR_SPEED_LOOP_CURRENT_ONLY)
    {
        lcd_show_string(10, 90, 470, 24, 24,
            "Mode: CURRENT-ONLY [DEBUG] ", BLUE);
        /* 清除后两行数据区 */
        lcd_fill(0, LCD_ROW2_Y, 479, LCD_ROW3_Y + LCD_FONT_SZ, WHITE);
        y = LCD_ROW1_Y;
        sprintf(buf, "I_set:%+5.0fmA", cur_speed_loop.cur_setpoint);
        lcd_show_string(LCD_X_LEFT,  y, 240, LCD_FONT_SZ, LCD_FONT_SZ, buf, BLUE);
        sprintf(buf, "I_meas:%+5.0fmA", cur_speed_loop.cur_measured);
        lcd_show_string(LCD_X_RIGHT, y, 240, LCD_FONT_SZ, LCD_FONT_SZ, buf, BLUE);
    }
    else  /* STOPPED */
    {
        lcd_show_string(10, 90, 470, 24, 24,
            "Mode: STOPPED              ", BLUE);
        /* 清除三行数据区域，用背景色填充 */
        lcd_fill(0, LCD_ROW1_Y, 479, LCD_ROW3_Y + LCD_FONT_SZ, WHITE);
    }
}

#if SERIAL_DEBUG_ENABLE
/* 串口阻塞发送（主循环 ~100ms 调用一次，不阻塞 ISR） */
static void USART_SendString(const char *str)
{
    while (*str) {
        while (!(USART1->SR & USART_SR_TXE)); /* 等待发送寄存器空 */
        USART1->DR = (uint8_t)(*str++);       /* 逐字符发送 */
    }
}

/* VOFA+ FireWater 协议：逗号分隔浮点数帧，每 100ms 一帧 */
static void Sys_Serial_Output(void)
{
    char buf[80];
    sprintf(buf, "%.1f,%.1f,%.1f,%.1f,%d\r\n",
            cur_speed_loop.target_rpm,      /* 通道1: 目标转速 rpm */
            cur_speed_loop.spd_measured,     /* 通道2: 实测转速 rpm */
            cur_speed_loop.cur_setpoint,     /* 通道3: 电流给定(mA) = 速度PID输出 */
            cur_speed_loop.cur_measured,     /* 通道4: 实测电流(mA) = 滤波后 ADC */
            (int)dbg_cur_duty);              /* 通道5: PWM 占空比（带符号）*/
    USART_SendString(buf);
}
#endif
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
