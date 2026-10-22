/**
 * @file    bsp_key_led.c
 * @brief   按键扫描与LED控制源文件
 * @author  Dr. GAO
 * @date    2026-05-26
 * @version V1.0
 * @website https://genbotter.taobao.com
 * @note    该文件适用于GenBotter Motor-1电机开发板，STM32F407IGT6。
 *          按键采用轮询扫描方式，含10ms消抖和松手检测。
 *          LED低电平点亮（共阳接法），支持开/关/翻转。
 */
#include "bsp_key_led.h"

/**
 * @brief 按键初始化（使能GPIOE时钟，引脚模式由CubeMX配置）
 */
void Key_Init(void)
{
    __HAL_RCC_GPIOE_CLK_ENABLE();
}

/**
 * @brief 按键扫描（10ms 消抖 + 松手检测，防止重复触发）
 * @param mode: 0=不连续按（松手后才能再次触发）, 1=连续按
 * @retval 按键ID
 */
uint8_t Key_Scan(uint8_t mode)
{
    static uint8_t key_up = 1;
    uint8_t keyval = KEY_None;

    if (mode) key_up = 1;

    if (key_up && (HAL_GPIO_ReadPin(KEY0_GPIO_Port, KEY0_Pin) == GPIO_PIN_RESET
                || HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin) == GPIO_PIN_RESET
                || HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin) == GPIO_PIN_RESET))
    {
        HAL_Delay(10);                              /* 消抖 */
        key_up = 0;

        if (HAL_GPIO_ReadPin(KEY0_GPIO_Port, KEY0_Pin) == GPIO_PIN_RESET)  keyval = KEY0_Pressed;
        if (HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin) == GPIO_PIN_RESET)  keyval = KEY1_Pressed;
        if (HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin) == GPIO_PIN_RESET)  keyval = KEY2_Pressed;
    }
    else if (HAL_GPIO_ReadPin(KEY0_GPIO_Port, KEY0_Pin) == GPIO_PIN_SET
          && HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin) == GPIO_PIN_SET
          && HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin) == GPIO_PIN_SET)
    {
        key_up = 1;                                 /* 所有键松开，允许下次触发 */
    }

    return keyval;
}

/**
 * @brief LED初始化（使能GPIOE时钟，引脚模式由CubeMX配置）
 */
void Led_Init(void)
{
    __HAL_RCC_GPIOE_CLK_ENABLE();
}

/**
 * @brief 点亮指定LED（低电平有效）
 * @param led_id LED1 或 LED2
 */
void Led_On(LED_ID led_id)
{
    if(led_id == LED1)
        HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
    else if(led_id == LED2)
        HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);
}

/**
 * @brief 熄灭指定LED（高电平有效）
 * @param led_id LED1 或 LED2
 */
void Led_Off(LED_ID led_id)
{
    if(led_id == LED1)
        HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
    else if(led_id == LED2)
        HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_SET);
}

/**
 * @brief 翻转指定LED状态
 * @param led_id LED1 或 LED2
 */
void Led_Toggle(LED_ID led_id)
{
    if(led_id == LED1)
        HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
    else if(led_id == LED2)
        HAL_GPIO_TogglePin(LED2_GPIO_Port, LED2_Pin);
}
