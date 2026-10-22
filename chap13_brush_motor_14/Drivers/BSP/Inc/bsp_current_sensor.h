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

/* 注：ADC_TO_MA_FACTOR、CURRENT_LOWPASS_ALPHA 等参数统一在 user_config.h 中定义 */

/* 函数声明 */
void     BSP_CurrentSensor_Init(void);
void     BSP_CurrentSensor_CalibrateOffset(void);
uint32_t BSP_CurrentSensor_GetADCValue(void);
float    BSP_CurrentSensor_GetOffset(void);
float    BSP_CurrentSensor_GetCurrent(void);
void     BSP_CurrentSensor_Update(void);

#endif
