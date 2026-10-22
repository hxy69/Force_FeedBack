/**
 * @file    current_loop.h
 * @brief   直流有刷电机电流环闭环控制接口
 * @note    本模块在开环电流检测工程基础上，接入 PID 控制器实现电流环闭环。
 *
 *          ┌──────────┐     ┌──────────┐     ┌───────────┐     ┌────────┐
 *          │ I_target ├──→○─┤ PID 控制  ├──→○─┤ PWM 桥驱动 ├──→○─┤  电机   │
 *          │  (设定值) │  -  │  (位置式)  │     │ (IR2104)   │     │        │
 *          └──────────┘  ↑  └──────────┘     └───────────┘     └───┬────┘
 *                        │                                         │
 *                        │    ┌──────────────┐     ┌────────────┐  │
 *                        └────┤ 低通滤波 +   │←────┤ ADC1 注入  │←─┘
 *                             │ 零漂校准      │     │ (TIM1触发) │
 *                             └──────────────┘     └────────────┘
 *
 *          控制流程：
 *          1. TIM1 TRGO 上升沿触发 ADC1 注入转换（硬件自动，不占 CPU）
 *          2. ADC 注入中断回调中完成过采样 + 低通滤波 → 得到滤波电流
 *          3. 滤波电流更新后立??在 ISR 内调用 CurrentLoop_Update() 执行 PID
 *          4. PID 输出 → BrushMotor_SetSpeed 更新 PWM 占空比（下个 PWM 周期生效）
 *
 *          控制频率：1000Hz（16kHz/16×过采样），零延迟电流闭环。
 *
 *          方向处理策略：
 *          PID 输出为正 → 正转，|输出| = 占空比
 *          PID 输出为负 → 反转，|输出| = 占空比
 *          PID 输出在死区 → 停止
 */

#ifndef __CURRENT_LOOP_H
#define __CURRENT_LOOP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "pid.h"
#include "brush_motor.h"


// 新增：VOFA+调试用全局变量（尽开启VOFA+时定义）,也可以给显示屏显示等使用
typedef struct
{
    float target_current;   // 目标转速（SpeedLoop_SetTargetRPM设置的值）
    float actual_current;   // 实际转速（滤波后的filtered_rpm）
    float pid_output;   // PID原始输出（未补偿的pid_output）
    float final_output; // 死区补偿+限幅后的最终输出
} CurrentLoop_DebugData_t;


/**
 * @brief 电流环控制状态枚举
 */
typedef enum
{
    CURRENT_LOOP_STOPPED = 0,  /**< 电流环未启动（电机停止，PID 冻结） */
    CURRENT_LOOP_RUNNING = 1   /**< 电流环运行中（闭环控制激活）      */
} CurrentLoopState;

/**
 * @brief 电流环控制实例结构体
 *
 * @note  一个电流环实例绑定一个电机和一个 PID 控制器。
 *        多电机系统中，每个电机创建独立的 CurrentLoopCtrl 变量。
 */
typedef struct
{
    BrushMotorConfig *motor;     /**< 绑定的电机驱动配置（PWM 通道、SD 引脚等） */
    PID_Controller    pid;       /**< PID 控制器实例（位置式）                     */
    CurrentLoopState  state;     /**< 电流环运行状态                               */
    float             target_ma; /**< 当前目标电流（mA），运行时可通过接口修改     */
    MotorDirection    prev_dir;  /**< 上一次设置的方向（避免重复调用 SetDirection） */
    uint16_t          ctrl_health; /**< SD 健康检查计数器（周期性重使能 IR2104）      */
    uint8_t motor_id;
} CurrentLoopCtrl;

/*==============================================================================
 * 公开接口
 *============================================================================*/

/**
 * @brief 初始化电流环控制器
 * @param loop  电流环实例指针
 * @param motor 绑定的电机驱动配置指针
 * @note  初始化 PID 控制器（使用 user_config.h 中的默认参数），
 *        将状态设为 STOPPED，电机保持停止。
 *        调用后需手动调用 CurrentLoop_Start() 启动闭环。
 */
void CurrentLoop_Init(CurrentLoopCtrl *loop, BrushMotorConfig *motor);

/**
 * @brief 启动电流环闭环控制
 * @param loop 电流环实例指针
 * @note  使能电机驱动（SD 拉高），复位 PID 状态（避免历史误差干扰），
 *        将状态切换为 RUNNING。此后主循环中周期性调用
 *        CurrentLoop_Update() 即开始闭环控制。
 */
void CurrentLoop_Start(CurrentLoopCtrl *loop);

/**
 * @brief 停止电流环闭环控制
 * @param loop 电流环实例指针
 * @note  关闭电机驱动（SD 拉低），状态切换为 STOPPED。
 *        PID 内部状态保留不重置，下次 Start 时可选择是否复位。
 */
void CurrentLoop_Stop(CurrentLoopCtrl *loop);

/**
 * @brief 执行一次电流环控制迭代
 * @param loop 电流环实例指针
 * @note  这是电流环的核心函数，由 ADC ISR 内的 BSP_CurrentSensor_Update() 调用。
 *        运行频率 = TIM1 TRGO / 过采样次数 = 16kHz / 16 = 1000Hz（1ms）。
 *
 *        执行步骤：
 *        1. 检查运行状态 → STOPPED 则直接返回
 *        2. 读取当前滤波电流 BSP_CurrentSensor_GetCurrent()
 *        3. PID_Compute(target, actual) → 得到控制量（带符号）
 *        4. 控制量分离：符号 → 方向，幅值 → PWM 占空比
 *        5. BrushMotor_SetDirection / SetSpeed 写入硬件
 *
 *        PID 输出值映射规则：
 *        - 输出 >  正向阈值 → 正转，占空比 = min(输出, 100)
 *        - 输出 < -反向阈值 → 反转，占空比 = min(|输出|, 100)
 *        - 输出在死区范围 → 停止（避免零附近抖动）
 */
void CurrentLoop_Update(CurrentLoopCtrl *loop);

/**
 * @brief 运行时修改目标电流
 * @param loop      电流环实例指针
 * @param target_ma 新的目标电流值（mA）
 * @note  正值 = 正转方向电流，负值 = 反转方向电流。
 *        目标值被钳位在 [0, CURRENT_TARGET_MAX_mA] 范围内。
 *        目标值大幅变化时建议调用 PID_Reset()，或依靠 PID 自身调节。
 */
void CurrentLoop_SetTarget(CurrentLoopCtrl *loop, float target_ma);

/**
 * @brief 获取当前目标电流
 * @param loop 电流环实例指针
 * @return float 目标电流值（mA）
 */
float CurrentLoop_GetTarget(const CurrentLoopCtrl *loop);

/**
 * @brief 获取 PID 控制器最近一次误差
 * @param loop 电流环实例指针
 * @return float 误差值 = target - actual（mA）
 */
float CurrentLoop_GetError(const CurrentLoopCtrl *loop);

/**
 * @brief 获取 PID 控制器当前输出
 * @param loop 电流环实例指针
 * @return float 输出值（占空比%的等效值）
 */
float CurrentLoop_GetOutput(const CurrentLoopCtrl *loop);

/**
 * @brief 获取当前运行状态
 * @param loop 电流环实例指针
 * @return CurrentLoopState STOPPED 或 RUNNING
 */
CurrentLoopState CurrentLoop_GetState(const CurrentLoopCtrl *loop);

/**
 * @brief 在线修改 PID 参数（调参用）
 * @param loop 电流环实例指针
 * @param Kp   新的比例系数
 * @param Ki   新的积分系数
 * @param Kd   新的微分系数
 */
void CurrentLoop_SetPID(CurrentLoopCtrl *loop, float Kp, float Ki, float Kd);

CurrentLoop_DebugData_t CurrentLoop_GetDebugData(void);

#ifdef __cplusplus
}
#endif

#endif /* __CURRENT_LOOP_H */
