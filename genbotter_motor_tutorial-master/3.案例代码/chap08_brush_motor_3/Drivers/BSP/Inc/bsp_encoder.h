/**
 * @file    bsp_encoder.h
 * @brief   直流有刷电机编码器测速头文件
 * @author  Dr. GAO
 * @date    2025-11-07
 * @version V1.0
 * @website https://genbotter.taobao.com
 * @email   mailto:yanzenggao@163.com
 * @note    该文件适用于GenBotter Motor-1电机开发板, 直流有刷电机控制，使用电机开发板上的PM1或PM2
 *          编译环境: 定时器、IO口等外设已经在CubeMX中配置完成，请确保项目正确配置
 */
#ifndef __BSP_ENCODER_H
#define __BSP_ENCODER_H

#include "main.h"

// 编码器配置
#define ENCODER_PPR        52       // 编码器每转脉冲数（4倍频后的值）
#define GEAR_RATIO         30.0f    // 减速比
#define SPEED_UPDATE_MS    100       // 速度更新周期(ms)

// 编码器编号
typedef enum {
    ENCODER_PM1 = 0,  // PM1接口，使用TIM3
    ENCODER_PM2 = 1,  // PM2接口，使用TIM2  
} Encoder_ID_t;

// 函数声明
void BSP_Encoder_Init(void);                                // 初始化编码器模块
void BSP_Encoder_Start(Encoder_ID_t encoder_id);           // 启动指定编码器计数
void BSP_Encoder_Stop(Encoder_ID_t encoder_id);            // 停止指定编码器计数
void BSP_Encoder_StartAll(void);                           // 启动所有编码器
void BSP_Encoder_StopAll(void);                            // 停止所有编码器
int32_t BSP_Encoder_GetCount(Encoder_ID_t encoder_id);     // 获取编码器计数
float BSP_Encoder_GetSpeedRPM(Encoder_ID_t encoder_id);    // 获取转速(RPM)
void BSP_Encoder_HandleOverflow(TIM_HandleTypeDef *htim);  // 处理编码器溢出
void BSP_Encoder_UpdateSpeed(void);                        // 更新转速计算
void BSP_Encoder_Reset(Encoder_ID_t encoder_id);           // 重置编码器计数
uint8_t BSP_Encoder_IsAvailable(Encoder_ID_t encoder_id);  // 检查编码器是否可用
uint8_t BSP_Encoder_IsRunning(Encoder_ID_t encoder_id);    // 检查编码器是否正在运行

#endif
