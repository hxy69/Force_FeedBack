/**
 * @file    bsp_current_sensor.h
 * @brief   直流有刷电机电流检测头文件
 * @author  Dr. GAO
 * @date    2026-05-26
 * @version V1.1
 * @website https://genbotter.taobao.com
 * @note    适用于GenBotter Motor-1电机开发板（PM1接口）。
 *          使用ADC1 IN8（PB0）采集电流传感器输出电压，
 *          由TIM1 TRGO（Update Event）上升沿触发ADC注入通道转换，
 *          在注入中断回调中完成电流计算和低通滤波。
 */
#ifndef __BSP_CURRENT_SENSOR_H
#define __BSP_CURRENT_SENSOR_H

#include "main.h"

/* 电流转换因子（ADC阶数 → 电流mA）
 * 推导：3.3V参考 / 4096阶 / 0.12V/A灵敏度 × 1000 ≈ 6.714
 * 实际值需根据硬件传感器灵敏度校准 */
#define ADC_TO_MA_FACTOR 6.714f

/* 函数声明 */
void     BSP_CurrentSensor_Init(void);
void     BSP_CurrentSensor_CalibrateOffset(void);
uint32_t BSP_CurrentSensor_GetADCValue(void);
float    BSP_CurrentSensor_GetOffset(void);
float    BSP_CurrentSensor_GetCurrent(void);
void     BSP_CurrentSensor_Update(void);

#endif
