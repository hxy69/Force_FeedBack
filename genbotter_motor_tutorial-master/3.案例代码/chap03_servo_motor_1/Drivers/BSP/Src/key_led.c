#include "key_led.h"

void KEY_Init(void) {
    // Initialize GPIO pins for keys
    // 已经在gpio.c中配置了按键GPIO
}

void LED_Init(void) {
    // Initialize GPIO pins for keys
    // 已经在gpio.c中配置了按键GPIO
}

/**
 * @brief 控制LED点亮
 * @param led_pin LED对应的GPIO引脚
 */
void LED_On(uint16_t led_pin) {
    HAL_GPIO_WritePin(GPIOE, led_pin, GPIO_PIN_RESET);
}

/**
 * @brief 控制LED熄灭
 * @param led_pin LED对应的GPIO引脚
 */
void LED_Off(uint16_t led_pin) {
    HAL_GPIO_WritePin(GPIOE, led_pin, GPIO_PIN_SET);
}

/**
 * @brief 控制LED翻转
 * @param led_pin LED对应的GPIO引脚
 */
void LED_Toggle(uint16_t led_pin) {
    HAL_GPIO_TogglePin(GPIOE, led_pin);
}

/**
 * @brief 扫描按键状态
 * @return 按键状态，
 *     0表示无按键按下，
 *     1表示KEY0按下,
 *     2表示KEY1按下,
 *     3表示KEY2按下
 */
uint8_t KEY_Scan(void) {
    if (HAL_GPIO_ReadPin(KEY0_GPIO_Port, KEY0_Pin) == GPIO_PIN_RESET) {
        HAL_Delay(10); // 防抖延时
        if (HAL_GPIO_ReadPin(KEY0_GPIO_Port, KEY0_Pin) == GPIO_PIN_RESET) {
            while (HAL_GPIO_ReadPin(KEY0_GPIO_Port, KEY0_Pin) == GPIO_PIN_RESET)
            {
                ; // 等待按键释放
            }
            
            return 1; // KEY0 pressed
        } 
    } else if (HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin) == GPIO_PIN_RESET) {
        HAL_Delay(10); // 防抖延时
        if (HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin) == GPIO_PIN_RESET) {
            while (HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin) == GPIO_PIN_RESET)
            {
                ; // 等待按键释放
            }       
            return 2; // KEY1 pressed
        }
    } else if (HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin) == GPIO_PIN_RESET) {
        HAL_Delay(10); // 防抖延时
        if (HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin) == GPIO_PIN_RESET) {
            while (HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin) == GPIO_PIN_RESET)
            {
                ; // 等待按键释放
            }
            return 3; // KEY2 pressed
        }
    }

    return 0;
}
