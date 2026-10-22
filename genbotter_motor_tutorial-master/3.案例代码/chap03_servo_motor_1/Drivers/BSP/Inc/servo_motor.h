/**
 * @file servo_motor.h
 * @brief Header file for servo motor control functions.
 * 专门针对GenBotter Motor-1上的三个舵机控制接口的驱动.
 * @version 1.0
 * @date 2025-08-05
 */

#ifndef SERVO_MOTOR_H
#define SERVO_MOTOR_H

#include "main.h"

#define SERVO_MAX_ANGLE 180 // 舵机最大角度
#define SERVO_MIN_ANGLE 0   // 舵机最小角度

typedef enum {
    SERVO_1 = 1, // 舵机1
    SERVO_2,     // 舵机2
    SERVO_3      // 舵机3
}ServoID;

void ServoMotor_Init(void); //配置舵机初始角度;;

// 获取指定角度对应的比较值
uint16_t ServoMotor_GetCompareValue(uint8_t angle);
void ServoMotor_SetAngle(ServoID servo_id, uint8_t angle); //设置指定舵机的角度

#endif // SERVO_MOTOR_H
