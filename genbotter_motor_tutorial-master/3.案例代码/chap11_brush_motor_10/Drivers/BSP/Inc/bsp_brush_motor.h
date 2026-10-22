/**
 * @file    bsp_brush_motor.h
 * @brief   直流有刷电机控制头文件
 * @author  Dr. GAO
 * @date    2026-05-26
 * @version V1.1
 * @website https://genbotter.taobao.com
 * @note    适用于GenBotter Motor-1电机开发板，使用PM1接口。
 *          TIM1 CH1 (PA8) 为PWM主输出，CH1N (PB13) 为互补输出，
 *          搭配IR2104半桥驱动实现正反转和启停。
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
    uint32_t          tim_channel; /* 定时器通道 */
    uint32_t          tim_arr;     /* 自动重装载值（Period+1），用于占空比计算 */
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

/* 全局方向变量：SetDirection 写，current_sensor 读，避免BSP模块间函数耦合 */
extern int8_t g_motor_current_direction;

#endif /* __BSP_BRUSH_MOTOR_H__ */
