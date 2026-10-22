/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2026-07-29 13:31:04
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2026-07-29 13:50:25
 * @FilePath: \Force_feedback\Drivers\BSP\Inc\key_led.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef __KEY_LED_H
#define __KEY_LED_H

#include "main.h"

void KEY_Init(void);
void LED_Init(void);

void LED_On(uint8_t led_pin);
void LED_Off(uint8_t led_pin);
void LED_Toggle(uint8_t led_pin);

/* 按键扫描 */
uint8_t KEY_Scan(void);

#endif
