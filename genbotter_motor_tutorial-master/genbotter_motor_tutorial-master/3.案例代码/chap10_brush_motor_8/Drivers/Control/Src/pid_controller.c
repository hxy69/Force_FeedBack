#include "pid_controller.h"
#include "control_config.h"
#include <main.h>
#include <math.h>

/**
 * @brief 初始化PID控制器（教学版）
 * @param pid       PID控制器句柄（保存PID的所有状态）
 * @param kp        比例系数（快速响应误差）
 * @param ki        积分系数（消除静态误差）
 * @param kd        微分系数（抑制超调，预判趋势）
 * @param max_out   输出限幅（保护执行器，比如电机）
 * @param max_int   积分限幅（防止积分饱和）
 */
void PID_Init(PID_Handle_t *pid, float kp, float ki, float kd, float max_out, float max_int)
{
    // 1. 赋值PID核心参数
    pid->Kp = kp;                  // 比例系数
    pid->Ki = ki;                  // 积分系数
    pid->Kd = kd;                  // 微分系数
    pid->Output_Max = max_out;     // 输出最大值（限幅）
    pid->Integral_Max = max_int;   // 积分最大值（抗饱和）

    // 2. 复位PID状态（初始无误差、无积分、无输出）
    PID_Reset(pid);
}

/**
 * @brief 复位PID控制器状态（教学版）
 * @param pid PID控制器句柄
 * @note 把所有历史状态清零，恢复初始状态
 */
void PID_Reset(PID_Handle_t *pid)
{
    // 状态区清零：无目标、无反馈、无误差、无积分、无输出
    pid->Target = 0.0f;            // 控制目标值（如设定转速）
    pid->Actual = 0.0f;            // 传感器反馈值（如实际转速）
    pid->Last_Actual = 0.0f;       // 上一次实际值，微分先行专用
    pid->Error = 0.0f;             // 当前误差 e(n) = Target - Actual
    pid->Prev_Error = 0.0f;        // 上一次误差 e(n-1)
    pid->Prev_Prev_Error = 0.0f;   // 上上一次误差 e(n-2)（仅增量式用）
    pid->Integral = 0.0f;          // 积分累加值（仅位置式用）
    pid->Output = 0.0f;            // 控制器最终输出（如PWM占空比）
}

/**
 * @brief PID核心计算（教学版，分位置式/增量式）
 * @param pid     PID控制器句柄
 * @param target  目标值（想要的电机转速）
 * @param actual  实际值（编码器测到的转速）
 * @param mode    PID模式（位置式/增量式）
 * @return        PID最终输出值
 */
float PID_Compute(PID_Handle_t *pid, float target, float actual, PID_Mode_t mode)
{
    // ===================== 第一步：更新基础值，计算当前误差 =====================
    pid->Target = target;                     // 更新控制目标值
    pid->Actual = actual;                     // 更新传感器反馈值
    pid->Error = pid->Target - pid->Actual;   // 计算当前误差 e(n) = 目标值 - 实际值
    // 【教学重点】误差的物理意义：当前系统“欠多少”或“超多少”

    // ===================== 第二步：分模式计算PID输出 =====================
    if (mode == PID_MODE_POSITIONAL)
    {
        /********************* 位置式PID（全量输出） *********************/
        // 核心公式：Output = Kp*e(n) + Ki*∑e(n) + Kd*[e(n)-e(n-1)]
        float p_out, i_out, diff_out; // 分步计算，教学更清晰

        // 1. 比例项计算：直接与当前误差成正比，快速响应
        p_out = pid->Kp * pid->Error; // 比例输出 P = Kp * e(n)

        // 2. 积分项计算：累加误差，消除静态误差（无误差时积分不再变化）
        // 积分分离：误差超阈值时不积分，防止大误差时积分反而加剧问题
        if (fabs(pid->Error) < PID_INTEG_SEP_THRESH)
        {
            pid->Integral += pid->Error;  // 积分累加 ∑e(n) = 历史积分 + 当前误差
            // 原有积分抗饱和逻辑保留，防止积分过大导致输出失控
            if (pid->Integral * pid->Ki > pid->Integral_Max)
            {
                pid->Integral = pid->Integral_Max / pid->Ki;  // 积分上限限制
            }
        }else
        {
            // 误差过大，停止积分，防止积分反作用
            pid->Integral = 0.0f; // 或者保持不变，根据实际情况调整
        }
        i_out = pid->Ki * pid->Integral; // 积分输出 I = Ki * ∑e(n)

        // 3. 微分项计算：微分先行
        // 替代传统 Kd*[e(n) - e(n-1)]，使用 -Kd*(实际值变化率)，避免目标值跳变冲击微分
        diff_out = pid->Kd * (pid->Actual - pid->Last_Actual); // 微分输出 D = -Kd * Δ实际值

        pid->Output = p_out + i_out - diff_out; // 总输出（减微分是因为diff_out的符号特性）
    }
    else if (mode == PID_MODE_INCREMENTAL)
    {
        /********************* 增量式PID（增量输出） *********************/
        // 核心公式：ΔOutput = Kp*[e(n)-e(n-1)] + Ki*e(n) + Kd*[e(n)-2e(n-1)+e(n-2)]
        // 最终输出：Output = 上次输出 + 增量ΔOutput
        // 【教学重点】增量式输出是“变化量”，适合步进电机等需要增量控制的场景
        
        // 1. 分步计算增量（拆解公式，教学更直观）
        float p_delta = pid->Kp * (pid->Error - pid->Prev_Error);  // 比例增量
        float i_delta = pid->Ki * pid->Error;                      // 积分增量
        float d_delta = pid->Kd * (pid->Error - 2.0f * pid->Prev_Error + pid->Prev_Prev_Error); // 微分增量
        
        // 2. 计算总增量
        float delta_out = p_delta + i_delta + d_delta;
        
        // 3. 增量式输出：在上一次输出基础上加增量
        pid->Output += delta_out;
        // 【教学重点】增量式无需积分限幅（天然抗饱和），因为输出只和误差变化有关
    }

    // ===================== 第三步：通用保护逻辑（所有模式都执行） =====================
    // 1. 输出限幅（教学重点：保护执行器，比如电机PWM不能超过100%）
    if (pid->Output > pid->Output_Max)
    {
        pid->Output = pid->Output_Max;  // 超过上限，限制为最大值
    }
    else if (pid->Output < -pid->Output_Max)
    {
        pid->Output = -pid->Output_Max; // 低于下限，限制为最小值
    }

    // 2. 更新误差历史（为下一次计算做准备，教学重点：时序更新）
    pid->Last_Actual = pid->Actual;         // 实际值后移，微分先行专用
    pid->Prev_Prev_Error = pid->Prev_Error; // e(n-1) → e(n-2)（旧误差后移）
    pid->Prev_Error = pid->Error;           // e(n) → e(n-1)（当前误差变旧误差）

    // 返回最终输出值（供执行器使用，如PWM、电压等）
    return pid->Output;
}
