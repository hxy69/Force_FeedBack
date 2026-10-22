/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2026-07-29 13:31:16
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2026-07-29 14:55:46
 * @FilePath: \Force_feedback\Drivers\BSP\Src\key_led.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "key_led.h"

void KEY_Init(void)
{

}

/**
 * @brief  LED初始化
 * @param  led_pin: LED引脚
 */
void LED_On(uint8_t led_pin)
{
    HAL_GPIO_WritePin(GPIOE, led_pin, GPIO_PIN_SET);
}

/**
 * @brief  LED关闭
 * @param  led_pin: LED引脚
 */
void LED_Off(uint8_t led_pin)
{
    HAL_GPIO_WritePin(GPIOE, led_pin, GPIO_PIN_RESET);
}

/**
 * @brief  LED翻转
 * @param  led_pin: LED引脚
 */
void LED_Toggle(uint8_t led_pin)
{
    HAL_GPIO_TogglePin(GPIOE, led_pin);
}

/**
 * @brief  按键扫描
 * @return uint8_t: 按键状态
 * 0: 无按键按下
 * 1: KEY1按下
 * 2: KEY2按下
 * 3: KEY3按下
 */
uint8_t KEY_Scan(void)
{
    if(HAL_GPIO_ReadPin(KEY0_GPIO_Port, KEY0_Pin) == GPIO_PIN_RESET)
    {
        HAL_Delay(10); // 消抖
        if(HAL_GPIO_ReadPin(KEY0_GPIO_Port, KEY0_Pin) == GPIO_PIN_RESET)
        {
            while(HAL_GPIO_ReadPin(KEY0_GPIO_Port, KEY0_Pin) == GPIO_PIN_RESET); // 等待按键释放
            return 1; // KEY0按下
        }
    }

    if(HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin) == GPIO_PIN_RESET)
    {
        HAL_Delay(10); // 消抖
        if(HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin) == GPIO_PIN_RESET)
        {
            while(HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin) == GPIO_PIN_RESET); // 等待按键释放
            return 2; // KEY1按下
        }
    }

    if (HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin) == GPIO_PIN_RESET)
    {
        HAL_Delay(10); // 消抖
        if(HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin) == GPIO_PIN_RESET)
        {
            while(HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin) == GPIO_PIN_RESET); // 等待按键释放
            return 3; // KEY2按下
        }
    }
	
	return 0;
}

