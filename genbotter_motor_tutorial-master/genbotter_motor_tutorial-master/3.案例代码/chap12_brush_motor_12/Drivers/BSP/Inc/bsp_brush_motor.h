/**
 * @file    bsp_brush_motor.h
 * @brief   直流有刷电机控制头文件
 * @author  Dr. GAO
 * @date    2026-05-26
 * @version V1.2
 * @website https://genbotter.taobao.com
 * @note    适用于GenBotter Motor-1电机开发板，使用PM1接口控制直流有刷电机。
 *          电机驱动采用双IR2104半桥组成全H桥架构：
 *            - TIM1 CH1  (PA8)  → 半桥A HIN
 *            - TIM1 CH1N (PB13) → 半桥B HIN
 *          （每路IR2104的LIN由板载反相器从HIN生成，实现自动互补）
 *          正转时CH1输出PWM、CH1N关闭；反转时CH1关闭、CH1N输出PWM。
 */
#ifndef __BSP_BRUSH_MOTOR_H__
#define __BSP_BRUSH_MOTOR_H__

#include "main.h"

/* 电机转向枚举 */
typedef enum {
    MOTOR_FORWARD = 0,  /* 正转：CH1输出PWM，CH1N关闭 */
    MOTOR_REVERSE = 1,  /* 反转：CH1关闭，CH1N输出PWM */
    MOTOR_STOP    = 2   /* 停止：两路均关闭 */
} MotorDirection;

/* 电机配置结构体 */
typedef struct
{
    TIM_HandleTypeDef *htim;    /* 定时器句柄 */
    uint32_t          tim_channel;    /* 定时器通道 */
    uint32_t          tim_arr_plus1;  /* ARR+1（PWM 分辨率 0~1000），用于占空比计算 */
    GPIO_TypeDef     *sd_port;     /* IR2104 SD引脚端口 */
    uint16_t          sd_pin;      /* IR2104 SD引脚编号 */
} BrushMotorConfig;

/* 函数声明 */
void BrushMotor_Init(BrushMotorConfig *config);
void BrushMotor_Enable(void);
void BrushMotor_Disable(void);
void BrushMotor_Stop(void);
void BrushMotor_SetSpeed(uint8_t speed);
void BrushMotor_SetDirection(MotorDirection direction);
void BrushMotor_StartPWM(void);
void BrushMotor_BootstrapCharge(void);

#endif /* __BSP_BRUSH_MOTOR_H__ */
