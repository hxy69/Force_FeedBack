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
 * @brief 按键扫描（轮询方式，10ms消抖，松手检测）
 * @retval 按键ID：KEY0_Pressed / KEY1_Pressed / KEY2_Pressed / KEY_None
 * @note   采用阻塞式松手等待，函数会卡住直到按键释放
 */
KeyPressedID Key_Scan(void)
{
    if (HAL_GPIO_ReadPin(KEY0_GPIO_Port, KEY0_Pin) == GPIO_PIN_RESET)
    {
        HAL_Delay(10);  // 消抖
        if (HAL_GPIO_ReadPin(KEY0_GPIO_Port, KEY0_Pin) == GPIO_PIN_RESET)
        {
            while (HAL_GPIO_ReadPin(KEY0_GPIO_Port, KEY0_Pin) == GPIO_PIN_RESET);  // 等待松手
            return KEY0_Pressed;
        }
    }
    else if (HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin) == GPIO_PIN_RESET)
    {
        HAL_Delay(10);
        if (HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin) == GPIO_PIN_RESET)
        {
            while (HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin) == GPIO_PIN_RESET);
            return KEY1_Pressed;
        }
    }
    else if (HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin) == GPIO_PIN_RESET)
    {
        HAL_Delay(10);
        if (HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin) == GPIO_PIN_RESET)
        {
            while (HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin) == GPIO_PIN_RESET);
            return KEY2_Pressed;
        }
    }
    return KEY_None;
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
