/**
 * @file    bsp_delay.h
 * @brief   微秒级延时函数头文件（基于DWT Cycle Counter）
 * @author  Dr. GAO
 * @date    2026-05-26
 * @version V1.0
 * @website https://genbotter.taobao.com
 * @note    该文件适用于GenBotter Motor-1电机开发板，STM32F407IGT6。
 *          使用Cortex-M4 DWT（Data Watchpoint and Trace）周期计数器实现微秒级延时，
 *          本案例中主要为LCD屏幕提供延时支持。
 */

#ifndef __DELAY_H
#define __DELAY_H

#ifdef __cplusplus
 extern "C" {
#endif

#include "main.h"

void DWT_Init(void);          /* 初始化DWT（需在SystemClock_Config之后调用） */
void delay_us(uint32_t us);   /* 微秒级延时函数 */

#ifdef __cplusplus
}
#endif

#endif /* __DELAY_H */
