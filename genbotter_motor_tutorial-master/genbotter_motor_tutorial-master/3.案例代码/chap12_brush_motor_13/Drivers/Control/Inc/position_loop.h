/**
 * @file    position_loop.h
 * @brief   位置环 PID 闭环控制模块
 * @author  Dr. GAO
 * @date    2026-07-09
 * @version V2.3
 * @website https://genbotter.taobao.com
 *
 * @note    12-03 版本：不再直接接受阶跃目标。
 *          TIM6 ISR 先推进轨迹规划器 → 提取中间目标 → 再调用本模块 PID 跟踪。
 */

#ifndef __POSITION_LOOP_H
#define __POSITION_LOOP_H

#include "main.h"
#include "pid.h"
#include "bsp_brush_motor.h"

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
 * 运行状态
 *============================================================================*/

typedef enum {
    POS_STATE_IDLE     = 0,   /**< 空闲 */
    POS_STATE_MOVING   = 1,   /**< 定位中 */
    POS_STATE_HOLDING  = 2,   /**< 到位保持 */
} PositionState_t;

/*==============================================================================
 * 位置环控制句柄
 *============================================================================*/

typedef struct {
    PID_Controller   pid;            /**< PID 控制器 */
    BrushMotorConfig *motor;         /**< 电机驱动配置 */

    int32_t  target;                 /**< 目标位置（counts） */
    int32_t  feedback;               /**< 当前位置（counts） */
    int32_t  error;                  /**< 位置误差（counts） */

    float    pid_output;             /**< PID 原始输出 */
    float    pwm_filtered;           /**< 滤波后 PWM 值 */
    int16_t  pwm_final;              /**< 最终 PWM 百分比 */

    PositionState_t state;           /**< 运行状态 */
    uint8_t         home_flag;       /**< 回零请求标记 */
    MotorDirection  last_dir;        /**< 上次输出方向（避免反复启停） */
} PositionLoopCtrl;

/*==============================================================================
 * API
 *============================================================================*/

void  PositionLoop_Init(PositionLoopCtrl *ctrl, BrushMotorConfig *motor);
void  PositionLoop_SetTarget(PositionLoopCtrl *ctrl, int32_t target);
void  PositionLoop_Reset(PositionLoopCtrl *ctrl);
void  PositionLoop_RequestHome(PositionLoopCtrl *ctrl);
void  PositionLoop_Update(PositionLoopCtrl *ctrl);

int32_t          PositionLoop_GetTarget(PositionLoopCtrl *ctrl);
int32_t          PositionLoop_GetFeedback(PositionLoopCtrl *ctrl);
int32_t          PositionLoop_GetError(PositionLoopCtrl *ctrl);
float            PositionLoop_GetOutput(PositionLoopCtrl *ctrl);
PositionState_t  PositionLoop_GetState(PositionLoopCtrl *ctrl);
const char*      PositionLoop_StateName(PositionState_t state);

#ifdef __cplusplus
}
#endif

#endif /* __POSITION_LOOP_H */
