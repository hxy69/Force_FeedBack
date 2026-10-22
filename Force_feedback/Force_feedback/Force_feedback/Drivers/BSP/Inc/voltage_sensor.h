/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2026-07-30 16:39:02
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2026-07-30 16:45:11
 * @FilePath: \Force_feedback\Drivers\BSP\Inc\voltage_sensor.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef __VOLTAGE_SENSOR_H
#define __VOLTAGE_SENSOR_H

#include "main.h"

#define ADC_REF_VOLTAGE   3.3f
#define ADC_RESOLUTION    4096.0f

// 分压网络： 12k + 12k + 1k → 采样点 = POWER/25
#define POWER_DIV_RATIO   25.0f

void BSP_VoltageSensor_Init(void);          // 初始化
void BSP_VoltageSensor_Update(void);        // 更新电压值

float BSP_VoltageSensor_GetADCValue(void);    // 获取ADC1_IN9平均值
float BSP_VoltageSensor_GetVBUS(void);        // 获取VBUS电压值
float BSP_VoltageSensor_GetPowerVoltage(void);// 获取最终换算后的POWER电压值

#endif
