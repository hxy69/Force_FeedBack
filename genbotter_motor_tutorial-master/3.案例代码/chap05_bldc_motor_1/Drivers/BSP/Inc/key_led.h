#ifndef __KEY_LED_H
#define __KEY_LED_H

#include "main.h"

typedef enum {
    KEY0_Pressed = 1,
    KEY1_Pressed,
    KEY2_Pressed,
    KEY_None
}KeyPressedID;

/* 按键初始化函数声明 */
void Key_Init(void);
/* 按键扫描函数，返回按下的按键值，可根据实际需求定义返回类型和值 */
uint8_t Key_Scan(void);

/* LED 初始化函数声明 */
void Led_Init(void);
/* 控制 LED 点亮函数 */
void Led_On(uint16_t led_pin);
/* 控制 LED 熄灭函数 */
void Led_Off(uint16_t led_pin);
/* 控制 LED 翻转函数 */
void Led_Toggle(uint16_t led_pin);

#endif
