/**
 * @file    bsp_brush_motor.h
 * @brief   直流有刷电机控制头文件
 * @author  Dr. GAO
 * @date    2025-11-07
 * @version V1.0
 * @website https://genbotter.taobao.com
 * @email   mailto:yanzenggao@163.com
 * @note    该文件适用于GenBotter Motor-1电机开发板, 直流有刷电机控制，使用电机开发板上的PM1或PM2
 *          编译环境: 定时器、IO口等外设已经在CubeMX中配置完成，请确保项目正确配置
 */
#ifndef __BSP_BRUSH_MOTOR_H__
#define __BSP_BRUSH_MOTOR_H__

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


#endif /* __BSP_BRUSH_MOTOR_H__ */
