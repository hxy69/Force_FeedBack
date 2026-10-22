/**
 * @file    bsp_voltage_sensor.c
 * @brief   直流有刷电机驱动板H桥供电电压检测源*.c文件
 * @author  Dr. GAO
 * @date    2025-12-06
 * @version V1.0
 * @website https://genbotter.taobao.com
 * @email   mailto:yanzenggao@163.com
 * @note    该文件适用于GenBotter Motor-1电机开发板, 直流有刷电机控制，使用电机开发板上的PM1(如果要使用PM2，请修改ADC通道配置)
 *          编译环境: 定时器、IO口等外设已经在CubeMX中配置完成，请确保项目正确配置
 */
#include "bsp_voltage_sensor.h"
#include "adc.h"

extern float adc_filtered_data[ADC_CHANNEL_NUM];

static float vbus = 0.0f;  // ADC1_IN9 的电压
static float power_voltage = 0.0f;  // 最终换算后的 POWER 电压

void BSP_VoltageSensor_Init(void)
{
    vbus = 0.0f;
    power_voltage = 0.0f;
}

void BSP_VoltageSensor_Update(void)
{
    float adc_val = adc_filtered_data[1]; // IN9 通道（ADC-IN9的500次采样平均值）

    // 计算 ADC 转换电压
    vbus = adc_val * (ADC_REF_VOLTAGE / ADC_RESOLUTION);

    // 根据电路关系计算 POWER 电压
    // POWER = 25 × vbus
    power_voltage = vbus * POWER_DIV_RATIO;
}

float BSP_VoltageSensor_GetADCValue(void)
{
    return adc_filtered_data[1];
}

float BSP_VoltageSensor_GetVBUS(void)
{
    return vbus;
}

float BSP_VoltageSensor_GetPowerVoltage(void)
{
    return power_voltage;
}
