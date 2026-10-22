/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2026-07-30 16:38:23
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2026-08-28 08:57:17
 * @FilePath: \MDK-ARMc:\Users\Administrator\Desktop\Force_feedback\Drivers\BSP\Src\voltage_sensor.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "voltage_sensor.h"
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
    float adc_val = adc_filtered_data[0]; // IN9 通道（ADC-IN9的500次采样平均值）

    // 计算 ADC 转换电压
    vbus = adc_val * (ADC_REF_VOLTAGE / ADC_RESOLUTION);

    // 根据电路关系计算 POWER 电压
    // POWER = 25 × vbus
    power_voltage = vbus * POWER_DIV_RATIO;
}

float BSP_VoltageSensor_GetADCValue(void)
{
    return adc_filtered_data[0];
}

float BSP_VoltageSensor_GetVBUS(void)
{
    return vbus;
}

float BSP_VoltageSensor_GetPowerVoltage(void)
{
    return power_voltage;
}
