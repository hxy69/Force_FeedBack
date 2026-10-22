/**
 * @file    pid.c
 * @brief   通用 PID 控制器（位置式）实现
 * @note    位置式数字 PID 算法实现。
 *          核心思想：直接计算绝对控制量 u(k)，每次输出完整的控制值。
 *
 *          公式（连续 → 离散）：
 *          ┌────────────────────────────────────────────────────┐
 *          │  u(k) = Kp × e(k)                                  │
 *          │       + Ki × Σe(k)           ← 积分累加            │
 *          │       + Kd × [e(k)-e(k-1)]   ← 微分（差分近似）    │
 *          │                                                    │
 *          │  抗饱和：积分项 Σe 受 integral_limit 限幅。        │
 *          │  输出 u(k) 受 [output_min, output_max] 钳位。     │
 *          └────────────────────────────────────────────────────┘
 */

#include "pid.h"
#include <string.h> /* memset */

/*==============================================================================
 * 公开接口实现
 *============================================================================*/

/**
 * @brief 初始化 PID 控制器
 * @note  将所有内部状态清零，绑定用户配置的参数。
 *        初始化后即可调用 PID_Compute 开始闭环控制。
 */
void PID_Init(PID_Controller *pid, float Kp, float Ki, float Kd,
              float out_max, float out_min, float int_limit)
{
    if (pid == NULL)
    {
        return; /* 空指针保护，避免硬件错误 */
    }

    /* 清零整个结构体，确保所有状态字段初始为零 */
    memset(pid, 0, sizeof(PID_Controller));

    /* 绑定用户参数 */
    pid->Kp = Kp;
    pid->Ki = Ki;
    pid->Kd = Kd;
    pid->output_max = out_max;
    pid->output_min = out_min;
    pid->integral_limit = int_limit;
}

/**
 * @brief 执行一次位置式 PID 计算
 *
 * @note  调用时序（以电流环为例）：
 *        1. 读 ADC 电流 → actual
 *        2. 取目标电流 → target
 *        3. pid_output = PID_Compute(&pid, target, actual);
 *        4. BrushMotor_SetSpeed(pid_output);
 *
 *        计算步骤：
 *        Step 1: e(k) = target - actual
 *        Step 2: 积分累加 Σe += e(k)，并对 Σe 限幅
 *        Step 3: u(k) = Kp × e(k) + Ki × Σe + Kd × [e(k) - e(k-1)]
 *        Step 4: u(k) 钳位 → [output_min, output_max]
 *        Step 5: 保存 e(k) → e(k-1) 供下一拍微分使用
 */
float PID_Compute(PID_Controller *pid, float target, float actual)
{
    if (pid == NULL)
    {
        return 0.0f; /* 空指针保护 */
    }

    /* ---- Step 1: 计算当前拍误差 ---- */
    pid->target = target;
    pid->actual = actual;
    float error = target - actual; /* e(k)：正误差 = 实际值不足，需要增加输出 */
    pid->error = error;

    /* ---- Step 2: 积分累加 + 限幅（抗饱和） ---- */
    /*
     *  位置式 PID 的积分项 = Ki × Σe，Σe 会持续累加。
     *  必须限制 Σe 的幅值，防止误差持续存在时积分无限增长（windup）。
     *  同时也实现"遇限削弱积分"：当输出已达上限且误差仍为正时，
     *  积分不再累加（此处通过 integral_limit 钳位间接实现）。
     */
    pid->integral += error;
    if (pid->integral > pid->integral_limit)
        pid->integral = pid->integral_limit;
    else if (pid->integral < -pid->integral_limit)
        pid->integral = -pid->integral_limit;

    /* ---- Step 3: 位置式 PID 核心计算 ---- */
    /*
     *  u(k) = Kp × e(k)      ← 比例项：误差越大输出越大，立即响应
     *       + Ki × Σe         ← 积分项：消除稳态误差
     *       + Kd × Δe         ← 微分项：抑制超调（电流环通常设 0）
     */
    float p_term = pid->Kp * error;
    float i_term = pid->Ki * pid->integral;
    float d_term = pid->Kd * (error - pid->error_prev);

    pid->output = p_term + i_term + d_term;

    /* ---- Step 4: 输出钳位 ---- */
    if (pid->output > pid->output_max)
    {
        pid->output = pid->output_max;
    }
    else if (pid->output < pid->output_min)
    {
        pid->output = pid->output_min;
    }

    /* ---- Step 5: 保存误差供下一拍微分使用 ---- */
    pid->error_prev = error;

    return pid->output;
}

/**
 * @brief 重置 PID 控制器内部状态
 *
 * @note  将输出、积分项、误差全部归零。
 *        适用场景举例：
 *        - 电机从停止到启动：避免上电瞬间积分项累积导致输出跳变
 *        - 目标值大幅切换（如 100mA → 800mA）：避免历史积分干扰新稳态
 *        - 发生过流保护后恢复：清除可能已饱和的积分项
 */
void PID_Reset(PID_Controller *pid)
{
    if (pid == NULL)
    {
        return;
    }

    pid->output = 0.0f;
    pid->integral = 0.0f;
    pid->error = 0.0f;
    pid->error_prev = 0.0f;
}

/**
 * @brief 运行时修改 PID 参数（在线调参）
 *
 * @note  不改变内部状态（误差历史、积分），只替换系数。
 *        因此切换 Kp/Ki/Kd 后下一个周期立即生效，无需重新初始化。
 *        如果新参数与旧参数差异很大，建议配合 PID_Reset() 使用。
 */
void PID_SetTunings(PID_Controller *pid, float Kp, float Ki, float Kd)
{
    if (pid == NULL)
    {
        return;
    }

    pid->Kp = Kp;
    pid->Ki = Ki;
    pid->Kd = Kd;
}

/**
 * @brief 运行时修改输出限幅
 */
void PID_SetOutputLimits(PID_Controller *pid, float out_max, float out_min)
{
    if (pid == NULL)
    {
        return;
    }

    pid->output_max = out_max;
    pid->output_min = out_min;
}

/**
 * @brief 获取当前输出值（不执行计算）
 */
float PID_GetOutput(const PID_Controller *pid)
{
    if (pid == NULL)
    {
        return 0.0f;
    }

    return pid->output;
}

/**
 * @brief 获取当前误差（不执行计算）
 */
float PID_GetError(const PID_Controller *pid)
{
    if (pid == NULL)
    {
        return 0.0f;
    }

    return pid->error;
}
