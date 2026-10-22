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
