/**
 * @file brush_motor.h
 * @brief Brush motor control header file直流有刷电机控制头文件
 * @version 1.0
 * @date 2025-07-25
 * @author Dr. GAO
 */
#ifndef BRUSH_MOTOR_H
#define BRUSH_MOTOR_H

#include "main.h"

typedef enum {
    MOTOR_FORWARD = 0, // 正转
    MOTOR_REVERSE = 1,  // 反转
    MOTOR_STOP = 2      // 停止
} MotorDirection;

typedef struct
{
    TIM_HandleTypeDef *htim; // 定时器句柄
    uint32_t tim_channel;    // 定时器通道
    uint32_t tim_arr;        // 定时器自动重装载值
    GPIO_TypeDef *sd_port;
    uint16_t sd_pin; // IR2104使能引脚
} BrushMotorConfig;

void BrushMotor_Init(BrushMotorConfig *config);
void BrushMotor_Enable(void);
void BrushMotor_Stop(void);
void BrushMotor_SetSpeed(uint8_t speed); // 设置电机速度0-100,0停止，100全速
void BrushMotor_SetDirection(MotorDirection direction); // 设置电机转向

#endif // BRUSH_MOTOR_H
