/**
 * @file    bsp_voltage_sensor.h
 * @brief   直流有刷电机供电电压检测头文件
 * @author  Dr. GAO
 * @date    2025-12-12
 * @version V1.0
 * @website https://genbotter.taobao.com
 * @email   mailto:yanzenggao@163.com
 * @note    该文件适用于GenBotter Motor-1电机开发板, 直流有刷电机控制，使用电机开发板上的PM1
 *          若使用PM2，请修改ADC通道及其对应IO口配置
 *          编译环境: 定时器、IO口等外设已经在CubeMX中配置完成，请确保项目正确配置
 */
#ifndef __BSP_VOLTAGE_SENSOR_H
#define __BSP_VOLTAGE_SENSOR_H

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
