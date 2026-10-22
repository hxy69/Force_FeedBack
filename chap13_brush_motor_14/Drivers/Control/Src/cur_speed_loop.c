/**
 * @file    cur_speed_loop.c
 * @brief   直流有刷电机双环串级控制实现（电流内环 + 速度外环）
 * @author  Dr. GAO / GenBotter
 * @date    2026-07-20
 * @version V2.0
 * @website https://genbotter.taobao.com
 *
 * @note    V2.0 架构改动：拆分为两个 ISR 入口
 *
 *          CurSpeedLoop_CurrentISR()  → ADC 注入完成 ISR（1kHz）
 *            → 读电流 → CurrentPI → PWM 刷新（每拍）
 *
 *          CurSpeedLoop_SpeedTask()   → TIM6 周期回调（100Hz）
 *            → 读速度 → SpeedPID → CLAMP 电流给定（每 10ms）
 *
 *          两个 ISR 不同频、不同优先级，共享 cur_setpoint（float 原子操作）。
 */
#include "cur_speed_loop.h"
#include "user_config.h"
#include "bsp_current_sensor.h"
#include "bsp_encoder.h"
#include "bsp_brush_motor.h"

extern volatile int16_t dbg_cur_duty;  /* 来自 main.c，VOFA 诊断用 */

#define CLAMP(val, lo, hi)  ((val) < (lo) ? ((val) = (lo)) : \
                             (val) > (hi) ? ((val) = (hi)) : (val))

/*==============================================================================
 * 公开接口
 *============================================================================*/

void CurSpeedLoop_Init(CurSpeedLoopCtrl *ctrl, BrushMotorConfig *motor)
{
    if (ctrl == NULL || motor == NULL) return;

    ctrl->motor        = motor;               /* 绑定电机硬件，内部通过此句柄调 BSP */
    ctrl->mode         = CUR_SPEED_LOOP_STOPPED; /* 初始状态：停止 */
    ctrl->tick         = 0;                   /* 内部计数器，预留调速用 */
    ctrl->last_dir     = 0;                   /* 上次方向，0=停止，1=正转，-1=反转 */
    ctrl->cur_setpoint = 0.0f;                /* 电流给定 = 0 */
    ctrl->target_rpm   = 0.0f;                /* 目标转速 = 0 */
    ctrl->cur_measured = 0.0f;                /* 实测电流 = 0 */
    ctrl->spd_measured = 0.0f;                /* 实测转速 = 0 */

    /* 电流环 PI（Kd=0，纯 PI），输出 0~100% 占空比 */
    PID_Init(&ctrl->cur_pid, CURRENT_KP, CURRENT_KI, 0.0f,
             CURRENT_OUT_MAX, CURRENT_OUT_MIN, CURRENT_INTEGRAL_LIMIT);

    /* 速度环 PID（Kd>0，PID），输出限幅到电流范围 */
    PID_Init(&ctrl->spd_pid, SPEED_KP, SPEED_KI, SPEED_KD,
             SPEED_OUT_MAX, SPEED_OUT_MIN, SPEED_INTEGRAL_LIMIT);
}

void CurSpeedLoop_Start(CurSpeedLoopCtrl *ctrl)
{
    if (ctrl == NULL) return;
    PID_Reset(&ctrl->cur_pid);    /* 清零电流环积分/误差历史 */
    PID_Reset(&ctrl->spd_pid);    /* 清零速度环积分/误差历史 */
    ctrl->tick         = 0;       /* 重置内部计数器 */
    ctrl->cur_setpoint = 0.0f;    /* 电流给定从零开始 */
    ctrl->mode         = CUR_SPEED_LOOP_FULL; /* 进入双环模式 */
    BrushMotor_Enable();          /* 拉高 SD，使能 IR2104 */
}

void CurSpeedLoop_Stop(CurSpeedLoopCtrl *ctrl)
{
    if (ctrl == NULL) return;
    BrushMotor_Disable();         /* 拉低 SD + 清零 CCR，但保留 TIM1 TRGO→ADC */
    ctrl->mode         = CUR_SPEED_LOOP_STOPPED;
    ctrl->cur_setpoint = 0.0f;
    ctrl->cur_measured = 0.0f;
    ctrl->spd_measured = 0.0f;
    dbg_cur_duty       = 0;       /* 清除调试占空比 */
}

void CurSpeedLoop_SetSpeed(CurSpeedLoopCtrl *ctrl, float rpm)
{
    if (ctrl == NULL) return;
    ctrl->target_rpm = rpm;
}

/*==============================================================================
 * ISR 入口（双入口架构）
 *============================================================================*/

/**
 * @brief 电流内环 ISR — 由 ADC 注入转换完成回调调用（1kHz）
 *
 * @note  每 1ms 执行：读取滤波电流 → 电流 PI → 更新 PWM 占空比。
 *        不处理编码器、不处理速度环。最短路径，零延迟。
 *
 *        方向切换仅在方向改变时调用 SetDirection（避免每拍重启PWM）。
 *        SetDirection 内部 SD拉低→Stop→Start→SD拉高，频繁调用会导致
 *        PWM 输出被反复打断，电机无法稳定运行。
 */
void CurSpeedLoop_CurrentISR(CurSpeedLoopCtrl *ctrl)
{
    float pwm_raw;            /* PID 计算输出的原始占空比（带符号，±100） */
    uint8_t duty;             /* 无符号占空比 0~100 */
    int8_t new_dir = 0;       /* 当前周期计算出的方向：1=正传，-1=反转，0=停止 */

    if (ctrl == NULL || ctrl->mode == CUR_SPEED_LOOP_STOPPED) return;

    ctrl->cur_measured = BSP_CurrentSensor_GetCurrent();  /* 读取实测电流，单位 mA*/

    pwm_raw = PID_Compute(&ctrl->cur_pid, ctrl->cur_setpoint, ctrl->cur_measured);

    /* 死区处理：PID 输出在 ±DEADZONE 以内视为零，避免微小抖动来回切换方向 */
    if (pwm_raw > CURRENT_DEADZONE)
    {
        duty = (uint8_t)pwm_raw;           /* 正转：占空比 = pwm_raw（0~100） */
        BrushMotor_SetSpeed(duty);          /* 更新 CCR 寄存器 */
        new_dir = 1;
    }
    else if (pwm_raw < -CURRENT_DEADZONE)
    {
        duty = (uint8_t)(-pwm_raw);         /* 反转：占空比 = -pwm_raw（取绝对值 0~100） */
        BrushMotor_SetSpeed(duty);
        new_dir = -1;
    }
    else
    {
        BrushMotor_SetSpeed(0);             /* 死区：CCR = 0，PWM 占空比 = 0 */
        duty = 0;
        new_dir = 0;
    }

    dbg_cur_duty = new_dir ? (new_dir > 0 ? (int16_t)duty : -(int16_t)duty) : 0;  /* VOFA+ 诊断：带符号占空比 */

    /* 只在方向改变时调用 SetDirection（避免 ISR 内反复重启 PWM） */
    if (new_dir != ctrl->last_dir)
    {
        if (new_dir == 1)
            BrushMotor_SetDirection(MOTOR_FORWARD);
        else if (new_dir == -1)
            BrushMotor_SetDirection(MOTOR_REVERSE);
        else
            BrushMotor_SetSpeed(0);  /* 死区：停转但保持 SD 高，不拉低 */
        ctrl->last_dir = new_dir;
    }
}

/**
 * @brief 速度外环 Task — 由 TIM6 周期回调调用（100Hz / 每 10ms）
 *
 * @note  读编码器速度 → 速度 PID → CLAMP 限幅 → 更新 cur_setpoint。
 *        cur_setpoint 被下一拍 ADC ISR 自动读取（零同步开销，float 原子操作）。
 */
void CurSpeedLoop_SpeedTask(CurSpeedLoopCtrl *ctrl)
{
    if (ctrl == NULL || ctrl->mode != CUR_SPEED_LOOP_FULL) return;

    ctrl->spd_measured = BSP_Encoder_GetSpeedRPM(ENCODER_PM1); /* 读编码器转速 */

    float cur_raw = PID_Compute(&ctrl->spd_pid, ctrl->target_rpm, ctrl->spd_measured);

    CLAMP(cur_raw, CURRENT_MIN_MA, CURRENT_MAX_MA); /* 安全限幅：无论速度PID输出多大，电流给定不超 ±500mA */

    ctrl->cur_setpoint = cur_raw;  /* 原子写入：4字节 float，Cortex-M4 单条 STR，下一拍 CurrentISR 自动读取 */
}

/*==============================================================================
 * 调参与调试
 *============================================================================*/

void CurSpeedLoop_Debug_CurrentOnly(CurSpeedLoopCtrl *ctrl, float cur_setpoint)
{
    if (ctrl == NULL) return;
    CLAMP(cur_setpoint, CURRENT_MIN_MA, CURRENT_MAX_MA); /* 限幅：不超 ±500mA */
    ctrl->cur_setpoint = cur_setpoint;                  /* 直接赋值电流给定，跳过速度环 */
    ctrl->mode         = CUR_SPEED_LOOP_CURRENT_ONLY;   /* 进入纯电流模式（调试用） */
}

void CurSpeedLoop_SetCurrentPID(CurSpeedLoopCtrl *ctrl, float Kp, float Ki, float Kd)
{
    if (ctrl == NULL) return;
    PID_SetTunings(&ctrl->cur_pid, Kp, Ki, Kd);
}

void CurSpeedLoop_SetSpeedPID(CurSpeedLoopCtrl *ctrl, float Kp, float Ki, float Kd)
{
    if (ctrl == NULL) return;
    PID_SetTunings(&ctrl->spd_pid, Kp, Ki, Kd);
}

float CurSpeedLoop_GetCurrentError(const CurSpeedLoopCtrl *ctrl)
{
    return ctrl ? PID_GetError(&ctrl->cur_pid) : 0.0f;
}

float CurSpeedLoop_GetSpeedError(const CurSpeedLoopCtrl *ctrl)
{
    return ctrl ? PID_GetError(&ctrl->spd_pid) : 0.0f;
}
