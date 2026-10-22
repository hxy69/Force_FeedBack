#ifndef __VOFA_PLUS_H__
#define __VOFA_PLUS_H__

#include "main.h"
#include "speed_loop.h" // 引入速度环头文件，获取调试数据
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* ==================== 核心宏开关 ==================== */
// 开启/关闭VOFA+调试功能：调试时设1，量产时设0
#define VOFA_PLUS_ENABLE        1

#if VOFA_PLUS_ENABLE

/* ==================== 硬件配置（根据工程实际修改） ==================== */
// 1. 选择VOFA使用的串口句柄（需匹配你实际使用的串口，如STM32的huart1）
#define VOFA_UART_HANDLE        huart1  
// 2. VOFA协议选择：1=FireWater（字符串，易调试），2=RawData
#define VOFA_PROTOCOL 1  // 默认选FireWater，改2即可切换为RawData（暂未实现）
// 3. 定义发送缓冲区长度（根据数据量调整，至少32字节）
#define VOFA_SEND_BUF_MAX_LEN   64

// 复用SpeedLoop中的SpeedLoop_DebugData_t结构体作为VOFA+数据载体，避免重复定义
typedef SpeedLoop_DebugData_t VOFA_Data_t; // VOFA+数据结构与SpeedLoop调试数据结构相同

/* ==================== 函数声明 ==================== */
/**
 * @brief  VOFA+模块初始化
 * @note   串口初始化建议在main.c中统一做，此处仅做空实现（避免重复初始化）
 */
void VOFA_Plus_Init(void);

/**
 * @brief 封装VOFA+待发送数据（FireWater协议）
 * @param vofa_data: 待发送的数据结构体
 * @param send_buf: 输出缓冲区
 * @param buf_len: 缓冲区长度
 * @retval 封装后的数据长度
 */
uint16_t VOFA_Plus_PackData(VOFA_Data_t *vofa_data, uint8_t *send_buf, uint16_t buf_len);

/**
 * @brief  VOFA+模块发送数据到串口（封装上层接口，简化业务代码调用）
 * @note   无需手动传参，该函数可在speed_loop.c的主循环或定时器回调中调用，确保实时性
 */
void VOFA_Plus_SendData(VOFA_Data_t *data);

/**
 * @brief  从speed_loop.c获取当前数据并发送到VOFA+(封装上层接口，简化业务代码调用)
 * @note   无需手动传参，该函数可在speed_loop.c的主循环或定时器回调中调用，确保实时性
 */
void VOFA_Plus_SendSpeedLoopData(void);

#else
/* ==================== 空实现（当VOFA+功能关闭时，避免编译错误） ==================== */
#define VOFA_Plus_Init() ((void)0)
#define VOFA_Plus_PackData(vofa_data, send_buf, buf_len) ((uint16_t)0)
#define VOFA_Plus_SendData(data) ((void)0)
#define VOFA_Plus_SendSpeedLoopData() ((void)0)

#endif // VOFA_PLUS_ENABLE

#endif /* __VOFA_PLUS_H__ */
