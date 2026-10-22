#include "current_loop.h"
#include "current_sensor.h" /* BSP_CurrentSensor_GetCurrent */
#include "user_config.h"    /* 所有 PID 和系统参数宏定义    */
#include "brush_motor.h"    /* BrushMotor_SetDirection/SetSpeed/Enable/Disable */
#include "vofa_plus.h"      /* VOFA+调试数据输出接口       */

/** @brief 方向死区阈值（PID 输出幅值低于此值视为停止） */
#define DIRECTION_DEADBAND 1.0f

/**
 * @brief CTRL 健康检查间隔（控制周期数）
 * @note  IR2104 欠压保护（UVLO）后芯片内部锁死，即使 CTRL 脚高电平也不输出。
 *        必须先拉低再拉高才能真正退出保护态。每 CTRL_HEALTH_PERIOD 个控制周期
 *        执行一次 CTRL 拉低→拉高操作，确保 UVLO 后能自动恢复。
 *        2ms × 250 = 500ms 间隔，对电机运行无影响。
 */
#define CTRL_HEALTH_PERIOD 250u

CurrentLoopCtrl current_loop[3];
/*==============================================================================
 * 内部辅助
 *============================================================================*/

/**
 * @brief 安全地重新使能 IR2104 SD 引脚（拉低→延时→拉高）
 * @note  专门用于 UVLO 恢复场景。
 *        操作期间不停止 PWM 通道，仅短暂拉低 SD 引脚再恢复。
 */
static void SD_Recover(BrushMotorConfig *motor)
{
    if (motor == NULL || motor->ctrl_port == NULL || motor->ctrl_pin == 0)
    {
        return;
    }

    HAL_GPIO_WritePin(motor->ctrl_port, motor->ctrl_pin, GPIO_PIN_RESET);
    /* 短暂延时（~50us），等 IR2104 内部状态机复位 */
    for (volatile uint32_t i = 0; i < 100; i++)
    {
        __NOP();
    }
    HAL_GPIO_WritePin(motor->ctrl_port, motor->ctrl_pin, GPIO_PIN_SET);
}


/**
 * @brief 初始化电流环控制器
 */
void CurrentLoop_Init(CurrentLoopCtrl *loop, BrushMotorConfig *motor)
{
    if (loop == NULL || motor == NULL)
    {
        return;
    }

    loop->motor = motor;
    loop->state = CURRENT_LOOP_STOPPED;
    loop->target_ma = CURRENT_TARGET_DEFAULT_mA;
    loop->prev_dir = MOTOR_STOP;
    loop->ctrl_health = 0;

    /* 根据PWM通道判断是哪一路电机，加载对应的PID默认值 */
    float kp, ki, kd;
    if (loop->motor->tim_channel == TIM_CHANNEL_1)
    {
        kp = PID_KP_X;
        ki = PID_KI_X;
        kd = PID_KD_X;
    }
    else if (loop->motor->tim_channel == TIM_CHANNEL_2)
    {
        kp = PID_KP_Y;
        ki = PID_KI_Y;
        kd = PID_KD_Y;
    }
    else
    {
        kp = PID_KP_Z;
        ki = PID_KI_Z;
        kd = PID_KD_Z;
    }

    PID_Init(&loop->pid, kp, ki, kd, OUTPUT_MAX, OUTPUT_MIN, INTEGRAL_LIMIT);

    BrushMotor_Disable(loop->motor);
}

/**
 * @brief 启动电流环闭环控制
 *
 * @note  先根据目标电流符号决定初始方向，避免固定正转与负目标冲突。
 */
void CurrentLoop_Start(CurrentLoopCtrl *loop)
{
    if (loop == NULL)
    {
        return;
    }

    PID_Reset(&loop->pid);

    /* 根据目标电流符号决定初始方向 */
    if (loop->target_ma >= 0.0f)
    {
        BrushMotor_SetDirection(loop->motor, MOTOR_FORWARD);
        loop->prev_dir = MOTOR_FORWARD;
    }
    else
    {
        BrushMotor_SetDirection(loop->motor, MOTOR_REVERSE);
        loop->prev_dir = MOTOR_REVERSE;
    }
    BrushMotor_SetSpeed(loop->motor, 0);

    /* SetDirection 内部已拉高 SD，这里的 Enable 是显式保险 */
    BrushMotor_Enable(loop->motor);

    loop->state = CURRENT_LOOP_RUNNING;
    loop->ctrl_health = 0;
}

/**
 * @brief 停止电流环闭环控制
 */
void CurrentLoop_Stop(CurrentLoopCtrl *loop)
{
    if (loop == NULL)
    {
        return;
    }

    BrushMotor_Disable(loop->motor);
    loop->state = CURRENT_LOOP_STOPPED;
    loop->prev_dir = MOTOR_STOP;
    PID_Reset(&loop->pid);
}

/**
 * @brief 执行一次电流环控制迭代
 *
 * @note  V1.1 改动：
 *        - 仅在方向变化时调用 SetDirection，避免每 2ms 重复 SD 翻转和 PWM 启停
 *        - 死区内用 BrushMotor_Disable() 代替 SetDirection(MOTOR_STOP)
 *        - 每 CTRL_HEALTH_PERIOD 周期执行一次 SD 恢复脉冲，防止 IR2104 UVLO 锁死
 */
void CurrentLoop_Update(CurrentLoopCtrl *loop)
{
    if (loop == NULL || loop->state != CURRENT_LOOP_RUNNING)
    {
        return;
    }

    /* ---- Step 1: 读取滤波后的电流反馈值（mA） ---- */
    float current_ma = BSP_CurrentSensor_GetCurrent(loop->motor_id);

    /* ---- Step 2: PID 计算 ---- */
    float output = PID_Compute(&loop->pid, loop->target_ma, current_ma);

    /* ---- Step 3: 分离方向与占空比 ---- */
    MotorDirection new_dir;
    uint8_t duty;

    if (output > DIRECTION_DEADBAND)
    {
        new_dir = MOTOR_FORWARD;
        if (output > 100.0f)
            output = 100.0f;
        duty = (uint8_t)output;
    }
    else if (output < -DIRECTION_DEADBAND)
    {
        new_dir = MOTOR_REVERSE;
        if (-output > 100.0f)
            output = -100.0f;
        duty = (uint8_t)(-output);
    }
    else
    {
        new_dir = MOTOR_STOP;
        duty = 0;
    }

    /* ---- Step 4: 仅在方向变化或从停止恢复时写硬件 ---- */
    if (new_dir != MOTOR_STOP)
    {
        if (new_dir != loop->prev_dir)
        {
            /* 方向变了（或刚从停止恢复）→ SetDirection 内部会停 PWM、切方向、拉高 SD */
            BrushMotor_SetDirection(loop->motor, new_dir);
            loop->prev_dir = new_dir;
        }
        BrushMotor_SetSpeed(loop->motor, duty);
    }
    else
    {
        /* 死区停止：如果之前是运行态，关断驱动 */
        if (loop->prev_dir != MOTOR_STOP)
        {
            BrushMotor_Disable(loop->motor);
            loop->prev_dir = MOTOR_STOP;
        }
    }

    /* ---- Step 5: IR2104 SD 周期性健康检查（防 UVLO 锁死） ---- */
    /*
     * IR2104 特性：当 VCC < 8.6V 或 VB-VS < 8.2V 时触发欠压保护，
     * 内部强制关闭输出。即使 SD 引脚持续为高，UVLO 退出后也需
     * SD 先拉低再拉高才能重新使能。
     *
     * 注意：SD 脉冲会瞬间关闭输出，调试阶段暂时关闭。
     * 若需 UVLO 自动恢复，将 #if 0 改为 #if 1。
     */
#if 0
    loop->ctrl_health++;
    if (loop->ctrl_health >= CTRL_HEALTH_PERIOD)
    {
        loop->ctrl_health = 0;
        if (loop->prev_dir != MOTOR_STOP)
        {
            SD_Recover(loop->motor);
        }
    }
#endif
}

void CurrentLoop_SetTarget(CurrentLoopCtrl *loop, float target_ma)
{
    if (loop == NULL)
        return;

    if (target_ma > CURRENT_TARGET_MAX_mA)
        target_ma = CURRENT_TARGET_MAX_mA;
    else if (target_ma < CURRENT_TARGET_MIN_mA)
        target_ma = CURRENT_TARGET_MIN_mA;

    loop->target_ma = target_ma;
}

float CurrentLoop_GetTarget(const CurrentLoopCtrl *loop)
{
    return (loop == NULL) ? 0.0f : loop->target_ma;
}

float CurrentLoop_GetError(const CurrentLoopCtrl *loop)
{
    return (loop == NULL) ? 0.0f : PID_GetError(&loop->pid);
}

float CurrentLoop_GetOutput(const CurrentLoopCtrl *loop)
{
    return (loop == NULL) ? 0.0f : PID_GetOutput(&loop->pid);
}

CurrentLoopState CurrentLoop_GetState(const CurrentLoopCtrl *loop)
{
    return (loop == NULL) ? CURRENT_LOOP_STOPPED : loop->state;
}

void CurrentLoop_SetPID(CurrentLoopCtrl *loop, float Kp, float Ki, float Kd)
{
    if (loop == NULL)
        return;
    PID_SetTunings(&loop->pid, Kp, Ki, Kd);
}

/**
 * @brief  获取当前速度环调试数据
 * @return 包含目标转速、实际转速、PID输出、最终输出的结构体
 */
CurrentLoop_DebugData_t CurrentLoop_GetDebugData(void)
{
    CurrentLoop_DebugData_t debug_data;
    debug_data.target_current = CurrentLoop_GetTarget(&current_loop[0]); // 目标转速
    debug_data.actual_current = BSP_CurrentSensor_GetCurrent(0);         // 实际转速（滤波后的值）
    debug_data.pid_output = PID_GetOutput(&current_loop[0].pid);         // PID原始输出
    debug_data.final_output = PID_GetOutput(&current_loop[0].pid);       // 死区补偿+限幅后的最终输出
    return debug_data;
}
