/**
 * @file    bsp_brush_motor.c
 * @brief   直流有刷电机控制源文件
 * @author  Dr. GAO
 * @date    2026-05-26
 * @version V1.2
 * @website https://genbotter.taobao.com
 * @note    适用于GenBotter Motor-1电机开发板，使用PM1接口控制直流有刷电机。
 *          电机驱动采用双IR2104半桥组成全H桥架构：
 *            - TIM1 CH1  (PA8)  → 半桥A HIN → 控制A相高端
 *            - TIM1 CH1N (PB13) → 半桥B HIN → 控制B相高端
 *          （每路IR2104的LIN由板载反相器从HIN生成，实现自动互补）
 *          正转时CH1输出PWM、CH1N关闭；反转时CH1关闭、CH1N输出PWM。
 *          方向切换时通过 Stop→Stop→Start 确保HAL状态机不冲突。
 *
 *          ★ 自举电容预充电：
 *            IR2104为自举供电架构，高端MOSFET需要Cboot维持栅极驱动电压。
 *            Cboot仅在低端MOSFET导通（VS拉至GND）时通过自举二极管充电。
 *            BrushMotor_BootstrapCharge() 在上电初始化时以50%互补PWM短暂运行，
 *            使双半桥等电位（电机不动）同时两端Cboot均充满，解决掉电/复位后
 *            "自举欠压→高端锁死→电机不动"的冷启动问题。
 */
#include "bsp_brush_motor.h"

static BrushMotorConfig *motor_config = NULL;
int8_t g_motor_current_direction = 1;  /* 1=正向, -1=反向，供 current_sensor 读取 */

/**
 * @brief 初始化电机控制，保存配置并确保电机初始停止
 * @param config 电机配置结构体指针（定时器句柄、通道、ARR值、SD引脚）
 */
void BrushMotor_Init(BrushMotorConfig *config)
{
    if (config == NULL)
    {
        return;
    }
    motor_config = config;
    BrushMotor_Stop();
}

/**
 * @brief 自举电容预充电（上电初始化时调用一次）
 * @note  以50%互补PWM运行10ms，双半桥等电位电机不动，
 *        两路Cboot均充满，消除掉电/UVLO后冷启动不自举的问题。
 *        调用前PWM已初始化（MX_TIM1_Init完成），调用后恢复Stop状态。
 */
void BrushMotor_BootstrapCharge(void)
{
    if (motor_config == NULL)
    {
        return;
    }
    if (motor_config->sd_port == NULL || motor_config->sd_pin == 0)
    {
        return;
    }

    /* 设置50%占空比（双端等电位，电机两端电压 = 0，不动） */
    uint32_t half_arr = (motor_config->tim_arr + 1) / 2;    /* 50% 占空比 */
    __HAL_TIM_SET_COMPARE(motor_config->htim, motor_config->tim_channel, half_arr); /* CCR = 50% */

    /* 同时启动主通道和互补通道（H桥四管均交替导通） */
    HAL_TIM_PWM_Start(motor_config->htim, motor_config->tim_channel);
    HAL_TIMEx_PWMN_Start(motor_config->htim, motor_config->tim_channel);

    /* 拉高SD使能IR2104，Cboot经10ms充分充电 */
    HAL_GPIO_WritePin(motor_config->sd_port, motor_config->sd_pin, GPIO_PIN_SET);
    HAL_Delay(10);

    /* 恢复停止态：拉低SD → 停止PWM → 清零CCR */
    HAL_GPIO_WritePin(motor_config->sd_port, motor_config->sd_pin, GPIO_PIN_RESET);
    HAL_TIMEx_PWMN_Stop(motor_config->htim, motor_config->tim_channel);
    HAL_TIM_PWM_Stop(motor_config->htim, motor_config->tim_channel);
    __HAL_TIM_SET_COMPARE(motor_config->htim, motor_config->tim_channel, 0);
}

/**
 * @brief 使能驱动芯片（SD引脚拉高）
 */
void BrushMotor_Enable(void)
{
    if (motor_config == NULL)
    {
        return;
    }
    if (motor_config->sd_port == NULL || motor_config->sd_pin == 0)
    {
        return;
    }
    HAL_GPIO_WritePin(motor_config->sd_port, motor_config->sd_pin, GPIO_PIN_SET);
}

/**
 * @brief 完全停止电机（关闭PWM输出，CCR清零，SD拉低）
 * @note  仅在初始化或紧急停止时使用。
 *         运行中临时停机请使用 BrushMotor_Disable()，保留TIM1运行以维持TRGO→ADC触发。
 */
void BrushMotor_Stop(void)
{
    if (motor_config == NULL)
    {
        return;
    }
    if (motor_config->sd_port == NULL || motor_config->sd_pin == 0)
    {
        return;
    }

    HAL_GPIO_WritePin(motor_config->sd_port, motor_config->sd_pin, GPIO_PIN_RESET);
    HAL_TIM_PWM_Stop(motor_config->htim, motor_config->tim_channel);
    HAL_TIMEx_PWMN_Stop(motor_config->htim, motor_config->tim_channel);
    __HAL_TIM_SET_COMPARE(motor_config->htim, motor_config->tim_channel, 0);
}

/**
 * @brief 禁用电机关断（仅拉低SD + 清零CCR，不停定时器）
 * @note  与 Stop() 的区别：不调 HAL_TIM_PWM_Stop，保留TIM1运行以保证
 *         TRGO（Update Event）→ ADC注入触发链不间断。
 *         运行中临时停转用此函数，之后可立即恢复。
 */
void BrushMotor_Disable(void)
{
    if (motor_config == NULL)
    {
        return;
    }
    if (motor_config->sd_port == NULL || motor_config->sd_pin == 0)
    {
        return;
    }

    HAL_GPIO_WritePin(motor_config->sd_port, motor_config->sd_pin, GPIO_PIN_RESET);
    __HAL_TIM_SET_COMPARE(motor_config->htim, motor_config->tim_channel, 0);
}

/**
 * @brief 设置电机速度（PWM占空比）
 * @param speed 速度百分比 0~100
 * @note   CCR = speed% × tim_arr / 100
 *         tim_arr 应等于定时器 Period+1（本工程为8367）
 */
void BrushMotor_SetSpeed(uint8_t speed)
{
    if (motor_config == NULL) return;
    if (speed > 100) speed = 100;

    uint32_t compare_val = ((uint32_t)speed * motor_config->tim_arr) / 100;
    __HAL_TIM_SET_COMPARE(motor_config->htim, motor_config->tim_channel, compare_val);
}

/**
 * @brief 设置电机转向
 * @param direction MOTOR_FORWARD（正转，仅CH1输出PWM）/ MOTOR_REVERSE（反转，仅CH1N输出PWM）/ MOTOR_STOP
 * @note   切换方向时分三步：拉低SD → 同时Stop两路（HAL Stop无状态检查，总置READY）
 *          → 按方向仅Start需要的那一路 → 拉高SD。
 *         不在此函数内清零CCR，速度统一由外部 SetSpeed() 独立设置。
 */
void BrushMotor_SetDirection(MotorDirection direction)
{
    if (motor_config == NULL)
    {
        return;
    }

    /* 拉低SD，安全过渡 */
    if (motor_config->sd_port != NULL && motor_config->sd_pin != 0)
    {
        HAL_GPIO_WritePin(motor_config->sd_port, motor_config->sd_pin, GPIO_PIN_RESET);
    }

    /* 同时Stop主通道和互补通道，将HAL状态机复位为READY。
       由于TIM1 TRGO（Update Event）始终在输出，TRGO不受影响。 */
    HAL_TIM_PWM_Stop(motor_config->htim, motor_config->tim_channel);
    HAL_TIMEx_PWMN_Stop(motor_config->htim, motor_config->tim_channel);

    switch (direction)
    {
        case MOTOR_FORWARD:
            HAL_TIM_PWM_Start(motor_config->htim, motor_config->tim_channel);
            g_motor_current_direction = 1;
            break;

        case MOTOR_REVERSE:
            HAL_TIMEx_PWMN_Start(motor_config->htim, motor_config->tim_channel);
            g_motor_current_direction = -1;
            break;

        case MOTOR_STOP:
        default:
            break;
    }

    /* 拉高SD，使能驱动芯片 */
    if (motor_config->sd_port != NULL && motor_config->sd_pin != 0)
    {
        HAL_GPIO_WritePin(motor_config->sd_port, motor_config->sd_pin, GPIO_PIN_SET);
    }
}

/**
 * @brief 同时开启PWM主通道和互补通道（备用接口）
 * @note  一般不直接调用，由 SetDirection() 按需启停对应通道
 */
void BrushMotor_StartPWM(void)
{
    if (motor_config == NULL)
    {
        return;
    }

    HAL_TIM_PWM_Start(motor_config->htim, motor_config->tim_channel);
    HAL_TIMEx_PWMN_Start(motor_config->htim, motor_config->tim_channel);
}
