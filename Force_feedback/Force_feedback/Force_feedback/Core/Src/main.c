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
#include "dma.h"
#include "tim.h"
#include "usart.h"
#include "usb_otg.h"
#include "gpio.h"
#include "fsmc.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdlib.h>
#include <stdio.h>
#include "key_led.h"
#include "brush_motor.h"
#include "delay.h"
#include "lcd.h"
#include "current_sensor.h"
#include "voltage_sensor.h"
#include "temper_sensor.h"
#include "current_loop.h"
#include "pid.h"
#include "user_config.h"
#include "vofa_plus.h"
#include "pid_param_parse.h"
#include "string.h"
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
#define DWT_TICK_US (DWT->CYCCNT / (SystemCoreClock / 1000000u))
#define DWT_TICK_MS (DWT_TICK_US / 1000u)

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
BrushMotorConfig motor[3] =
    {
        {&htim1, TIM_CHANNEL_1, 2625, X_CTRL_GPIO_Port, X_CTRL_Pin, 1},
        {&htim1, TIM_CHANNEL_2, 2625, Y_CTRL_GPIO_Port, Y_CTRL_Pin, 1},
        {&htim1, TIM_CHANNEL_3, 2625, Z_CTRL_GPIO_Port, Z_CTRL_Pin, 1},
};
extern uint16_t adc_raw_data[ADC_TOTAL_SAMPLES]; // 原始ADC采样值（单通道×采样次数）
extern float adc_filtered_data[ADC_CHANNEL_NUM]; // 滤波后的ADC平均值（浮点型）

extern CurrentLoopCtrl current_loop[3];
extern PID_ParamParser_t current_pid_parse;

/* ---- 演示状态 ---- */
uint8_t closed_loop_active[3] = {0};  /* 闭环演示是否激活（KEY0控制启停）         */
extern uint32_t adc_injected_data[3]; /* adc.c 定义的注入通道转换结果              */

uint16_t vofa_send_cnt = 0; // VOFA+发送频率计数器，配合定时器中断实现定期发送调试数据到VOFA+
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
    MX_ADC1_Init();
    MX_FSMC_Init();
    MX_TIM1_Init();
    MX_TIM8_Init();
    MX_TIM6_Init();
    MX_USART1_UART_Init();
    MX_ADC2_Init();
    MX_ADC3_Init();
    MX_USB_OTG_FS_PCD_Init();
    /* USER CODE BEGIN 2 */
    DWT_Init();       // 初始化DWT
    lcd_init();       // 初始化LCD
    VOFA_Plus_Init(); // 初始化VOFA+模块

    HAL_TIM_Base_Start(&htim1);    // 打开定时器
    HAL_TIM_Base_Start_IT(&htim6); // 打开电压，温度，VOFA+的定时中断
    // HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_raw_data, ADC_TOTAL_SAMPLES); // 启动ADC1的DMA采集
    HAL_Delay(10);

    /* 电机初始化 */
    BrushMotor_Init(&motor[0]);
    BrushMotor_Init(&motor[1]);
    BrushMotor_Init(&motor[2]);

    /* 自举电容预充电：50%互补PWM短暂运行，双半桥等位不转，Cboot充满 */
    BrushMotor_BootstrapCharge(&motor[0]);
    BrushMotor_BootstrapCharge(&motor[1]);
    BrushMotor_BootstrapCharge(&motor[2]);

    /* 初始方向为正向 */
    BrushMotor_SetDirection(&motor[0], MOTOR_FORWARD);
    BrushMotor_SetDirection(&motor[1], MOTOR_FORWARD);
    BrushMotor_SetDirection(&motor[2], MOTOR_FORWARD);

    /* 初始化速度为0 */
    BrushMotor_SetSpeed(&motor[0], 0);
    BrushMotor_SetSpeed(&motor[1], 0);
    BrushMotor_SetSpeed(&motor[2], 0);

    /* 初始化电机使能 */
    BrushMotor_Enable(&motor[0]);
    BrushMotor_Enable(&motor[1]);
    BrushMotor_Enable(&motor[2]);

    HAL_Delay(500);

    /* 电流零点校准 */
    BSP_CurrentSensor_Init(); 
    // BSP_VoltageSensor_Init(); /* 电压采集初始化 */

    /* 电机关闭 */
    BrushMotor_Disable(&motor[0]); 
    BrushMotor_Disable(&motor[1]); 
    BrushMotor_Disable(&motor[2]); 

    /* ---- 初始化电流环控制器（绑定电机驱动，加载默认 PID 参数） ---- */
    CurrentLoop_Init(&current_loop[0], &motor[0]);
    current_loop[0].motor_id = 0;
    CurrentLoop_Init(&current_loop[1], &motor[1]);
    current_loop[1].motor_id = 1;
    CurrentLoop_Init(&current_loop[2], &motor[2]);
    current_loop[2].motor_id = 2;

    CurrentLoop_SetPID(&current_loop[0], PID_KP_X, PID_KI_X, PID_KD_X);
    CurrentLoop_SetPID(&current_loop[1], PID_KP_Y, PID_KI_Y, PID_KD_Y);
    CurrentLoop_SetPID(&current_loop[2], PID_KP_Z, PID_KI_Z, PID_KD_Z);
    /* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    char buffer[50]; // 用于显示占空比的缓冲区
    while (1)
    {

        /* ---- 串口指令解析 ---- */
        PID_ParseState_t parse_state = PID_ParamParser_Parse(&current_pid_parse);
        if (parse_state == PARSE_STATE_COMPLETE)
        {
            ParsedCmd_t cmd = PID_ParamParser_GetCmd(&current_pid_parse);

            if (cmd.type == CMD_TYPE_RST)
            {
                /* RST：同时停止3路 */
                for (int i = 0; i < 3; i++)
                {
                    CurrentLoop_SetTarget(&current_loop[i], 0.0f);
                    CurrentLoop_Stop(&current_loop[i]);
                    closed_loop_active[i] = 0;
                }
            }
            else if (cmd.type == CMD_TYPE_TARGET)
            {
                /* X/Y/Z：分别设目标电流，没启动就自动启动 */
                CurrentLoop_SetTarget(&current_loop[0], cmd.target_x);
                CurrentLoop_SetTarget(&current_loop[1], cmd.target_y);
                CurrentLoop_SetTarget(&current_loop[2], cmd.target_z);
                for (int i = 0; i < 3; i++)
                {
                    if (CurrentLoop_GetState(&current_loop[i]) != CURRENT_LOOP_RUNNING)
                    {
                        CurrentLoop_Start(&current_loop[i]);
                        closed_loop_active[i] = 1;
                    }
                }
            }
            else if (cmd.type == CMD_TYPE_PID)
            {
                /* KP/KI/KD：同时应用到3路 */
                for (int i = 0; i < 3; i++)
                {
                    CurrentLoop_SetPID(&current_loop[i], cmd.kp, cmd.ki, cmd.kd);
                }
            }

            else if (cmd.type == CMD_TYPE_PID_AXIS)
            {
                /* PID_X/Y/Z:kp,ki,kd; 单独设置某一路PID */
                CurrentLoop_SetPID(&current_loop[cmd.pid_axis], cmd.kp, cmd.ki, cmd.kd);
            }

            PID_ParamParser_Reset(&current_pid_parse);
        }
        else if (parse_state == PARSE_STATE_ERROR)
        {
            PID_ParamParser_Reset(&current_pid_parse);
        }

        float current_x_ma = BSP_CurrentSensor_GetCurrent(0);
        float current_y_ma = BSP_CurrentSensor_GetCurrent(1);
        float current_z_ma = BSP_CurrentSensor_GetCurrent(2);

        
        /* ============ LCD显示：3路电流环数据 ============ */
        /* ---- 电机X (索引0) ---- */
        sprintf(buffer, "PWM_X:%.0f%%  ", PID_GetOutput(&current_loop[0].pid));
        lcd_show_string(10, 10, 300, 12, 12, buffer, RED);
        sprintf(buffer, "Tgt_X:%.0fmA  ", CurrentLoop_GetTarget(&current_loop[0]));
        lcd_show_string(10, 25, 300, 12, 12, buffer, BLUE);
        sprintf(buffer, "Cur_X:%.0fmA  ", current_x_ma);
        lcd_show_string(10, 40, 300, 12, 12, buffer, BLUE);

        sprintf(buffer, "ADC_X:%lu Off_X:%.0f",
                (unsigned long)adc_injected_data[0],
                BSP_CurrentSensor_GetOffsetById(0));
        lcd_show_string(100, 55, 300, 12, 12, buffer, BLUE);

        /* ---- 电机Y (索引1) ---- */
        sprintf(buffer, "PWM_Y:%.0f%%  ", PID_GetOutput(&current_loop[1].pid));
        lcd_show_string(10, 60, 300, 12, 12, buffer, RED);
        sprintf(buffer, "Tgt_Y:%.0fmA  ", CurrentLoop_GetTarget(&current_loop[1]));
        lcd_show_string(10, 75, 300, 12, 12, buffer, BLUE);
        sprintf(buffer, "Cur_Y:%.0fmA  ", current_y_ma);
        lcd_show_string(10, 90, 300, 12, 12, buffer, BLUE);

        sprintf(buffer, "ADC_Y:%lu Off_Y:%.0f",
                (unsigned long)adc_injected_data[1],
                BSP_CurrentSensor_GetOffsetById(1));
        lcd_show_string(100, 105, 300, 12, 12, buffer, BLUE);

        /* ---- 电机Z (索引2) ---- */
        sprintf(buffer, "PWM_Z:%.0f%%  ", PID_GetOutput(&current_loop[2].pid));
        lcd_show_string(10, 110, 300, 12, 12, buffer, RED);
        sprintf(buffer, "Tgt_Z:%.0fmA  ", CurrentLoop_GetTarget(&current_loop[2]));
        lcd_show_string(10, 125, 300, 12, 12, buffer, BLUE);
        sprintf(buffer, "Cur_Z:%.0fmA  ", current_z_ma);
        lcd_show_string(10, 140, 300, 12, 12, buffer, BLUE);

        sprintf(buffer, "ADC_Z:%lu Off_Z:%.0f",
                (unsigned long)adc_injected_data[2],
                BSP_CurrentSensor_GetOffsetById(2));
        lcd_show_string(100, 155, 300, 12, 12, buffer, BLUE);

        //        /* ---- 电压 ---- */
        //        sprintf(buffer, "V:%.1fV   ",
        //                BSP_VoltageSensor_GetPowerVoltage());
        //        lcd_show_string(10, 165, 300, 12, 12, buffer, BLUE);

        /* ---- 三路PID参数 ---- */
        sprintf(buffer, "X:Kp%.3f Ki%.3f Kd%.3f",
                current_loop[0].pid.Kp, current_loop[0].pid.Ki, current_loop[0].pid.Kd);
        lcd_show_string(10, 185, 400, 12, 12, buffer, BLUE);

        sprintf(buffer, "Y:Kp%.3f Ki%.3f Kd%.3f",
                current_loop[1].pid.Kp, current_loop[1].pid.Ki, current_loop[1].pid.Kd);
        lcd_show_string(10, 200, 400, 12, 12, buffer, BLUE);

        sprintf(buffer, "Z:Kp%.3f Ki%.3f Kd%.3f",
                current_loop[2].pid.Kp, current_loop[2].pid.Ki, current_loop[2].pid.Kd);
        lcd_show_string(10, 215, 400, 12, 12, buffer, BLUE);

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
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 8;
    RCC_OscInitStruct.PLL.PLLN = 336;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 7;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    /** Initializes the CPU, AHB and APB buses clocks
     */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
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
    if (htim->Instance == TIM6)
    {
        BSP_VoltageSensor_Update(); // 更新电压检测结果
        vofa_send_cnt++;
        if (vofa_send_cnt >= 5)
        {
            vofa_send_cnt = 0;
            VOFA_Plus_SendCurrentLoopData();
        }
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
