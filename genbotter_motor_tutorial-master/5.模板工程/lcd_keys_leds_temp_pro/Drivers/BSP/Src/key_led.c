 #include "key_led.h"

/* 按键初始化函数实现
 * 配置按键对应的 GPIO 为输入模式，由于在 main.h 中已通过 CubeMX 配置好
 * 这里主要是使能相应 GPIO 时钟（若 CubeMX 未自动使能，实际项目中需补充，一般 CubeMX 会处理）
 */
void Key_Init(void)
{
    // 使能 GPIOE 时钟，STM32F4 系列中，可根据实际 MCU 型号调整时钟使能函数
    __HAL_RCC_GPIOE_CLK_ENABLE(); 
    // 这里按键在 CubeMX 中已配置为输入模式，无需再次配置模式等，若有需要可补充
}

/* 按键扫描函数实现
 * 简单轮询方式检测按键，可根据需求改为中断方式
 * 返回值：0 表示无按键按下，1 表示 KEY0 按下，2 表示 KEY1 按下，3 表示 KEY2 按下 
 */
KeyPressedID Key_Scan(void)
{
    if (HAL_GPIO_ReadPin(KEY0_GPIO_Port, KEY0_Pin) == GPIO_PIN_RESET)
    {
        HAL_Delay(10);  // 消抖
        if (HAL_GPIO_ReadPin(KEY0_GPIO_Port, KEY0_Pin) == GPIO_PIN_RESET)
        {
            while (HAL_GPIO_ReadPin(KEY0_GPIO_Port, KEY0_Pin) == GPIO_PIN_RESET);  // 等待按键释放
            return KEY0_Pressed;    // KEY0 按下
        }
    }
    else if (HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin) == GPIO_PIN_RESET)
    {
        HAL_Delay(10);  // 消抖
        if (HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin) == GPIO_PIN_RESET)
        {
            while (HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin) == GPIO_PIN_RESET);  // 等待按键释放
            return KEY1_Pressed;    // KEY1 按下
        }
    }
    else if (HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin) == GPIO_PIN_RESET)
    {
        HAL_Delay(10);  // 消抖
        if (HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin) == GPIO_PIN_RESET)
        {
            while (HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin) == GPIO_PIN_RESET);  // 等待按键释放
            return KEY2_Pressed;    // KEY2 按下
        }
    }
    return KEY_None;  // 无按键按下
}

/* LED 初始化函数实现
 * 配置 LED 对应的 GPIO 为输出模式，由于在 main.h 中已通过 CubeMX 配置好
 * 这里主要是使能相应 GPIO 时钟（若 CubeMX 未自动使能，实际项目中需补充，一般 CubeMX 会处理）
 */
void Led_Init(void)
{
    // 使能 GPIOE 时钟，STM32F4 系列中，可根据实际 MCU 型号调整时钟使能函数
    __HAL_RCC_GPIOE_CLK_ENABLE(); 
    // 这里 LED 在 CubeMX 中已配置为输出模式，无需再次配置模式等，若有需要可补充
}

/* 点亮指定 LED 函数实现 */
void Led_On(LED_ID led_id)
{
    if(led_id == LED1)
        HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
    else if(led_id == LED2)
        HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);
}

/* 熄灭指定 LED 函数实现 */
void Led_Off(LED_ID led_id)
{
    if(led_id == LED1)
        HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
    else if(led_id == LED2)
        HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_SET);
}

/* 翻转指定 LED 状态函数实现 */
void Led_Toggle(LED_ID led_id)
{
    if(led_id == LED1)
        HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
    else if(led_id == LED2)
        HAL_GPIO_TogglePin(LED2_GPIO_Port, LED2_Pin);
}
