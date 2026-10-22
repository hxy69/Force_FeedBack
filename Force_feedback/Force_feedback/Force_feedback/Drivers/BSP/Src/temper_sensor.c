/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2026-07-30 17:40:11
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2026-08-27 17:36:00
 * @FilePath: \MDK-ARMc:\Users\Administrator\Desktop\Force_FeedBack\Force_feedback\Drivers\BSP\Src\temper_sensor.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "temper_sensor.h"
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
