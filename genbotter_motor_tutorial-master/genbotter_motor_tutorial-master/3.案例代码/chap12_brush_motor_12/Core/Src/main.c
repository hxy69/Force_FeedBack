/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body — 直流有刷电机位置环 PID 控制
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
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
#include "bsp_encoder.h"
#include "pid.h"
#include "position_loop.h"
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

/* ---- 电机驱动配置（PM1：TIM1 CH1 PA8 / CH1N PB13） ---- */
BrushMotorConfig pm1_motor = {
    .htim              = &htim1,
    .tim_channel       = TIM_CHANNEL_1,
    .tim_arr_plus1     = 1000,       /* ARR=999, 分辨率 = ARR+1 = 1000 */
    .sd_port      = PM1_CTRL_SD_GPIO_Port,
    .sd_pin       = PM1_CTRL_SD_Pin
};

/* ---- 位置环控制实例 ---- */
PositionLoopCtrl position_loop;

/* ---- TIM6 1ms 系统心跳 ---- */
volatile uint32_t g_tick_1ms = 0;

/* ---- 报告更新标志（ISR 每 200ms 置位，主循环执行 LCD + 串口） ---- */
volatile uint8_t g_report_flag = 0;

/* ==== 调试变量：确认 PID 更新是否真正执行 ==== */
volatile uint32_t g_pid_run_cnt = 0;   /* PositionLoop_Update 累计执行次数 */
volatile int16_t  g_dbg_last_pwm = 0;  /* 最后一次 PID 输出的 PWM 值 */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static void Sys_LCD_Update(void);
static void Sys_Serial_Output(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/*==============================================================================
 * 按键处理
 *============================================================================*/

/** @brief KEY0: 目标位置 +1 圈（STEP_ANGLE_COUNTS = 1320 counts） */
static void Key0_AddAngle(void)
{
    int32_t new_target = PositionLoop_GetTarget(&position_loop) + STEP_ANGLE_COUNTS;
    if (new_target > POSITION_MAX) new_target = POSITION_MAX;
    PositionLoop_SetTarget(&position_loop, new_target);
    Led_Toggle(LED1);
}

/** @brief KEY1: 目标位置 -1 圈（STEP_ANGLE_COUNTS = 1320 counts） */
static void Key1_SubAngle(void)
{
    int32_t new_target = PositionLoop_GetTarget(&position_loop) - STEP_ANGLE_COUNTS;
    if (new_target < POSITION_MIN) new_target = POSITION_MIN;
    PositionLoop_SetTarget(&position_loop, new_target);
    Led_Toggle(LED2);
}

/** @brief KEY2: 复位到原点 */
static void Key2_ResetOrigin(void)
{
    PositionLoop_RequestHome(&position_loop);
    Led_Off(LED1);
    Led_Off(LED2);
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
  MX_TIM1_Init();
  MX_TIM3_Init();
  MX_TIM6_Init();
  /* USER CODE BEGIN 2 */
  DWT_Init();
  Key_Init();
  Led_Init();
  lcd_init();

  /* ---- 屏幕标题 ---- */
  lcd_show_string(100, 10, 300, 32, 32, "Dr.GAO-Motor-1", RED);
  lcd_show_string(10,  60, 450, 24, 24, "Chap12: Position-Loop PID", BLUE);

  /* ---- 初始化电机驱动 ---- */
  BrushMotor_Init(&pm1_motor);
  BrushMotor_BootstrapCharge();

  /* ---- 初始化编码器 ---- */
  BSP_Encoder_Init();
  BSP_Encoder_Start(ENCODER_PM1);

  /* ---- 初始化位置环 ---- */
  PositionLoop_Init(&position_loop, &pm1_motor);

  /* ---- LED 初始状态 ---- */
  Led_Off(LED1);
  Led_Off(LED2);

  /* TIM6 优先级设为 1（高于 TIM3 的 2），保证编码器读取不被溢出中断打断 */
  HAL_NVIC_SetPriority(TIM6_DAC_IRQn, 1, 0);

  /* ---- 启动 TIM6 1ms 系统心跳 ---- */
  HAL_TIM_Base_Start_IT(&htim6);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* ---- 按键扫描（10ms 消抖，松手检测） ---- */
    {
      uint8_t key = Key_Scan(0);
      if (key == KEY0_Pressed)      Key0_AddAngle();
      else if (key == KEY1_Pressed) Key1_SubAngle();
      else if (key == KEY2_Pressed) Key2_ResetOrigin();
    }

    /* ---- LCD + 串口更新 ---- */
    if (g_report_flag)
    {
      g_report_flag = 0;
      Sys_LCD_Update();
      Sys_Serial_Output();
    }

    HAL_Delay(10);   /* 主循环 10ms 节拍 */
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
 * TIM 中断回调
 *============================================================================*/

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM6)
  {
    static uint32_t pid_cnt    = 0;
    static uint32_t report_cnt = 0;
    static uint32_t speed_cnt  = 0;

    g_tick_1ms++;

    /* PID 位置环（周期 = PID_PERIOD_TICKS ms） */
    if (++pid_cnt >= PID_PERIOD_TICKS)
    {
      pid_cnt = 0;
      PositionLoop_Update(&position_loop);
    }

    /* 编码器速度更新（SPEED_UPDATE_MS = 100ms） */
    if (++speed_cnt >= 100u)
    {
      speed_cnt = 0;
      BSP_Encoder_UpdateSpeed();
    }

    /* 报告更新标志（主循环执行 LCD + 串口） */
    if (++report_cnt >= REPORT_PERIOD_TICKS)
    {
      report_cnt = 0;
      g_report_flag = 1;
    }
  }
  else if (htim->Instance == TIM3)
  {
    BSP_Encoder_HandleOverflow(htim);
  }
}

/*==============================================================================
 * LCD 显示更新（在主循环中执行，不在 ISR 中）
 *============================================================================*/

static void Sys_LCD_Update(void)
{
    char buffer[60];
    PositionState_t state = PositionLoop_GetState(&position_loop);

    /* 行1：运行状态 */
    sprintf(buffer, "State: %s                          ",
            PositionLoop_StateName(state));
    lcd_show_string(10, 90, 450, 24, 24, buffer,
                    (state == POS_STATE_HOLDING) ? GREEN : BLUE);

    /* 行2：目标位置 + 对应圈数 */
    {
        int32_t t = PositionLoop_GetTarget(&position_loop);
        int32_t t_abs = (t < 0) ? -t : t;
        sprintf(buffer, "Target: %ld (%c%ld.%01ld rev)        ",
                (long)t, (t < 0) ? '-' : '+', (long)(t_abs / 1320),
                (long)((t_abs % 1320) * 10 / 1320));
    }
    lcd_show_string(10, 120, 400, 24, 24, buffer, BLUE);

    /* 行3：当前位置 + 圈数 */
    {
        int32_t f = PositionLoop_GetFeedback(&position_loop);
        int32_t f_abs = (f < 0) ? -f : f;
        sprintf(buffer, "Actual: %ld (%c%ld.%01ld rev)        ",
                (long)f, (f < 0) ? '-' : '+', (long)(f_abs / 1320),
                (long)((f_abs % 1320) * 10 / 1320));
    }
    lcd_show_string(10, 150, 400, 24, 24, buffer, BLUE);

    /* 行4：误差 + PWM 百分比（pwm_final 已是 ±50% 域） */
    {
        sprintf(buffer, "Error: %ld  PWM: %+d%%             ",
                (long)PositionLoop_GetError(&position_loop),
                (int)position_loop.pwm_final);
    }
    lcd_show_string(10, 180, 400, 24, 24, buffer, BLUE);

    /* 行5：PID 模式 + 周期 */
    sprintf(buffer, "Mode: IncrPD  Period: %lums          ",
            (unsigned long)PID_PERIOD_TICKS);
    lcd_show_string(10, 210, 400, 24, 24, buffer, BLUE);
}

/*==============================================================================
 * 串口调试输出（在主循环中执行，不在 ISR 中）
 *============================================================================*/

static void Sys_Serial_Output(void)
{
#if VOFA_FIREWATER_ENABLE
    /* ---- VOFA+ FireWater 协议：逗号分隔浮点数 + 换行 ----
     *
     * FireWater 格式：ch0,ch1,ch2,ch3\n
     * 既是有效的 VOFA+ 数据帧，也能在普通串口终端直读。
     *
     * 通道定义：
     *   ch0: 目标位置（counts）
     *   ch1: 实际位置（counts）
     *   ch2: 位置误差（counts）
     *   ch3: PWM 输出（%，有符号）
     *
     * VOFA+ 上位机设置：
     *   协议：FireWater
     *   绑定串口后自动绘图，无需额外配置
     * -------------------------------------------------------- */
    char vofa_buf[80];
    sprintf(vofa_buf, "%.1f,%.1f,%.1f,%.1f\n",
            (float)PositionLoop_GetTarget(&position_loop),
            (float)PositionLoop_GetFeedback(&position_loop),
            (float)PositionLoop_GetError(&position_loop),
            (float)position_loop.pwm_final);
    USART_SendString(vofa_buf);
#else
    /* ---- 文本格式调试输出 ---- */
    char buffer[100];
    sprintf(buffer,
            "[POS] T:%ld A:%ld E:%ld %s PWM:%+d%% [run:%lu dbg:%+d]\r\n",
            (long)PositionLoop_GetTarget(&position_loop),
            (long)PositionLoop_GetFeedback(&position_loop),
            (long)PositionLoop_GetError(&position_loop),
            PositionLoop_StateName(PositionLoop_GetState(&position_loop)),
            (int)position_loop.pwm_final,
            (unsigned long)g_pid_run_cnt,
            (int)g_dbg_last_pwm);
    USART_SendString(buffer);
#endif
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
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
