/**
 * @file servo_motor.c
 * @brief Implementation of servo motor control functions.
 * 专门针对GenBotter Motor-1上的三个舵机控制接口的驱动.
 * @version 1.0
 * @date 2025-08-05
 */
#include "servo_motor.h"

extern TIM_HandleTypeDef htim8; // 定时器句柄

void ServoMotor_Init(void)
{
    // 开启TIM8的ch1、ch2、ch3的PWM输出
    HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_3);

    // 设置初始角度为90度
    ServoMotor_SetAngle(SERVO_1, 90);
    ServoMotor_SetAngle(SERVO_2, 90);
    ServoMotor_SetAngle(SERVO_3, 90);
}; // 配置舵机初始角度;;

// 获取指定角度对应的比较值
uint16_t ServoMotor_GetCompareValue(uint8_t angle) {

    // 确保角度在合法范围内
    if (angle <= SERVO_MIN_ANGLE) angle = SERVO_MIN_ANGLE;
    if (angle > SERVO_MAX_ANGLE) angle = SERVO_MAX_ANGLE;

    // 将角度转换为比较值
    // 计算公式: 比较值 = (角度 / 最大角度) * 最大比较值
    // 最大比较值为19999（对应于TIM8的Period）
    return (uint16_t)(500 + (angle * 100)/9);

};

/**
 * @brief 设置指定舵机的角度
 * @param servo_id 舵机ID
 * @param angle 角度值
 * @note 角度范围为0到180度
 */
void ServoMotor_SetAngle(ServoID servo_id, uint8_t angle)
{

    if (angle < SERVO_MIN_ANGLE || angle > SERVO_MAX_ANGLE)
    {
        // 如果角度不在范围内，返回
        return;
    }

    // 将角度angle转换为比较值
    uint16_t compare_value = ServoMotor_GetCompareValue(angle);
    switch (servo_id)
    {
    case SERVO_1:
        __HAL_TIM_SET_COMPARE(&htim8, TIM_CHANNEL_1, compare_value);
        break;
    case SERVO_2:
        __HAL_TIM_SET_COMPARE(&htim8, TIM_CHANNEL_2, compare_value);
        break;
    case SERVO_3:
        __HAL_TIM_SET_COMPARE(&htim8, TIM_CHANNEL_3, compare_value);
        break;
    default:
        // 如果传入的舵机ID不合法，返回
        return;

    }; // 设置指定舵机的角度
}
