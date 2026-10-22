/**
 * @file    cur_speed_loop.h
 * @brief   直流有刷电机双环串级控制（电流内环 + 速度外环）接口
 * @author  Dr. GAO
 * @date    2026-07-20
 * @version V1.0
 * @website https://genbotter.taobao.com
 *
 * @note    本模块在 chap11 电流环基础上，叠加速度外环，实现双环串级控制。
 *
 *          ┌──────────┐     ┌───────────┐     ┌──────────┐     ┌─────────┐
 *          │ ω_target ├──→○─┤ 速度PID    ├──→○─┤ 电流限幅  ├──→○─┤ 电流PI  ├──→ PWM
 *          │  (外环)   │  -  │  (100Hz)   │     │ CLAMP     │     │ (1kHz)  │
 *          └──────────┘  ↑  └───────────┘     └──────────┘     └────┬────┘
 *                        │                                          │
 *                        │        ┌──────────┐        ┌───────────┐ │
 *                        └────────┤ 编码器测速 ├←───────┤ TIM编码器  │←┤
 *             实际转速 ←          └──────────┘        └───────────┘ │
 *                                                                    │
 *                        ┌──────────┐        ┌───────────┐          │
 *                        │ 滤波电流   ├────────┤ ADC采样    ├──────────┘
 *                        └──────────┘        └───────────┘
 *
 *
 *          控制频率与调度策略：
 *          - 电流内环：1kHz，在 TIM1 溢出中断中每拍都执行
 *          - 速度外环：100Hz，每 10 次 TIM1 中断执行 1 次（分频）
 *          - 分频整数 = 10，保证相位稳定（无累计误差）
 *
 *          内外环分工：
 *          - 速度外环决策：目标转速 vs 实际转速 → PID 输出"期望电流"
 *          - 电流内环执行：期望电流 vs 实测电流 → PI 输出 PWM 占空比
 *
 *          为什么需要内环电流环：
 *          1. 转速控制器不知道负载变化（堵转、突加载），输出的是"期望"而非实际值
 *          2. 电流环 1kHz 远快于外环 100Hz，扰动在 1ms 内被电流环抑制，外环感觉不到
 *          3. 电流限幅自然实现硬件保护，无论如何不会烧毁电机
 *
 *          前后章节衔接：
 *          - chap10：速度单环 PID → 学习"调参思想和VOFA+观察"
 *          - chap11：电流单环 PI → 学习"电流检测和PID基础"
 *          - chap12：温升实验 → 学习"长期运行考量"
 *          - ★ chap13（本章）：双环串级 → 整合前几章知识，构建完整控制系统
 *          - chap14（预告）：三环串级 + 位置环 + 自整定
 */

#ifndef __CUR_SPEED_LOOP_H
#define __CUR_SPEED_LOOP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "pid.h"                  /* PID_Controller（位置式 PID）      */
#include "bsp_brush_motor.h"      /* BrushMotorConfig, BrushMotor_*   */

/*==============================================================================
 * 类型定义
 *============================================================================*/

/**
 * @brief 双环控制运行状态
 */
typedef enum
{
    CUR_SPEED_LOOP_STOPPED = 0,       /**< 停止：双环冻结，电机 SD 拉低          */
    CUR_SPEED_LOOP_CURRENT_ONLY = 1,  /**< 电流环单独运行（调试用，固??电流给定） */
    CUR_SPEED_LOOP_FULL      = 2      /**< 完整双环：速度外环 + 电流内环           */
} CurSpeedLoopMode;

/**
 * @brief 双环控制实例
 *
 * @note  一个实例控制一个电机。
 *        电流环 PID 输出 → PWM 占空比（直接控制电机）
 *        速度环 PID 输出 → 电流给定值（电流环的目标）
 */
typedef struct
{
    /* ---- 绑定的硬件 ---- */
    BrushMotorConfig *motor;        /**< 电机驱动配置（PWM 通道、SD 引脚等） */

    /* ---- 双 PID 控制器 ---- */
    PID_Controller    cur_pid;      /**< 电流环 PI（内环，1kHz）                */
    PID_Controller    spd_pid;      /**< 速度环 PID（外环，100Hz）              */

    /* ---- 控制状态 ---- */
    CurSpeedLoopMode      mode;         /**< 当前运行模式                           */
    volatile uint16_t tick;         /**< ISR 计数（0→SPEED_DIVIDER-1 循环）    */
    float             cur_setpoint; /**< 电流给定值 (A)，速度环输出经 CLAMP 后 */
    float             target_rpm;   /**< 目标转速 (rpm)                        */

    /* ---- 实时测量值（在 ISR 中更新，供 LCD/串口读取） ---- */
    float             cur_measured; /**< 实测电流 (A)                          */
    float             spd_measured; /**< 实测转速 (rpm)                        */
    int8_t            last_dir;     /**< 上次方向 (1/-1/0，防 ISR 重复切向)      */

} CurSpeedLoopCtrl;

/*==============================================================================
 * 公开接口
 *============================================================================*/

/**
 * @brief 初始化双环控制器
 * @param ctrl  双环实例指针
 * @param motor 绑定的电机驱动配置指针
 * @note  内部初始化电流 PI 和速度 PID 两个控制器（使用默认参数）。
 *        初始化后状态 = STOPPED，需手动调用 CurSpeedLoop_Start() 启动。
 */
void CurSpeedLoop_Init(CurSpeedLoopCtrl *ctrl, BrushMotorConfig *motor);

/**
 * @brief 启动双环控制
 * @param ctrl 双环实例指针
 * @note  使能电机驱动，复位两个 PID 内部状态，启动中断驱动。
 *        此后在 TIM1 溢出中断中调用 CurSpeedLoop_ISR() 即可。
 */
void CurSpeedLoop_Start(CurSpeedLoopCtrl *ctrl);

/**
 * @brief 停止双环控制
 * @param ctrl 双环实例指针
 * @note  停止电机（SD 拉低），保留 PID 参数不重置。
 */
void CurSpeedLoop_Stop(CurSpeedLoopCtrl *ctrl);

/**
 * @brief 设置目标转速
 * @param ctrl 双环实例指针
 * @param rpm  目标转速 (rpm)，正 = 正转，负 = 反转
 */
void CurSpeedLoop_SetSpeed(CurSpeedLoopCtrl *ctrl, float rpm);

/**
 * @brief 电流内环 ISR 入口（由 ADC 注入完成回调调用，1kHz）
 * @param ctrl 双环实例指针
 * @note  每 1ms 由硬件触发，只做电流 PI → PWM 刷新，不碰编码器。
 */
void CurSpeedLoop_CurrentISR(CurSpeedLoopCtrl *ctrl);

/**
 * @brief 速度外环 Task（由 TIM6 周期回调调用，100Hz）
 * @param ctrl 双环实例指针
 * @note  每 10ms 调用一次：读编码器速度 → 速度 PID → CLAMP → 更新 cur_setpoint。
 */
void CurSpeedLoop_SpeedTask(CurSpeedLoopCtrl *ctrl);

/**
 * @brief 仅在调试时：切换到纯电流环模式（断开速度外环）
 * @param ctrl          双环实例指针
 * @param cur_setpoint  固定的电流给定值 (A)
 * @note  用于第一步调参：断开外环，固定电流给定，调好电流 PI。
 */
void CurSpeedLoop_Debug_CurrentOnly(CurSpeedLoopCtrl *ctrl, float cur_setpoint);

/**
 * @brief 设置电流环 PID 参数（在线调参）
 * @param ctrl 双环实例指针
 * @param Kp   比例系数
 * @param Ki   积分系数
 * @param Kd   微分系数（电流环通常为 0）
 */
void CurSpeedLoop_SetCurrentPID(CurSpeedLoopCtrl *ctrl, float Kp, float Ki, float Kd);

/**
 * @brief 设置速度环 PID 参数（在线调参）
 * @param ctrl 双环实例指针
 * @param Kp   比例系数
 * @param Ki   积分系数
 * @param Kd   微分系数
 */
void CurSpeedLoop_SetSpeedPID(CurSpeedLoopCtrl *ctrl, float Kp, float Ki, float Kd);

/**
 * @brief 获取电流误差（供 LCD/串口/VOFA+ 显示）
 */
float CurSpeedLoop_GetCurrentError(const CurSpeedLoopCtrl *ctrl);

/**
 * @brief 获取速度误差（供 LCD/串口/VOFA+ 显示）
 */
float CurSpeedLoop_GetSpeedError(const CurSpeedLoopCtrl *ctrl);

#ifdef __cplusplus
}
#endif

#endif /* __CUR_SPEED_LOOP_H */
