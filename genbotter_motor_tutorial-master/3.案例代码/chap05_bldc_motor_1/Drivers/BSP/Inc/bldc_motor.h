#ifndef __BLDC_MOTOR_H
#define __BLDC_MOTOR_H

#include "main.h"

// 电机旋转方向枚举
typedef enum {
    BLDC_CW = 0, // 顺时针
    BLDC_CCW = 1 // 逆时针
}BLDC_Dir_t;

// 电机状态枚举
typedef enum {
    BLDC_STOPPED = 0, // 停止
    BLDC_STEP,        // 单步换相转动
    BLDC_RUN          // 持续转动
}BLDC_State_t;

// 初始化BLDC
void BLDC_Init(TIM_HandleTypeDef *htim);

// 设置状态
void BLDC_SetState(BLDC_State_t state, BLDC_Dir_t dir);

// 获取状态
BLDC_State_t BLDC_GetState(void);
BLDC_Dir_t BLDC_GetDir(void);

// 单步换相
void BLDC_StepOnce(void);

// 设置连续旋转的单步换相间隔时间，防止转的太快, 单位ms
void BLDC_SetStepInterval(uint32_t interval);
//定时器中断中调用的函数
void BLDC_TickHandler(void);
// 内部函数：配置某一步的通道
void BLDC_SetStep(uint8_t step);

#endif
