/**
 * @file    bsp_temper_sensor.h
 * @brief   直流有刷电机驱动板温度检测头文件
 * @author  Dr. GAO
 * @date    2025-12-20
 * @version V1.0
 * @website https://genbotter.taobao.com
 * @email   mailto:yanzenggao@163.com
 * @note    该文件适用于GenBotter Motor-1电机开发板, 直流有刷电机控制，使用电机开发板上的PM1
 *          若使用PM2，请修改ADC通道及其对应IO口配置
 *          编译环境: 定时器、IO口等外设已经在CubeMX中配置完成，请确保项目正确配置
 */
#ifndef __BSP_TEMPER_SENSOR_H
#define __BSP_TEMPER_SENSOR_H

#ifdef __cplusplus
extern "C" {
#endif

/*==================== 宏定义 ====================*/

/* ADC 滤波后的通道索引 */
#define TEMP_ADC_CHANNEL_INDEX     2          // ADC1_IN0, 使用第3个rank拿到的数据

/* ADC 参数 */
#define TEMP_ADC_REF_VOLTAGE       3.3f
#define TEMP_ADC_RESOLUTION        4095.0f

/* NTC 参数（NCP18XH103F03RB） */
#define NTC_PULL_DOWN_RESISTOR     4700.0f     // 4.7k
#define NTC_R0                     10000.0f    // 10k @ 25℃
#define NTC_T0                     298.15f     // 25℃
#define NTC_BETA                   3435.0f     // B 值

/* 温度更新间隔, 因为温度并不会跳变太快 */
#define UPDATE_INTERVAL            5         // 每5次调用才更新一次（0.5秒更新一次）

/*==================== 接口函数 ====================*/

void  BSP_TemperSensor_Init(void);
void  BSP_TemperSensor_Update(void);

float BSP_TemperSensor_GetADCValue(void);     // 获取ADC-IN0平均值
float BSP_TemperSensor_GetVoltage(void);      // 获取4.1K分压电压
float BSP_TemperSensor_GetTemperature(void);  // 获取温度(单位：摄氏度)

/*==================== 内部函数声明 ====================*/
static void BSP_TemperSensor_Calculate(void);     // 实际计算函数

#ifdef __cplusplus
}
#endif

#endif /* __BSP_TEMPER_SENSOR_H */
