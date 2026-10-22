/**
 * @file    position_loop.c
 * @brief   直流有刷电机位置环 PID 控制模块
 * @author  Dr. GAO
 * @date    2026-07-09
 * @version V2.3
 * @website https://genbotter.taobao.com
 *
 * @note    12-03 版本：不再接受阶跃目标。
 *          由上层轨迹规划器逐周期推送中间目标，本模块只负责 PID 跟踪。
 *          - 编码器反馈 → 增量式 PID → LPF 滤波 → 限幅 → PWM 输出
 *          - 方向变化时自动处理 IR2104 自举电容重充电
 */

#include "position_loop.h"
#include "bsp_encoder.h"
#include "bsp_brush_motor.h"
#include "user_config.h"
#include <stdlib.h>   /* abs() */

/* 调试变量（定义在 main.c） */
extern volatile uint32_t g_pid_run_cnt;

/*==============================================================================
 * 初始化
 *============================================================================*/

void PositionLoop_Init(PositionLoopCtrl *ctrl, BrushMotorConfig *motor)
{
    if (!ctrl || !motor) return;

    ctrl->motor        = motor;
    ctrl->target       = 0;
    ctrl->feedback     = 0;
    ctrl->error        = 0;
    ctrl->pid_output   = 0.0f;
    ctrl->pwm_filtered = 0.0f;
    ctrl->pwm_final    = 0;
    ctrl->state        = POS_STATE_IDLE;
    ctrl->home_flag    = 0;
    ctrl->last_dir     = MOTOR_STOP;

    PID_Init(&ctrl->pid, POS_KP, POS_KI, POS_KD,
             PWM_LIMIT, PWM_LIMIT_NEG, INTEGRAL_LIMIT);
    PID_SetMode(&ctrl->pid, PID_MODE_SELECT);
}

/*==============================================================================
 * 设定目标位置
 *============================================================================*/

void PositionLoop_SetTarget(PositionLoopCtrl *ctrl, int32_t target)
{
    if (!ctrl) return;

    if (target > POSITION_MAX)  target = POSITION_MAX;
    if (target < POSITION_MIN)  target = POSITION_MIN;

    ctrl->target = target;
    ctrl->state  = POS_STATE_MOVING;
    /* 不再调用 PID_Reset——轨迹规划器逐周期更新目标，PID 必须保持运行状态 */
}

/*==============================================================================
 * 复位到原点（编码器归零 + 状态复位）
 *============================================================================*/

void PositionLoop_Reset(PositionLoopCtrl *ctrl)
{
    if (!ctrl) return;

    BSP_Encoder_Reset(ENCODER_PM1);
    ctrl->target       = 0;
    ctrl->feedback     = 0;
    ctrl->error        = 0;
    ctrl->pid_output   = 0.0f;
    ctrl->pwm_filtered = 0.0f;
    ctrl->pwm_final    = 0;
    ctrl->state        = POS_STATE_IDLE;
    ctrl->home_flag    = 0;
    ctrl->last_dir     = MOTOR_STOP;
    PID_Reset(&ctrl->pid);
}

/*==============================================================================
 * 回零请求：目标置零 → 到达后自动进入 IDLE
 *============================================================================*/

void PositionLoop_RequestHome(PositionLoopCtrl *ctrl)
{
    if (!ctrl) return;

    ctrl->target   = 0;
    ctrl->state    = POS_STATE_MOVING;
    ctrl->home_flag = 1;
    ctrl->last_dir = MOTOR_STOP;
    PID_Reset(&ctrl->pid);
}

/*==============================================================================
 * 位置环核心更新（TIM6 中断内调用，周期 50 ms）
 *============================================================================*/

void PositionLoop_Update(PositionLoopCtrl *ctrl)
{
    if (!ctrl || ctrl->state == POS_STATE_IDLE) return;

    /* ---- 读取编码器反馈 ---- */
    ctrl->feedback = BSP_Encoder_GetCount(ENCODER_PM1);
    ctrl->error    = ctrl->target - ctrl->feedback;

    /* ---- 死区处理 ---- */
    if (abs(ctrl->error) <= DEAD_ZONE)
    {
        ctrl->state = POS_STATE_HOLDING;

        /* 回零完成：禁能电机，进入空闲 */
        if (ctrl->home_flag)
        {
            ctrl->home_flag = 0;
            ctrl->state     = POS_STATE_IDLE;
            BrushMotor_Disable();
            return;
        }
        /* 不退出，继续走 PID 保持位置 */
    }
    else if (ctrl->state == POS_STATE_HOLDING)
    {
        /* 退出死区：恢复移动状态，重置方向标志触发自举重充电 */
        ctrl->state    = POS_STATE_MOVING;
        ctrl->last_dir = MOTOR_STOP;
    }

    /* ---- 增量式 PID 计算 ---- */
    ctrl->pid_output = PID_Compute(&ctrl->pid, (float)ctrl->target, (float)ctrl->feedback);

    /* ---- 一阶低通滤波 + 输出限幅 ---- */
    ctrl->pwm_filtered = ctrl->pwm_filtered * 0.5f + ctrl->pid_output * 0.5f;

    if      (ctrl->pwm_filtered > PWM_LIMIT)     ctrl->pwm_filtered = PWM_LIMIT;
    else if (ctrl->pwm_filtered < PWM_LIMIT_NEG) ctrl->pwm_filtered = PWM_LIMIT_NEG;

    ctrl->pwm_final = (int16_t)ctrl->pwm_filtered;

    /* ---- 最小启动占空比：不足阈值时抬起，克服减速电机静摩擦 ---- */
    if      (ctrl->pwm_final > 0 && ctrl->pwm_final <  (int16_t)PWM_MIN_START) ctrl->pwm_final =  (int16_t)PWM_MIN_START;
    else if (ctrl->pwm_final < 0 && ctrl->pwm_final > -(int16_t)PWM_MIN_START) ctrl->pwm_final = -(int16_t)PWM_MIN_START;

    /* ---- PWM 输出：符号定方向，绝对值定速度 ---- */
    if (ctrl->pwm_final > 0)
    {
        BrushMotor_SetSpeed((uint8_t)ctrl->pwm_final);
        if (MOTOR_FORWARD != ctrl->last_dir)
        {
            BrushMotor_SetDirection(MOTOR_FORWARD);
            ctrl->last_dir = MOTOR_FORWARD;
        }
        else BrushMotor_Enable();
    }
    else if (ctrl->pwm_final < 0)
    {
        BrushMotor_SetSpeed((uint8_t)(-ctrl->pwm_final));
        if (MOTOR_REVERSE != ctrl->last_dir)
        {
            BrushMotor_SetDirection(MOTOR_REVERSE);
            ctrl->last_dir = MOTOR_REVERSE;
        }
        else BrushMotor_Enable();
    }
    else
    {
        BrushMotor_Disable();
    }

    /* ---- 调试计数 ---- */
    g_pid_run_cnt++;
}

/*==============================================================================
 * 状态查询
 *============================================================================*/

int32_t PositionLoop_GetTarget(PositionLoopCtrl *ctrl)   { return ctrl ? ctrl->target   : 0; }
int32_t PositionLoop_GetFeedback(PositionLoopCtrl *ctrl) { return ctrl ? ctrl->feedback : 0; }
int32_t PositionLoop_GetError(PositionLoopCtrl *ctrl)    { return ctrl ? ctrl->error    : 0; }
float   PositionLoop_GetOutput(PositionLoopCtrl *ctrl)   { return ctrl ? ctrl->pid_output : 0.0f; }

PositionState_t PositionLoop_GetState(PositionLoopCtrl *ctrl)
{
    return ctrl ? ctrl->state : POS_STATE_IDLE;
}

const char* PositionLoop_StateName(PositionState_t state)
{
    switch (state)
    {
        case POS_STATE_IDLE:    return "IDLE";
        case POS_STATE_MOVING:  return "MOVING";
        case POS_STATE_HOLDING: return "HOLDING";
        default:                return "?";
    }
}
