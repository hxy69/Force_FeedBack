/**
 * @file    current_loop.c
 * @brief   直流有刷电机电流环闭环控制实现
 * @author  Dr. GAO
 * @date    2026-06-29
 * @version V1.1
 * @website https://genbotter.taobao.com
 *
 * @note    本模块负责：
 *          1. 将 PID 控制器与电机驱动/电流传感器桥接
 *          2. 每个控制周期读取电流反馈 → PID 计算 → 输出 PWM
 *          3. 处理方向切换、死区、IR2104 掉电恢复等工程细节
 *          4. 仅在方向变化时调用 SetDirection，避免每 2ms 重复 SD 翻转
 */

#include "current_loop.h"
#include "bsp_current_sensor.h"  /* BSP_CurrentSensor_GetCurrent */
#include "user_config.h"         /* 所有 PID 和系统参数宏定义    */

/*==============================================================================
 * 内部常量
 *============================================================================*/

/** @brief 方向死区阈值（PID 输出幅值低于此值视为停止） */
#define DIRECTION_DEADBAND  1.0f

/**
 * @brief SD 健康检查间隔（控制周期数）
 * @note  IR2104 欠压保护（UVLO）后芯片内部锁死，即使 SD 脚高电平也不输出。
 *        必须先拉低再拉高才能真正退出保护态。每 SD_HEALTH_PERIOD 个控制周期
 *        执行一次 SD 拉低→拉高操作，确保 UVLO 后能自动恢复。
 *        2ms × 250 = 500ms 间隔，对电机运行无影响。
 */
#define SD_HEALTH_PERIOD    250u

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
    if (motor == NULL || motor->sd_port == NULL || motor->sd_pin == 0)
    {
        return;
    }

    HAL_GPIO_WritePin(motor->sd_port, motor->sd_pin, GPIO_PIN_RESET);
    /* 短暂延时（~50us），等 IR2104 内部状态机复位 */
    for (volatile uint32_t i = 0; i < 100; i++) { __NOP(); }
    HAL_GPIO_WritePin(motor->sd_port, motor->sd_pin, GPIO_PIN_SET);
}

/*==============================================================================
 * 公开接口实现
 *============================================================================*/

/**
 * @brief 初始化电流环控制器
 */
void CurrentLoop_Init(CurrentLoopCtrl *loop, BrushMotorConfig *motor)
{
    if (loop == NULL || motor == NULL)
    {
        return;
    }

    loop->motor     = motor;
    loop->state     = CURRENT_LOOP_STOPPED;
    loop->target_ma = CURRENT_TARGET_DEFAULT_mA;
    loop->prev_dir  = MOTOR_STOP;
    loop->sd_health = 0;

    PID_Init(&loop->pid,
             PID_KP,
             PID_KI,
             PID_KD,
             OUTPUT_MAX,
             OUTPUT_MIN,
             INTEGRAL_LIMIT);

    BrushMotor_Disable();
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
        BrushMotor_SetDirection(MOTOR_FORWARD);
        loop->prev_dir = MOTOR_FORWARD;
    }
    else
    {
        BrushMotor_SetDirection(MOTOR_REVERSE);
        loop->prev_dir = MOTOR_REVERSE;
    }
    BrushMotor_SetSpeed(0);

    /* SetDirection 内部已拉高 SD，这里的 Enable 是显式保险 */
    BrushMotor_Enable();

    loop->state     = CURRENT_LOOP_RUNNING;
    loop->sd_health = 0;
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

    BrushMotor_Disable();
    loop->state    = CURRENT_LOOP_STOPPED;
    loop->prev_dir = MOTOR_STOP;
}

/**
 * @brief 执行一次电流环控制迭代
 *
 * @note  V1.1 改动：
 *        - 仅在方向变化时调用 SetDirection，避免每 2ms 重复 SD 翻转和 PWM 启停
 *        - 死区内用 BrushMotor_Disable() 代替 SetDirection(MOTOR_STOP)
 *        - 每 SD_HEALTH_PERIOD 周期执行一次 SD 恢复脉冲，防止 IR2104 UVLO 锁死
 */
void CurrentLoop_Update(CurrentLoopCtrl *loop)
{
    if (loop == NULL || loop->state != CURRENT_LOOP_RUNNING)
    {
        return;
    }

    /* ---- Step 1: 读取滤波后的电流反馈值（mA） ---- */
    float current_ma = BSP_CurrentSensor_GetCurrent();

    /* ---- Step 2: PID 计算 ---- */
    float output = PID_Compute(&loop->pid, loop->target_ma, current_ma);

    /* ---- Step 3: 分离方向与占空比 ---- */
    MotorDirection new_dir;
    uint8_t duty;

    if (output > DIRECTION_DEADBAND)
    {
        new_dir = MOTOR_FORWARD;
        if (output > 100.0f) output = 100.0f;
        duty = (uint8_t)output;
    }
    else if (output < -DIRECTION_DEADBAND)
    {
        new_dir = MOTOR_REVERSE;
        if (-output > 100.0f) output = -100.0f;
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
            BrushMotor_SetDirection(new_dir);
            loop->prev_dir = new_dir;
        }
        BrushMotor_SetSpeed(duty);
    }
    else
    {
        /* 死区停止：如果之前是运行态，关断驱动 */
        if (loop->prev_dir != MOTOR_STOP)
        {
            BrushMotor_Disable();
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
    loop->sd_health++;
    if (loop->sd_health >= SD_HEALTH_PERIOD)
    {
        loop->sd_health = 0;
        if (loop->prev_dir != MOTOR_STOP)
        {
            SD_Recover(loop->motor);
        }
    }
#endif
}

void CurrentLoop_SetTarget(CurrentLoopCtrl *loop, float target_ma)
{
    if (loop == NULL) return;

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
    if (loop == NULL) return;
    PID_SetTunings(&loop->pid, Kp, Ki, Kd);
}
