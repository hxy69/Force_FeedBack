/**
 * @file    bsp_current_sensor.h
 * @brief   直流有刷电机电流检测头文件
 * @author  Dr. GAO
 * @date    2025-12-06
 * @version V1.0
 * @website https://genbotter.taobao.com
 * @email   mailto:yanzenggao@163.com
 * @note    该文件适用于GenBotter Motor-1电机开发板, 直流有刷电机控制，使用电机开发板上的PM1或PM2
 *          编译环境: 定时器、IO口等外设已经在CubeMX中配置完成，请确保项目正确配置
 */
#ifndef __BSP_CURRENT_SENSOR_H
#define __BSP_CURRENT_SENSOR_H

#include "main.h"

// 电流转换因子（ADC值 -> 电流值(mA)）
// 推导：3.3V参考电压，12位ADC(4096阶)，传感器灵敏度0.12V/A（示例，需按硬件修改）
#define CURRENT_ADC_REF_VOLTAGE 3.3f     // ADC参考电压(V)
#define CURRENT_ADC_RESOLUTION 4096.0f   // 12位ADC的最大值
#define CURRENT_SENSOR_SENSITIVITY 0.12f // 传感器灵敏度(V/A)，需按硬件手册修改

// 转换公式：电流(mA) = (ADC值对应的电压 - 零漂电压) / 灵敏度 * 1000
// #define ADC_TO_MA_FACTOR ((CURRENT_ADC_REF_VOLTAGE / CURRENT_ADC_RESOLUTION) / CURRENT_SENSOR_SENSITIVITY * 1000.0f )
#define ADC_TO_MA_FACTOR 6.714f

// 函数声明
void BSP_CurrentSensor_Init(void);  // 初始化
void BSP_CurrentSensor_CalibrateOffset(void); // 校准零漂

float BSP_CurrentSensor_GetADCValue(void);// 获取ADC平均值
float BSP_CurrentSensor_GetOffset(void);  // 获取零漂值

float BSP_CurrentSensor_GetCurrent(void); // 获取当前电流值(mA)
void BSP_CurrentSensor_Update(void);      // 更新电流值

#endif
