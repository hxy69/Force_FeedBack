/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2026-07-29 14:07:26
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2026-08-03 17:58:11
 * @FilePath: \Force_feedback\Drivers\BSP\Src\brush_motor.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "brush_motor.h"



/**
 * @brief  刷电机初始化
 */
void BrushMotor_Init(BrushMotorConfig *config)
{
    if (config == NULL) return;
    config->current_direction = 1;  /* 默认正向 */
    BrushMotor_Stop(config);
}

/**
 * @brief 自举电容预充电（上电初始化时调用一次）
 * @note  以50%互补PWM运行10ms，双半桥等电位电机不动，
 *        两路Cboot均充满，消除掉电/UVLO后冷启动不自举的问题。
 */
void BrushMotor_BootstrapCharge(BrushMotorConfig *motor)
{
    if (motor == NULL) return;
    if (motor->ctrl_port == NULL || motor->ctrl_pin == 0) return;

    /* 设置50%占空比（双端等电位，电机两端电压 = 0，不动） */
    uint32_t half_arr = (motor->tim_arr + 1) / 2;
    __HAL_TIM_SET_COMPARE(motor->htim, motor->tim_channel, half_arr);

    /* 同时启动主通道和互补通道（H桥四管均交替导通） */
    HAL_TIM_PWM_Start(motor->htim, motor->tim_channel);
    HAL_TIMEx_PWMN_Start(motor->htim, motor->tim_channel);

    /* 拉高SD使能IR2104，Cboot经10ms充分充电 */
    HAL_GPIO_WritePin(motor->ctrl_port, motor->ctrl_pin, GPIO_PIN_SET);
    HAL_Delay(10);

    /* 恢复停止态：拉低SD → 停止PWM → 清零CCR */
    HAL_GPIO_WritePin(motor->ctrl_port, motor->ctrl_pin, GPIO_PIN_RESET);
    HAL_TIMEx_PWMN_Stop(motor->htim, motor->tim_channel);
    HAL_TIM_PWM_Stop(motor->htim, motor->tim_channel);
    __HAL_TIM_SET_COMPARE(motor->htim, motor->tim_channel, 0);
}

/**
 * @brief  刷电机使能（拉高SD）
 */
void BrushMotor_Enable(BrushMotorConfig *motor)
{
    if (motor == NULL) return;
    if (motor->ctrl_port == NULL) return;
    HAL_GPIO_WritePin(motor->ctrl_port, motor->ctrl_pin, GPIO_PIN_SET);
}

/**
 * @brief  刷电机停止（拉低SD + 停PWM + 清CCR）
 */
void BrushMotor_Stop(BrushMotorConfig *motor)
{
    if (motor == NULL) return;
    if (motor->ctrl_port == NULL) return;
    HAL_GPIO_WritePin(motor->ctrl_port, motor->ctrl_pin, GPIO_PIN_RESET);
    HAL_TIM_PWM_Stop(motor->htim, motor->tim_channel);
    HAL_TIMEx_PWMN_Stop(motor->htim, motor->tim_channel);
    __HAL_TIM_SET_COMPARE(motor->htim, motor->tim_channel, 0);
}

/**
 * @brief  设置刷电机速度
 * @param  speed: 速度值，范围0-100
 */
void BrushMotor_SetSpeed(BrushMotorConfig *motor, uint8_t speed)
{
    if (motor == NULL) return;
    if (speed > 100) 
    {
        speed = 100;
    }
    __HAL_TIM_SET_COMPARE(motor->htim, motor->tim_channel, (uint32_t)(speed * (motor->tim_arr / 100)));
}

/**
 * @brief  电机方向设置
 * @param  direction: MOTOR_FORWARD / MOTOR_REVERSE / MOTOR_STOP
 */
void BrushMotor_SetDirection(BrushMotorConfig *motor, MotorDirection direction)
{
    if (motor == NULL) return;

    /* 拉低SD，安全过渡 */
    if (motor->ctrl_port != NULL && motor->ctrl_pin != 0)
    {
        HAL_GPIO_WritePin(motor->ctrl_port, motor->ctrl_pin, GPIO_PIN_RESET);
    }

    /* 同时Stop主通道和互补通道，将HAL状态机复位为READY。
       由于TIM1 TRGO（Update Event）始终在输出，TRGO不受影响。 */
    HAL_TIM_PWM_Stop(motor->htim, motor->tim_channel);
    HAL_TIMEx_PWMN_Stop(motor->htim, motor->tim_channel);

    switch (direction)
    {
        case MOTOR_FORWARD:
            HAL_TIM_PWM_Start(motor->htim, motor->tim_channel);
            motor->current_direction = 1;
            break;

        case MOTOR_REVERSE:
            HAL_TIMEx_PWMN_Start(motor->htim, motor->tim_channel);
            motor->current_direction = -1;
            break;

        case MOTOR_STOP:
        default:
            break;
    }

    /* 拉高SD，使能驱动芯片 */
    if (motor->ctrl_port != NULL && motor->ctrl_pin != 0)
    {
        HAL_GPIO_WritePin(motor->ctrl_port, motor->ctrl_pin, GPIO_PIN_SET);
    }
}





/**
 * @brief 禁用电机关断（仅拉低SD + 清零CCR，不停定时器）
 * @note  与 Stop() 的区别：不调 HAL_TIM_PWM_Stop，保留TIM1运行以保证
 *         TRGO（Update Event）→ ADC注入触发链不间断。
 *         运行中临时停转用此函数，之后可立即恢复。
 */
void BrushMotor_Disable(BrushMotorConfig *motor)
{
    if (motor == NULL) return;
    if (motor->ctrl_port == NULL || motor->ctrl_pin == 0) return;

    HAL_GPIO_WritePin(motor->ctrl_port, motor->ctrl_pin, GPIO_PIN_RESET);
    __HAL_TIM_SET_COMPARE(motor->htim, motor->tim_channel, 0);
}
