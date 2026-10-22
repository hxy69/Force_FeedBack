/**
 * @file    pid_controller.h
 * @brief   PID控制器通用头文件
 * @author  Dr. GAO
 * @date    2025-3-2
 * @version V1.1
 * @note    适配GenBotter Motor-1开发板，支持位置式/增量式PID，可配置限幅
 */
#ifndef __PID_CONTROLLER_H
#define __PID_CONTROLLER_H

#include <stdint.h>

/**
 * @brief PID 计算模式选择
 */
typedef enum {
    PID_MODE_POSITIONAL = 0, // 位置式 PID：输出 = P + I + D
    PID_MODE_INCREMENTAL     // 增量式 PID：输出 = 上次输出 + ΔP + ΔI + ΔD
} PID_Mode_t;

/**
 * @brief PID 控制句柄结构体
 */
typedef struct {
    // --- 参数区 (根据控制对象手动调节) ---
    float Kp;               // 比例系数 (Proportional)
    float Ki;               // 积分系数 (Integral)
    float Kd;               // 微分系数 (Derivative)

    // --- 限制区 (系统保护) ---
    float Output_Max;       // 输出限幅：防止 PWM 超过设定上限 (如 100)
    float Integral_Max;     // 积分限幅：位置式特有，防止积分饱和 (Windup)
    
    // --- 状态区 (运行时自动更新) ---
    float Target;           // 控制目标值 (例如：设定转速 10 RPM)
    float Actual;           // 传感器反馈值 (例如：当前实际转速)
    // 相对brush_motor_6新增下面这1行（必要修改，微分先行用）
    float Last_Actual;      // 上一次实际值，微分先行专用
    float Error;            // 当前误差 e(n) = Target - Actual
    float Prev_Error;       // 上一次误差 e(n-1)
    float Prev_Prev_Error;  // 上上一次误差 e(n-2)，仅增量式计算需要
    float Integral;         // 积分累加值，仅位置式使用
    float Output;           // 控制器最终输出值 (占空比)
} PID_Handle_t;

// 函数声明
/**
 * @brief 初始化PID控制器
 * @param pid      PID控制器句柄指针
 * @param kp       比例系数
 * @param ki       积分系数
 * @param kd       微分系数
 * @param max_out  输出限幅最大值
 * @param max_int  积分限幅最大值
 */
void PID_Init(PID_Handle_t *pid, float kp, float ki, float kd, float max_out, float max_int);

/**
 * @brief 重置PID控制器状态
 * @param pid      PID控制器句柄指针
 * @note 清空误差、积分、输出等状态变量，恢复初始值
 */
void PID_Reset(PID_Handle_t *pid); // 重置 PID 状态（误差、积分等）

/**
 * @brief 执行PID核心计算
 * @param pid      PID控制器句柄指针
 * @param target   控制目标值
 * @param actual   实际反馈值
 * @param mode     PID计算模式（位置式/增量式）
 * @return         PID最终输出值
 */
float PID_Compute(PID_Handle_t *pid, float target, float actual, PID_Mode_t mode); // 计算 PID 输出

#endif
