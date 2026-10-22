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
#include <string.h>
#include "bsp_lcd.h"
#include "bsp_key_led.h"
#include "bsp_delay.h"
#include "bsp_brush_motor.h"
#include "bsp_current_sensor.h"
#include "pid.h"
#include "current_loop.h"
#include "user_config.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/*---- DWT 滴答时钟（μs）---- */
/*
 * 利用 Cortex-M4 DWT 周期计数器做高精度时间戳。
 * DWT_CYCCNT 每时钟周期 +1，168MHz 下约 59.5ns 分辨率。
 */
#define DWT_TICK_US  (DWT->CYCCNT / (SystemCoreClock / 1000000u))
#define DWT_TICK_MS  (DWT_TICK_US / 1000u)

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* ---- 电机驱动配置（PM1：TIM1 CH1 PA8 / CH1N PB13，IR2104 半桥） ---- */
BrushMotorConfig pm1_motor = {
    .htim         = &htim1,
    .tim_channel  = TIM_CHANNEL_1,
    .tim_arr      = 2625,           /* Period=2624, ARR+1=2625，占空比%计算基准 */
    .sd_port      = PM1_CTRL_SD_GPIO_Port,
    .sd_pin       = PM1_CTRL_SD_Pin /* PF10，IR2104 shutdown */
};

/* ---- 电流环控制实例 ---- */
CurrentLoopCtrl current_loop;

/* ---- 开环模式变量 ---- */
int16_t open_duty_percent  = 0;     /* 开环占空比，正=正转，负=反转              */
int8_t  open_duty_step     = 20;    /* 开环步进值                                */
uint8_t open_motor_running = 0;     /* 开环电机运行标志                          */

/* ---- 演示状态 ---- */
uint8_t closed_loop_active = 0;      /* 闭环演示是否激活（KEY0控制启停）         */
extern uint32_t adc_injected_data;   /* adc.c 定义的注入通道转换结果              */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static void Sys_LCD_Update(uint32_t tick_ms);
static void Sys_Serial_Output(uint32_t tick_ms);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/*==============================================================================
 * 按键处理函数
 *============================================================================*/

/**
 * @brief KEY0：闭环演示（极性轮转启停）
 * @note  按一下启动（±100mA 交替），再按停止。
 *        +100mA → 停 → -100mA → 停 → +100mA → ...
 */
static void Key0_ClosedLoop(void)
{
    static int8_t target_polarity = 1;

    if (CurrentLoop_GetState(&current_loop) == CURRENT_LOOP_RUNNING)
    {
        CurrentLoop_SetTarget(&current_loop, 0.0f);
        CurrentLoop_Stop(&current_loop);
        closed_loop_active = 0;
        Led_Off(LED1);
    }
    else
    {
        float target = (float)target_polarity * CURRENT_TARGET_DEFAULT_mA;
        CurrentLoop_SetTarget(&current_loop, target);
        CurrentLoop_Start(&current_loop);
        closed_loop_active = 1;
        open_motor_running  = 0;  /* 关闭开环（互斥） */
        Led_On(LED1);
        target_polarity = -target_polarity;
    }
}

/**
 * @brief KEY1：开环演示（占空比循环）
 * @note  0 → 20% → 40% → 60% → 80% → 100% → -100% → -80% → ... → 0
 */
static void Key1_OpenLoop(void)
{
    /* 如果闭环在跑，先停掉 */
    if (closed_loop_active)
    {
        CurrentLoop_Stop(&current_loop);
        closed_loop_active = 0;
        Led_Off(LED1);
    }

    Led_Toggle(LED2);
    open_motor_running = 1;
    open_duty_percent += open_duty_step;

    if (open_duty_percent >= 100)    { open_duty_percent = 100;  open_duty_step = -20; }
    else if (open_duty_percent <= -100) { open_duty_percent = -100; open_duty_step = 20;  }
}

/**
 * @brief KEY2：紧急失能（SD 拉低，停止一切）
 */
static void Key2_EmergencyStop(void)
{
    CurrentLoop_Stop(&current_loop);
    closed_loop_active = 0;
    open_motor_running  = 0;
    open_duty_percent   = 0;
    BrushMotor_Disable();
    Led_Off(LED1);
    Led_Off(LED2);
}

/**
 * @brief 将开环占空比应用到电机驱动
 * @note  只在状态变化时调用 SetDirection，避免重复操作 HAL 状态机。
 */
static void OpenLoop_ApplyDuty(void)
{
    static int16_t prev_duty    = 0;
    static uint8_t prev_running = 0;

    if (open_motor_running && open_duty_percent != 0)
    {
        if (!prev_running || prev_duty != open_duty_percent)
        {
            if (open_duty_percent > 0)
                BrushMotor_SetDirection(MOTOR_FORWARD);
            else
                BrushMotor_SetDirection(MOTOR_REVERSE);
            BrushMotor_SetSpeed(abs(open_duty_percent));
            BrushMotor_Enable();
        }
    }
    else
    {
        BrushMotor_Disable();
    }

    prev_duty    = open_duty_percent;
    prev_running = open_motor_running;
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
  /* USER CODE BEGIN 2 */
  DWT_Init();
  Key_Init();
  Led_Init();
  lcd_init();

  /* ---- 屏幕标题 ---- */
  lcd_show_string(100, 10, 300, 32, 32, "Dr.GAO-Motor-1", RED);
  lcd_show_string(10, 60, 450, 24, 24, "Chap11: Current-Loop PID", BLUE);

  /* ---- 电流传感器与零漂校准 ---- */
  /*
   * 流程：
   * 1. 启动 TIM1 基本定时器（TRGO 触发 ADC 注入转换）
   * 2. 初始化电机（SD 拉低、PWM 停止）
   * 3. 设置正转方向 + 0% 占空比，使能 SD 让下桥臂导通 → 电流采样回路闭合
   * 4. 校准电流零漂（软件触发阻塞采集 100 次取平均）
   * 5. 校准完成后保持电机停止，等待按键操作
   */
  HAL_TIM_Base_Start(pm1_motor.htim);
  HAL_Delay(10);

  BrushMotor_Init(&pm1_motor);

  /* 自举电容预充电：50%互补PWM短暂运行，双半桥等位不转，Cboot充满 */
  BrushMotor_BootstrapCharge();

  BrushMotor_SetDirection(MOTOR_FORWARD);
  BrushMotor_SetSpeed(0);
  BrushMotor_Enable();
  HAL_Delay(500);

  lcd_show_string(10, 240, 400, 24, 24, "Calibrating zero offset...", RED);
  BSP_CurrentSensor_Init();  /* 内部先阻塞校准，再开启注入中断 */
  lcd_show_string(10, 240, 400, 24, 24, "Zero offset calibrated.    ", GREEN);

  BrushMotor_Disable();

  /* ---- 初始化电流环控制器（绑定电机驱动，加载默认 PID 参数） ---- */
  CurrentLoop_Init(&current_loop, &pm1_motor);

  /* ---- 初始状态指示 ---- */
  Led_Off(LED1);
  Led_Off(LED2);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  /*
   * 简单同步调度（与 chap10 一致，方便调试）
   */
  {
    KeyPressedID key_id = KEY_None;
    char buffer[60];
    uint16_t send_cnt = 0;
    uint16_t lcd_cnt   = 0;
    uint32_t last_tick = 0;

    while (1)
    {
      key_id = Key_Scan();  /* 无按键时立即返回 KEY_None */

      /* ---- 按键处理 ---- */
      if (key_id == KEY0_Pressed)
      {
        Key0_ClosedLoop();
      }
      else if (key_id == KEY1_Pressed)
      {
        Key1_OpenLoop();
      }
      else if (key_id == KEY2_Pressed)
      {
        Key2_EmergencyStop();
      }

      /* ---- 电流环/开环控制 ---- */
      /* 注：闭环 PID 在 ADC ISR 中运行（1000Hz），主循环仅负责按键/LCD/串口 */
      if (!closed_loop_active)
      {
        OpenLoop_ApplyDuty();
      }

      /* ---- LCD 刷新（每 100 次 ~100ms，减少 FSMC 开销） ---- */
      if (++lcd_cnt >= 100)
      {
        lcd_cnt = 0;
        Sys_LCD_Update(0);
      }

      /* ---- 串口输出（每 500 次 ~500ms） ---- */
      if (++send_cnt >= 500)
      {
        send_cnt = 0;
        Sys_Serial_Output(0);
      }

      HAL_Delay(1);  /* 控制周期 ~1ms */
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
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM3 || htim->Instance == TIM2)
  {
    /* 编码器溢出回调（后续案例使用） */
  }
}

/*==============================================================================
 * LCD 显示更新
 *============================================================================*/

/**
 * @brief 按模式刷新 LCD 显示（每 ~100ms 调用一次，减少 FSMC 开销）
 * @param tick_ms 备用参数
 */
static void Sys_LCD_Update(uint32_t tick_ms)
{
    (void)tick_ms;
    char buffer[60];
    float current_ma = BSP_CurrentSensor_GetCurrent();

    if (closed_loop_active)
    {
        sprintf(buffer, "Mode: CLOSED-LOOP [RUN]          ");
        lcd_show_string(10, 90, 450, 24, 24, buffer, GREEN);

        sprintf(buffer, "Target: %.0f mA                 ",
                CurrentLoop_GetTarget(&current_loop));
        lcd_show_string(10, 120, 400, 24, 24, buffer, BLUE);

        sprintf(buffer, "Current: %.0f mA  Duty: %.0f%%   ",
                current_ma, PID_GetOutput(&current_loop.pid));
        lcd_show_string(10, 150, 400, 24, 24, buffer, BLUE);

        sprintf(buffer, "Error: %+.0f mA                  ",
                CurrentLoop_GetError(&current_loop));
        lcd_show_string(10, 180, 400, 24, 24, buffer, BLUE);
    }
    else
    {
        sprintf(buffer, "Mode: OPEN-LOOP                  ");
        lcd_show_string(10, 90, 450, 24, 24, buffer, BLUE);

        sprintf(buffer, "Duty: %d%%                      ", open_duty_percent);
        lcd_show_string(10, 120, 400, 24, 24, buffer, BLUE);

        sprintf(buffer, "Current: %.0f mA                ",
                open_motor_running ? current_ma : 0.0f);
        lcd_show_string(10, 150, 400, 24, 24, buffer, BLUE);

        sprintf(buffer, "                                ");
        lcd_show_string(10, 180, 400, 24, 24, buffer, BLUE);
    }

    sprintf(buffer, "Offset: %.1f  ADC: %u            ",
            BSP_CurrentSensor_GetOffset(), (unsigned int)adc_injected_data);
    lcd_show_string(10, 210, 400, 24, 24, buffer, BLUE);
}

/*==============================================================================
 * 串口调试输出
 *============================================================================*/

/**
 * @brief 按模式输出调试信息到 USART1
 * @param tick_ms 备用参数
 */
static void Sys_Serial_Output(uint32_t tick_ms)
{
    (void)tick_ms;
#if SERIAL_DEBUG_ENABLE
    char buffer[100];
    float current_ma = BSP_CurrentSensor_GetCurrent();

    if (closed_loop_active)
    {
        float target  = CurrentLoop_GetTarget(&current_loop);
        float output  = CurrentLoop_GetOutput(&current_loop);
        float error   = CurrentLoop_GetError(&current_loop);

        sprintf(buffer,
                "[CL] Target:%.0fmA Actual:%.0fmA Duty:%.0f%% Error:%+.0fmA\r\n",
                target, current_ma, output, error);
    }
    else
    {
        sprintf(buffer,
                "[OL] Duty:%d%% Current:%.0fmA\r\n",
                open_duty_percent, current_ma);
    }

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
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
