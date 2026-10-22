/**
 * @file    bsp_key_led.h
 * @brief   按键扫描与LED控制头文件
 * @author  Dr. GAO
 * @date    2026-05-26
 * @version V1.0
 * @website https://genbotter.taobao.com
 * @note    该文件适用于GenBotter Motor-1电机开发板，STM32F407IGT6。
 *          按键：KEY0(PE2)、KEY1(PE3)、KEY2(PE4)，低电平有效，轮询扫描+消抖。
 *          LED：LED1(PE0)、LED2(PE1)，低电平点亮。
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
/* 按键扫描函数（10ms 消抖 + 松手检测） */
uint8_t Key_Scan(uint8_t mode);

/* LED 初始化函数声明 */
void Led_Init(void);
/* 控制 LED 点亮函数 */
void Led_On(LED_ID led_id);
/* 控制 LED 熄灭函数 */
void Led_Off(LED_ID led_id);
/* 控制 LED 翻转函数 */
void Led_Toggle(LED_ID led_id);

#endif
