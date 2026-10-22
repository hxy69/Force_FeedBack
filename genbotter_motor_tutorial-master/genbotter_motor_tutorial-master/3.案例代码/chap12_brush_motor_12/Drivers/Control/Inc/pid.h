/**
 * @file    pid.h
 * @brief   通用 PID 控制器（位置式）头文件
 * @author  Dr. GAO
 * @date    2026-06-29
 * @version V2.0
 * @website https://genbotter.taobao.com
 *
 * @note    实现位置式数字 PID 控制算法，适用于电机电流环 / 速度环 / 位置环。
 *
 *          位置式 PID 公式：
 *          ┌─────────────────────────────────────────────────────────┐
 *          │  u(k) = Kp × e(k)                                       │
 *          │       + Ki × Σe(k)           ← 积分累加，消除稳态误差   │
 *          │       + Kd × [e(k) - e(k-1)]  ← 微分抑制超调            │
 *          └─────────────────────────────────────────────────────────┘
 *
 *          相比于增量式 PID 的优势：
 *          - P 项直接与误差大小成正比，误差大时输出立即大，响应快
 *          - 适合电流环等需要快速消除固定误差的场景
 *          - 公式直观，调参时 Kp/Ki/Kd 的物理意义更明确
 *
 *          注意：位置式 PID 需要对积分项做抗饱和处理（遇限削弱积分/
 *          积分分离），否则误差持续存在时积分会无限增长（windup）。
 */

#ifndef __PID_H
#define __PID_H

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
 * 类型定义
 *============================================================================*/

/**
 * @brief PID 模式选择
 */
typedef enum {
    PID_MODE_POSITIONAL  = 0,   /**< 位置式：u(k) = Kp×e(k) + Ki×Σe + Kd×[e(k)-e(k-1)] */
    PID_MODE_INCREMENTAL = 1    /**< 增量式：Δu = Kp×[e(k)-e(k-1)] + Ki×e(k) + Kd×[e(k)-2e(k-1)+e(k-2)] */
} PID_Mode_t;

/**
 * @brief PID 控制器实例结构体
 *
 * @note  每个被控对象（如一路电流环）需要独立的 PID 实例。
 *        例如：多电机控制时，为每个电机创建一个 PID_Controller 变量。
 *
 *        模式选择：
 *        - 位置式：直接计算绝对控制量，适合电流环/速度环（有物理阻尼）
 *        - 增量式：计算控制增量并累加，适合位置单环（自带"保持力"，KI=0 也能到位）
 */
typedef struct
{
    /* ---- 用户可调参数 ---- */
    float Kp;           /**< 比例系数                                          */
    float Ki;           /**< 积分系数（增量式直接乘 e(k)，位置式乘 Σe）        */
    float Kd;           /**< 微分系数                                          */
    PID_Mode_t mode;    /**< 控制模式：位置式 or 增量式                        */

    float output_max;   /**< 输出上限（位置式钳位值，增量式建议设很大）        */
    float output_min;   /**< 输出下限                                          */
    float integral_limit; /**< 积分项幅值限制（防积分饱和 windup）              */

    /* ---- 目标值与反馈值 ---- */
    float target;       /**< 设定目标值                                        */
    float actual;       /**< 当前实际值                                        */

    /* ---- 内部状态 ---- */
    float error;        /**< 当前拍误差 e(k)                                   */
    float error_prev;   /**< 上一拍误差 e(k-1)，用于比例项与微分项                   */
    float error_prev2;  /**< 前两拍误差 e(k-2)，增量式微分需要                       */
    float feedback_prev;  /**< 上一拍实测值 y(k-1)，D-on-measurement 需要            */
    float feedback_prev2; /**< 前两拍实测值 y(k-2)，D-on-measurement 需要            */
    float output;       /**< 当前控制器输出值                                  */
    float integral;     /**< 积分累加值 Σe(k)，带限幅                           */
    float accumulated;  /**< 增量式输出累加值                                  */

} PID_Controller;

/*==============================================================================
 * 公开接口
 *============================================================================*/

/**
 * @brief 初始化 PID 控制器
 * @param pid       PID 控制器实例指针
 * @param Kp        比例系数
 * @param Ki        积分系数
 * @param Kd        微分系数
 * @param out_max   输出上限
 * @param out_min   输出下限
 * @param int_limit 积分限幅
 * @note  调用后将清零所有内部状态（误差、输出、积分）。
 */
void PID_Init(PID_Controller *pid, float Kp, float Ki, float Kd,
              float out_max, float out_min, float int_limit);

/**
 * @brief 执行一次 PID 计算（位置式）
 * @param pid    PID 控制器实例指针
 * @param target 本次控制周期的目标值
 * @param actual 本次控制周期的实测值
 * @return float 控制输出值（已限幅）
 * @note  调用周期必须稳定（如每 2ms 一次），否则 Ki/Kd 的实际效果会偏离设计预期。
 *        返回值范围 = [output_min, output_max]。
 *
 *        使用示例：
 *        @code
 *        float duty = PID_Compute(&current_pid, target_ma, measured_ma);
 *        BrushMotor_SetSpeed((uint8_t)duty);
 *        @endcode
 */
float PID_Compute(PID_Controller *pid, float target, float actual);

/**
 * @brief 重置 PID 控制器内部状态
 * @param pid PID 控制器实例指针
 * @note  清零误差、积分项、输出值。
 *        适用场景：电机启停切换、目标值大幅跳变、过流保护复位后。
 */
void PID_Reset(PID_Controller *pid);

/**
 * @brief 运行时修改 PID 模式（位置式 ↔ 增量式）
 * @param pid  PID 控制器实例指针
 * @param mode 新模式
 * @note  切换模式会自动清零累加值/积分项，避免旧状态干扰。
 */
void PID_SetMode(PID_Controller *pid, PID_Mode_t mode);

/**
 * @brief 运行时修改 PID 参数（在线调参）
 * @param pid PID 控制器实例指针
 * @param Kp  新的比例系数
 * @param Ki  新的积分系数
 * @param Kd  新的微分系数
 * @note  可在系统运行中通过串口/按键调用，实时调整控制效果。
 *        参数突变可能导致瞬时输出跳变，建议渐变调整。
 */
void PID_SetTunings(PID_Controller *pid, float Kp, float Ki, float Kd);

/**
 * @brief 运行时修改输出限幅
 * @param pid      PID 控制器实例指针
 * @param out_max  新的输出上限
 * @param out_min  新的输出下限
 */
void PID_SetOutputLimits(PID_Controller *pid, float out_max, float out_min);

/**
 * @brief 获取当前输出值（不执行计算，仅读取）
 * @param pid PID 控制器实例指针
 * @return float 当前输出值
 */
float PID_GetOutput(const PID_Controller *pid);

/**
 * @brief 获取最近一次计算的误差（不执行计算，仅读取）
 * @param pid PID 控制器实例指针
 * @return float 最近一次误差 = target - actual
 */
float PID_GetError(const PID_Controller *pid);

#ifdef __cplusplus
}
#endif

#endif /* __PID_H */
