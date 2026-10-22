/**
 * @file    bsp_key_led.h
 * @brief   延时函数头文件
 * @author  Dr. GAO
 * @date    2025-10-06
 * @version V1.0
 * @website https://genbotter.taobao.com
 * @email   mailto:yanzenggao@163.com
 * @note    该文件适用于GenBotter Motor-1电机开发板, 且使用DWT实现微秒级延时, STM32F407IGT6，本案例中给LCD屏幕提供延时支持。
 */
#ifndef __KEY_LED_H
#define __KEY_LED_H

#include "main.h"

typedef enum {
    KEY0_Pressed = 1,
    KEY1_Pressed,
    KEY2_Pressed,
    KEY_None
}KeyPressedID;

typedef enum {
    LED1 = 0,
    LED2
}LED_ID;

/* 按键初始化函数声明 */
void Key_Init(void);
/* 按键扫描函数，返回按下的按键值，可根据实际需求定义返回类型和值 */
KeyPressedID Key_Scan(void);

/* LED 初始化函数声明 */
void Led_Init(void);
/* 控制 LED 点亮函数 */
void Led_On(LED_ID led_id);
/* 控制 LED 熄灭函数 */
void Led_Off(LED_ID led_id);
/* 控制 LED 翻转函数 */
void Led_Toggle(LED_ID led_id);

#endif
