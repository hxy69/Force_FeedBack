/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2026-07-30 13:25:24
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2026-08-26 18:02:04
 * @FilePath: \MDK-ARMc:\Users\Administrator\Desktop\Force_FeedBack\Force_feedback\Drivers\BSP\Inc\current_sensor.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef __CURRENT_SENSOR_H
#define __CURRENT_SENSOR_H

#include "main.h"

// 电流转换因子（ADC值 -> 电流值(mA)）
// 推导：3.3V参考电压，12位ADC(4096阶)，传感器灵敏度0.12V/A（示例，需按硬件修改）
#define CURRENT_ADC_REF_VOLTAGE 3.3f     // ADC参考电压(V)
#define CURRENT_ADC_RESOLUTION 4096.0f   // 12位ADC的最大值
#define CURRENT_SENSOR_SENSITIVITY 0.12f // 传感器灵敏度(V/A)，需按硬件手册修改

// 函数声明
void BSP_CurrentSensor_Init(void);            // 初始化
void BSP_CurrentSensor_CalibrateOffset(void); // 校准零漂
float BSP_CurrentSensor_GetOffsetById(uint8_t id);
float BSP_CurrentSensor_GetCurrent(uint8_t id); // 获取当前电流值(mA)
void BSP_CurrentSensor_Update(void);      // 更新电流值

#endif
