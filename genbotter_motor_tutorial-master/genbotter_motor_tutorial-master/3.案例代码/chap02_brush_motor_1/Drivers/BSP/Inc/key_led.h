#ifndef __KEY_LED_H
#define __KEY_LED_H

#include "main.h"

/* 初始化按键的函数声明 */
void KEY_Init(void);
/* 初始化LED的函数声明 */
void LED_Init(void);

/* 控制LED点亮函数  */
void LED_On(uint16_t led_pin);
/* 控制LED熄灭函数 */
void LED_Off(uint16_t led_pin);
/* 控制LED翻转函数 */
void LED_Toggle(uint16_t led_pin);

/* 按键扫描函数 */
uint8_t KEY_Scan(void);

#endif
