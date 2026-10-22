/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2026-07-29 14:07:08
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2026-08-03 17:42:08
 * @FilePath: \Force_feedback\Drivers\BSP\Inc\brush_motor.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef BRUSH_MOTOR_H
#define BRUSH_MOTOR_H

#include "main.h"

typedef struct
{
    TIM_HandleTypeDef *htim;       // PWM定时器句柄
    uint32_t tim_channel;          // PWM通道（TIM_CHANNEL_1/2/3）
    uint32_t tim_arr;              // 定时器自动重装载值
    GPIO_TypeDef *ctrl_port;       // SD控制引脚端口
    uint16_t ctrl_pin;             // SD控制引脚
    int8_t current_direction;      // 新增：1=正向, -1=反向（替代全局变量）
} BrushMotorConfig;

typedef enum
{
    MOTOR_FORWARD = 0,  // 正转
    MOTOR_REVERSE = 1,  // 反转
    MOTOR_STOP = 2      // 停止
} MotorDirection;

void BrushMotor_Init(BrushMotorConfig *config);
void BrushMotor_Enable(BrushMotorConfig *motor);
void BrushMotor_Disable(BrushMotorConfig *motor);
void BrushMotor_Stop(BrushMotorConfig *motor);
void BrushMotor_SetSpeed(BrushMotorConfig *motor, uint8_t speed);
void BrushMotor_SetDirection(BrushMotorConfig *motor, MotorDirection direction);
void BrushMotor_StartPWM(BrushMotorConfig *motor);
void BrushMotor_BootstrapCharge(BrushMotorConfig *motor);


#endif
