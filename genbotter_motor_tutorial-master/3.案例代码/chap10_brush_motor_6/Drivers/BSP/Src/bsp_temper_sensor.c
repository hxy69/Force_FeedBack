/**
 * @file    bsp_temper_sensor.c
 * @brief   直流有刷电机驱动板温度检测源*.c文件
 * @author  Dr. GAO
 * @date    2025-12-06
 * @version V1.0
 * @website https://genbotter.taobao.com
 * @email   mailto:yanzenggao@163.com
 * @note    该文件适用于GenBotter Motor-1电机开发板, 直流有刷电机控制，使用电机开发板上的PM1(如果要使用PM2，请修改ADC通道配置)
 *          编译环境: 定时器、IO口等外设已经在CubeMX中配置完成，请确保项目正确配置
 */

 #include "bsp_temper_sensor.h"
#include "adc.h"
#include <math.h>

extern float adc_filtered_data[ADC_CHANNEL_NUM];

/*==================== 内部变量 ====================*/

static float temp_adc_value = 0.0f;   // ADC 500次平均值
static float temp_voltage   = 0.0f;   // 分压电压
static float temp_value     = 0.0f;   // 温度（℃）
static uint16_t update_counter = 0;           // 调用计数器

/*==================== 接口实现 ====================*/

void BSP_TemperSensor_Init(void)
{
    temp_adc_value = 0.0f;
    temp_voltage   = 0.0f;
    temp_value     = 0.0f;
    update_counter = 0;
}

void BSP_TemperSensor_Update(void)
{
    /* 每次调用计数器加1 */
    update_counter++;
    
    /* 达到指定间隔才执行实际计算 */
    if (update_counter >= UPDATE_INTERVAL)
    {
        BSP_TemperSensor_Calculate();
        update_counter = 0;  // 重置计数器
    }
}

void BSP_TemperSensor_Calculate(void)
{
    float r_ntc;
    float temp_k;

    /* 1. 直接读取 ADC 滤波后的数据 */
    temp_adc_value = adc_filtered_data[TEMP_ADC_CHANNEL_INDEX];

    /* 防止异常值 */
    if (temp_adc_value < 1.0f)
    {
        temp_adc_value = 1.0f;
    }

    /* 2. ADC → 电压 */
    temp_voltage = temp_adc_value * (TEMP_ADC_REF_VOLTAGE / TEMP_ADC_RESOLUTION);

    /* 3. 电压 → NTC 阻值
     * Rntc = Rpull × (Vref / V - 1)
     */
    r_ntc = NTC_PULL_DOWN_RESISTOR *
            (TEMP_ADC_REF_VOLTAGE / temp_voltage - 1.0f);

    /* 4. NTC → 温度（B 参数公式） */
    temp_k = 1.0f /
            ( (1.0f / NTC_T0)
            + (1.0f / NTC_BETA) * logf(r_ntc / NTC_R0) );

    temp_value = temp_k - 273.15f;
}

float BSP_TemperSensor_GetADCValue(void)
{
    return temp_adc_value;
}

float BSP_TemperSensor_GetVoltage(void)
{
    return temp_voltage;
}

float BSP_TemperSensor_GetTemperature(void)
{
    return temp_value;
}
