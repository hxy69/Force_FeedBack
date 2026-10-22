/**
 * @file bsp_brush_motor.c
 * @brief Brush motor control source file直流有刷电机控制源文件
 * @version 1.0
 * @date 2025-11-07
 * @author Dr. GAO
 */
#include "bsp_brush_motor.h"

static BrushMotorConfig *motor_config = NULL;

/**
 * @brief Initialize the brush motor with the given configuration
 * @param config Pointer to BrushMotorConfig structure containing motor settings
 */
void BrushMotor_Init(BrushMotorConfig *config)
{
    // Initialize the GPIO pin for the IR2104 enable signal
    if (config == NULL)
    {
        return; // Handle null pointer
    }
    motor_config = config; // Store the configuration

    BrushMotor_Stop(); // Ensure motor is stopped initially
}

/**
 * @brief Enable the brush motor, activating its IR2104
 */
void BrushMotor_Enable(void)
{
    if (motor_config == NULL)
    {
        return; // Handle null pointer
    }

    if (motor_config->sd_port == NULL || motor_config->sd_pin == 0)
    {
        return; // Handle invalid GPIO configuration
    }

    HAL_GPIO_WritePin(motor_config->sd_port, motor_config->sd_pin, GPIO_PIN_SET);
}

/**
 * @brief Stop the brush motor, disabling its IR2104
 */
void BrushMotor_Stop(void)
{
    if (motor_config == NULL)
    {
        return; // Handle null pointer
    }

    if (motor_config->sd_port == NULL || motor_config->sd_pin == 0)
    {
        return; // Handle invalid GPIO configuration
    }

    HAL_GPIO_WritePin(motor_config->sd_port, motor_config->sd_pin, GPIO_PIN_RESET);
    HAL_TIM_PWM_Stop(motor_config->htim, motor_config->tim_channel);    // Stop PWM signal
    HAL_TIMEx_PWMN_Stop(motor_config->htim, motor_config->tim_channel); // Stop complementary PWM if used
    // Reset the PWM compare value to 0 to ensure motor is stopped
    __HAL_TIM_SET_COMPARE(motor_config->htim, motor_config->tim_channel, 0);
}

/**
 * @brief Set the speed of the brush motor
 * @param speed Speed value from 0 to 100, where 0 is stop and 100 is full speed
 */
void BrushMotor_SetSpeed(uint8_t speed)
{
    if (motor_config == NULL)
    {
        return; // Handle null pointer
    }
    // Assuming speed is a PWM value, this function would typically set a PWM duty cycle
    // Here we just simulate the action with a placeholder comment
    if (speed > 100)
    {
        speed = 100; // Clamp speed to maximum of 100
    }
    // 改变CH1或者CH1N的PWM的占空比来设置速度
    uint32_t compare_val = (uint32_t)(speed * (motor_config->tim_arr / 100)); // Scale speed to match PWM range
    __HAL_TIM_SET_COMPARE(motor_config->htim, motor_config->tim_channel, compare_val);
}

/**
 * @brief Set the direction of the brush motor
 * @param direction MotorDirection enum value indicating the desired direction
 */
void BrushMotor_SetDirection(MotorDirection direction)
{
    // Assuming direction is either 0 (forward) or 1 (reverse)
    if (motor_config == NULL)
    {
        return; // Handle null pointer
    }
    /*
    MOTOR_FORWARD = 0, // 正转
    MOTOR_REVERSE = 1,  // 反转
    MOTOR_STOP = 2      // 停止
    */
    switch (direction)
    {
    case MOTOR_FORWARD:
        // forward direction typically means enabling the motor with a PWM signal
        HAL_TIM_PWM_Start(motor_config->htim, motor_config->tim_channel);   // PWM for forward direction
        HAL_TIMEx_PWMN_Stop(motor_config->htim, motor_config->tim_channel); // Stop complementary PWM if used
        /* code */
        break;
    case MOTOR_REVERSE:
        HAL_TIM_PWM_Stop(motor_config->htim, motor_config->tim_channel);
        HAL_TIMEx_PWMN_Start(motor_config->htim, motor_config->tim_channel);
        break;
    case MOTOR_STOP:
        BrushMotor_Stop(); // Stop the motor if direction is stop
        break;

    default:
        break;
    }
}
