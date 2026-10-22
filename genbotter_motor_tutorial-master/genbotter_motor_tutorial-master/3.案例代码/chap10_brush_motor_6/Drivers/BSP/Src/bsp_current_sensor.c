/**
 * @file    bsp_current_sensor.c
 * @brief   直流有刷电机电流检测源*.c文件
 * @author  Dr. GAO
 * @date    2025-12-06
 * @version V1.0
 * @website https://genbotter.taobao.com
 * @email   mailto:yanzenggao@163.com
 * @note    该文件适用于GenBotter Motor-1电机开发板, 直流有刷电机控制，使用电机开发板上的PM1(如果要使用PM2，请修改ADC通道配置)
 *          编译环境: 定时器、IO口等外设已经在CubeMX中配置完成，请确保项目正确配置
 */
#include "bsp_current_sensor.h"
#include "adc.h"  // 包含ADC全局变量（g_adc_val、hadc1等）
#include "tim.h"  // 如需定时器中断更新电流，根据实际情况包含

// 声明adc.c中定义的全局变量
extern uint16_t adc_raw_data[ADC_TOTAL_SAMPLES];    // 原始ADC采样值（单通道×采样次数）
extern float adc_filtered_data[ADC_CHANNEL_NUM];    // 滤波后的ADC平均值（浮点型）

// 静态变量：零漂偏移（浮点型，提高校准精度）
static float current_offset_adc = 0.0f;
// 当前电流值（单位：mA）
static float current_mA = 0.0f;

/**
 * @brief 初始化电流传感器（启动ADC DMA采集）
 * @note 需在系统初始化阶段调用
 */
void BSP_CurrentSensor_Init(void)
{
    current_offset_adc = 0.0f;
    current_mA = 0.0f;

    BSP_CurrentSensor_CalibrateOffset(); // 校准零漂

    // 启动ADC1的DMA采集的操作在main.c中完成，保证模块之间的低耦合
}

/**
 * @brief 校准电流传感器零漂（需在电机停止时执行）
 * @note 采集多次取平均，消除静态偏移误差
 */
void BSP_CurrentSensor_CalibrateOffset(void)
{
    float sum_offset = 0.0f;
    uint8_t calibrate_count = 0;

    // 多次采集滤波后的ADC值，取平均作为零漂
    while (calibrate_count < 10)
    {
        HAL_Delay(1); // 等待数据更新
        sum_offset += adc_filtered_data[0]; // 使用滤波后的浮点值
        calibrate_count++;
    }
    current_offset_adc = sum_offset / 10.0f; // 浮点平均，保留小数
}

/**
 * @brief 获取电流传感器原始ADC平均值
 * @retval ADC平均值（已通过adc.c的calc_adc_val滤波）
 */
float BSP_CurrentSensor_GetADCValue(void)
{
    return adc_filtered_data[0];  // 直接返回电流通道的ADC滤波后值
}

/**
 * @brief 更新电流计算（建议在定时器中断中周期性调用）
 * @note 基于校准偏移和转换因子计算实际电流
 */
void BSP_CurrentSensor_Update(void)
{
    float adc_raw;

    // 减去零漂偏移（浮点运算，保留精度）
    adc_raw = adc_filtered_data[0] - current_offset_adc;

    // 转换为电流值（mA）
    current_mA = adc_raw * ADC_TO_MA_FACTOR;
}

/**
 * @brief 获取当前电流值
 * @retval 电流值（单位：mA，带符号表示方向）
 */
float BSP_CurrentSensor_GetCurrent(void)
{
    return current_mA;
}

float BSP_CurrentSensor_GetOffset(void){
	return current_offset_adc;
}
