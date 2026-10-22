/**
 * @file    pid.c
 * @brief   通用双模 PID 控制器（位置式 / 增量式）实现
 * @author  Dr. GAO
 * @date    2026-06-29
 * @version V2.1
 * @website https://genbotter.taobao.com
 *
 * @note    实现两种数字 PID 算法，通过 PID_SetMode() 切换。
 *
 *          位置式 PID 公式：
 *            u(k) = Kp×e(k) + Ki×Σe(k) + Kd×[e(k)-e(k-1)]
 *            适合：电流环、速度环（有物理阻尼 / 内环兜底）
 *
 *          增量式 PID 公式：
 *            Δu(k) = Kp×[e(k)-e(k-1)] + Ki×e(k) + Kd×[e(k)-2e(k-1)+e(k-2)]
 *            u(k)  = u(k-1) + Δu(k)
 *            适合：位置单环（输出累加自带保持力，KI=0 也能到位）
 *
 *          两者共用同一套误差历史和输出限幅机制。
 */

#include "pid.h"
#include "user_config.h"
#include <string.h>   /* memset */

/*==============================================================================
 * 公开接口实现
 *============================================================================*/

/**
 * @brief 初始化 PID 控制器（默认位置式）
 */
void PID_Init(PID_Controller *pid, float Kp, float Ki, float Kd,
              float out_max, float out_min, float int_limit)
{
    if (pid == NULL) return;

    memset(pid, 0, sizeof(PID_Controller));  /* 清零所有内部状态 */

    pid->Kp      = Kp;                        /* 比例系数 */
    pid->Ki      = Ki;                        /* 积分系数 */
    pid->Kd      = Kd;                        /* 微分系数 */
    pid->mode    = PID_MODE_POSITIONAL;       /* 默认位置式，兼容旧代码 */
    pid->output_max = out_max;                /* 输出上限（电流环=100，速度环=350mA） */
    pid->output_min = out_min;                /* 输出下限（电流环=0，速度环=-350mA） */
    pid->integral_limit = int_limit;          /* 积分限幅值 */
}

/**
 * @brief 执行一次 PID 计算（自动根据 mode 选择算法）
 */
float PID_Compute(PID_Controller *pid, float target, float actual)
{
    if (pid == NULL) return 0.0f;

    /* ---- 计算当前拍误差 ---- */
    pid->target = target;              /* 记录设定值（供 GetError 查询） */
    pid->actual = actual;              /* 记录反馈值（供 GetError 查询） */
    float error = target - actual;     /* 误差 = 给定 - 反馈 */
    pid->error = error;                /* 保存当前误差，供调试验证 */

    if (pid->mode == PID_MODE_INCREMENTAL)
    {
        /* ============================================================
         * 增量式 PID：D 项算在反馈值上，消除设定值突变时的微分尖峰
         *
         * Δu(k) = Kp × [e(k) - e(k-1)]
         *       + Ki × e(k)
         *       - Kd × [y(k) - 2y(k-1) + y(k-2)]
         *
         *  y = feedback（编码器实测值），设定值突变时不产生 D 冲击
         *
         * u(k)  = u(k-1) + Δu(k)
         * accumulated 不钳位——让第一个控制周期的 Δu 自由发挥，
         * 产生足够的瞬时输出克服减速电机静摩擦。后续由调用层的
         * LPF + PWM 硬限幅兜底。
         * ============================================================
         */
        float delta = pid->Kp * (error - pid->error_prev)
                    + pid->Ki * error
                    - pid->Kd * (actual - 2.0f * pid->feedback_prev + pid->feedback_prev2);

        pid->accumulated += delta;

        /* 更新误差/反馈历史 */
        pid->feedback_prev2 = pid->feedback_prev;
        pid->feedback_prev  = actual;
        pid->error_prev2    = pid->error_prev;
        pid->error_prev     = error;

        pid->output = pid->accumulated;
    }
    else
    {
        /* ============================================================
         * 位置式 PID：直接计算绝对控制量
         *
         * u(k) = Kp × e(k) + Ki × Σe + Kd × [e(k) - e(k-1)]
         * ============================================================
         */

        /* 积分累加（输出钳位且误差同向 → 跳过，防止 windup） */
        if (!((pid->output >= pid->output_max && error > 0.0f) ||
              (pid->output <= pid->output_min && error < 0.0f)))
        {
            pid->integral += error;
        }

        /* 积分硬限幅：双重保护，即使条件积分逻辑漏了也不会超限 */
        if (pid->integral >  pid->integral_limit)
            pid->integral =  pid->integral_limit;
        else if (pid->integral < -pid->integral_limit)
            pid->integral = -pid->integral_limit;

        float p_term = pid->Kp * error;                        /* P 项：比例，快速响应 */
        float i_term = pid->Ki * pid->integral;                /* I 项：积分，消除静差 */
        float d_term = pid->Kd * (error - pid->error_prev);    /* D 项：微分，抑制超调 */

        pid->output = p_term + i_term + d_term;   /* 三项求和 */

        /* 输出钳位：确保输出在 [output_min, output_max] 范围内 */
        if (pid->output > pid->output_max)
            pid->output = pid->output_max;
        else if (pid->output < pid->output_min)
            pid->output = pid->output_min;

        pid->error_prev = error;
    }

    return pid->output;
}

/**
 * @brief 重置 PID 控制器内部状态
 */
void PID_Reset(PID_Controller *pid)
{
    if (pid == NULL) return;

    pid->output      = 0.0f;   /* 输出清零 */
    pid->integral    = 0.0f;   /* 积分累加清零（位置式） */
    pid->accumulated = 0.0f;   /* 增量累加清零（增量式） */
    pid->error       = 0.0f;   /* 当前误差 */
    pid->error_prev  = 0.0f;   /* 上一次误差（D项用） */
    pid->error_prev2 = 0.0f;   /* 上上次误差（增量式D项用） */
    pid->feedback_prev  = 0.0f;   /* 上一次反馈值（增量式D项用） */
    pid->feedback_prev2 = 0.0f;   /* 上上次反馈值（增量式D项用） */
}

/**
 * @brief 切换 PID 模式（位置式 ↔ 增量式）
 *
 * @note  切换时清零累加值/积分项，避免旧模式状态干扰。
 */
void PID_SetMode(PID_Controller *pid, PID_Mode_t mode)
{
    if (pid == NULL) return;

    pid->mode = mode;
    pid->accumulated = 0.0f;
    pid->integral    = 0.0f;
    pid->error_prev  = 0.0f;
    pid->error_prev2 = 0.0f;
    pid->feedback_prev  = 0.0f;
    pid->feedback_prev2 = 0.0f;
}

/**
 * @brief 运行时修改 PID 参数（在线调参）
 */
void PID_SetTunings(PID_Controller *pid, float Kp, float Ki, float Kd)
{
    if (pid == NULL) return;
    pid->Kp = Kp;
    pid->Ki = Ki;
    pid->Kd = Kd;
}

/**
 * @brief 运行时修改输出限幅
 */
void PID_SetOutputLimits(PID_Controller *pid, float out_max, float out_min)
{
    if (pid == NULL) return;
    pid->output_max = out_max;
    pid->output_min = out_min;
}

/**
 * @brief 获取当前输出值（不执行计算）
 */
float PID_GetOutput(const PID_Controller *pid)
{
    return pid ? pid->output : 0.0f;
}

/**
 * @brief 获取当前误差（不执行计算）
 */
float PID_GetError(const PID_Controller *pid)
{
    return pid ? pid->error : 0.0f;
}
