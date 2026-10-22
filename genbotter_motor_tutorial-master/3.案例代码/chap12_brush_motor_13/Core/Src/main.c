/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body — 直流有刷电机 梯形速度规划 位置环 PID 控制
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
#include <stdio.h>
#include <string.h>
#include "bsp_lcd.h"
#include "bsp_key_led.h"
#include "bsp_delay.h"
#include "bsp_brush_motor.h"
#include "bsp_encoder.h"
#include "pid.h"
#include "position_loop.h"
#include "traj_planner.h"
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

/* ---- 梯形轨迹规划器 (12-03 新增) ---- */
TrajPlanner traj;

/* ---- TIM6 1ms 系统心跳 ---- */
volatile uint32_t g_tick_1ms = 0;

/* ---- 报告更新标志（ISR 每 200ms 置位，主循环执行 LCD + 串口） ---- */
volatile uint8_t g_report_flag = 0;

/* ==== 调试变量 ==== */
volatile uint32_t g_pid_run_cnt = 0;

/* ==== 轨迹启动同步 ==== */
static int32_t  g_traj_dest    = 0;   /* 已请求但尚未启动的轨迹目标位置 */
static uint8_t  g_traj_pending = 0;   /* 是否有新轨迹等待启动：1=有，0=无（按键设置，ISR 取用后清零） */
static uint8_t  g_traj_home    = 0;   /* 是否为回零轨迹：1=回零，0=普通定位（影响原点坐标处理） */

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

/** @brief KEY0: 目标位置 +1 圈
 *  @note  不在按键处理中启动轨迹——只记录目标行程，
 *         由 TIM6 ISR 在下一个 PID 周期用实时编码器值启动，
 *         消除按键时刻与 ISR 时刻之间的编码器漂移。
 *  @note  cur 取 traj.target_pos（上一条轨迹的数学终点），
 *         而非编码器读数，避免累积误差导致 Dest 不成倍数。 */
static void Key0_AddAngle(void)
{
    int32_t dest = traj.target_pos + STEP_ANGLE_COUNTS;
    if (dest > POSITION_MAX) dest = POSITION_MAX;

    g_traj_dest    = dest;
    g_traj_pending = 1;
    position_loop.state = POS_STATE_MOVING;
    Led_Toggle(LED1);
}

/** @brief KEY1: 目标位置 -1 圈 */
static void Key1_SubAngle(void)
{
    int32_t dest = traj.target_pos - STEP_ANGLE_COUNTS;
    if (dest < POSITION_MIN) dest = POSITION_MIN;

    g_traj_dest    = dest;
    g_traj_pending = 1;
    position_loop.state = POS_STATE_MOVING;
    Led_Toggle(LED2);
}

/** @brief KEY2: 复位到原点 */
static void Key2_ResetOrigin(void)
{
    g_traj_dest    = 0;
    g_traj_pending = 1;
    g_traj_home    = 1;
    position_loop.state = POS_STATE_MOVING;
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

  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

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
  lcd_show_string(10,  60, 450, 24, 24, "Chap12-03: Traj PID", BLUE);

  /* ---- 初始化电机驱动 ---- */
  BrushMotor_Init(&pm1_motor);
  BrushMotor_BootstrapCharge();

  /* ---- 初始化编码器 ---- */
  BSP_Encoder_Init();
  BSP_Encoder_Start(ENCODER_PM1);

  /* ---- 初始化位置环 ---- */
  PositionLoop_Init(&position_loop, &pm1_motor);

  /* ---- 初始化梯形轨迹规划器 ---- */
  TrajPlanner_Init(&traj, TRAJ_V_MAX, TRAJ_ACCEL);

  /* ---- LED 初始状态 ---- */
  Led_Off(LED1);
  Led_Off(LED2);

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

    HAL_Delay(10);
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

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

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

    /* ---- 位置环控制 ---- */
    if (++pid_cnt >= PID_PERIOD_TICKS)
    {
      pid_cnt = 0;

      /* 待启动轨迹：在 ISR 中用实时编码器值启动
       *
       * 为什么不在按键处理中直接启动？
       * 按键跑在 while(1) 主循环，而轨迹需要在 ISR 的 PID 周期内推演。
       * 如果按键时刻读编码器 = -5，50ms 后 ISR 执行时编码器已漂到 +10，
       * 轨迹从 -5 起步但编码器在 +10 → 第一帧误差为负 → 电机反转。
       * 推迟到 ISR 内读取 → 编码器和轨迹起步发生在同一时刻，零时序断层。 */
      if (g_traj_pending)
      {
        int32_t cur = PositionLoop_GetFeedback(&position_loop);
        TrajPlanner_Start(&traj, cur, g_traj_dest);
        PID_Reset(&position_loop.pid);
        /* 预填反馈历史：PID_Reset 将 feedback_prev 清零，
         * 但编码器当前值非零 → 首帧 D 项 = -Kd×(cur-0) = 负尖峰。
         * 预填为 cur 后 D 项 = 0，accumulated 保持纯正向。 */
        position_loop.pid.feedback_prev  = (float)cur;
        position_loop.pid.feedback_prev2 = (float)cur;
        position_loop.target    = cur;
        position_loop.state     = POS_STATE_MOVING;
        position_loop.home_flag = g_traj_home;
        g_traj_pending = 0;
        g_traj_home    = 0;
      }

      /* 轨迹未完成：推进轨迹 → 更新中间目标 */
      if (!TrajPlanner_IsDone(&traj))
      {
        int32_t setpoint = TrajPlanner_Update(&traj, 0.05f);
        PositionLoop_SetTarget(&position_loop, setpoint);
      }

      PositionLoop_Update(&position_loop);

      /* 轨迹完成后停机：两条路径
       *   ① 已进死区(HOLDING) → 立即 IDLE + 禁能
       *   ② 超时 15 帧(750ms) → 强制 IDLE + 禁能（防持续啸叫） */
      {
        static uint8_t done_cnt = 0;
        if (TrajPlanner_IsDone(&traj))
        {
          if (position_loop.state == POS_STATE_HOLDING)
          {
            position_loop.state = POS_STATE_IDLE;
            BrushMotor_Disable();
            done_cnt = 0;
          }
          else if (++done_cnt > 15)
          {
            position_loop.state = POS_STATE_IDLE;
            BrushMotor_Disable();
            done_cnt = 0;
          }
        }
        else
        {
          done_cnt = 0;
        }
      }
    }

    /* 编码器速度更新（100ms） */
    if (++speed_cnt >= 100u)
    {
      speed_cnt = 0;
      BSP_Encoder_UpdateSpeed();
    }

    /* 报告更新标志 */
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
 * LCD 显示更新
 *============================================================================*/

static void Sys_LCD_Update(void)
{
    char buffer[60];

    /* 行1：轨迹阶段 + 速度 */
    {
        const char *phase = TrajPlanner_PhaseName(traj.phase);
        sprintf(buffer, "Traj: %s  V:%3.0f cnt/s        ",
                phase, TrajPlanner_GetSpeed(&traj));
    }
    lcd_show_string(10, 90, 450, 24, 24, buffer,
                    TrajPlanner_IsDone(&traj) ? GREEN : BLUE);

    /* 行2：最终目标 + 轨迹中间目标 */
    {
        sprintf(buffer, "Dest: %ld  TSP: %ld             ",
                (long)traj.target_pos,
                (long)TrajPlanner_GetSetpoint(&traj));
    }
    lcd_show_string(10, 120, 400, 24, 24, buffer, BLUE);

    /* 行3：当前位置 + 误差 */
    {
        int32_t f = PositionLoop_GetFeedback(&position_loop);
        int32_t e = PositionLoop_GetError(&position_loop);
        sprintf(buffer, "Pos: %ld  Err: %ld              ",
                (long)f, (long)e);
    }
    lcd_show_string(10, 150, 400, 24, 24, buffer, BLUE);

    /* 行4：PWM 占空比 */
    {
        sprintf(buffer, "PWM: %+d%%  PIDcnt: %lu            ",
                (int)position_loop.pwm_final,
                (unsigned long)g_pid_run_cnt);
    }
    lcd_show_string(10, 180, 400, 24, 24, buffer, BLUE);

    /* 行5：进度 */
    {
        sprintf(buffer, "Progress: %3.0f%%                 ",
                TrajPlanner_GetProgress(&traj));
    }
    lcd_show_string(10, 210, 400, 24, 24, buffer, BLUE);
}

/*==============================================================================
 * VOFA+ / 串口输出
 *============================================================================*/

static void Sys_Serial_Output(void)
{
#if VOFA_FIREWATER_ENABLE
    /* ---- VOFA+ FireWater 协议 ----
     *
     * 通道定义 (12-03 新增 ch4: 轨迹中间目标)：
     *   ch0: 目标位置 (counts)
     *   ch1: 实际位置 (counts)
     *   ch2: 位置误差 (counts)
     *   ch3: PWM 输出 (%)
     *   ch4: 轨迹中间目标 (counts) ★ 12-03 新增
     *
     * VOFA+ 设置：协议 FireWater，绑定串口后自动绘图
     * -------------------------------------------------------- */
    char vofa_buf[100];
    sprintf(vofa_buf, "%.1f,%.1f,%.1f,%.1f,%.1f\n",
            (float)PositionLoop_GetTarget(&position_loop),
            (float)PositionLoop_GetFeedback(&position_loop),
            (float)PositionLoop_GetError(&position_loop),
            (float)position_loop.pwm_final,
            (float)TrajPlanner_GetSetpoint(&traj));
    USART_SendString(vofa_buf);
#else
    char buffer[120];
    sprintf(buffer,
            "[TRAJ-PID] T:%ld SP:%ld A:%ld E:%ld %s PWM:%+d%% [pid:%lu]\r\n",
            (long)PositionLoop_GetTarget(&position_loop),
            (long)TrajPlanner_GetSetpoint(&traj),
            (long)PositionLoop_GetFeedback(&position_loop),
            (long)PositionLoop_GetError(&position_loop),
            TrajPlanner_PhaseName(traj.phase),
            (int)position_loop.pwm_final,
            (unsigned long)g_pid_run_cnt);
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
